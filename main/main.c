#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include "apps/sntp/sntp.h"
#include "driver/gpio.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mongoose.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "display.h"
#include "endpoints.h"
#include "storage.h"

const int WIFI_CONNECTED_BIT = BIT0;
const int AP_START_BIT = BIT1;
const int TIME_UPDATED_BIT = BIT2;

static EventGroupHandle_t event_group;

// Function prototypes

static void setup_wifi(char *ssid, char *password);
static void setup_sntp(void);

static void ap_task(void *args);
static void sntp_task(void* args);
static void mongoose_task(void *data);
static void mongoose_ap_task(void *data);
static void display_time_task(void *data);

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
static void mongoose_event_handler(struct mg_connection *nc, int ev, void *evData);

// Main

void app_main() {
    nvs_flash_init();

    display_init();    
    display_draw_center("Octarine");

    event_group = xEventGroupCreate();

    char ssid[SSID_MAX];
    char password[PASSWORD_MAX];
    int setupOverride = gpio_get_level(GPIO_NUM_16);

    // Wifi credentials not found - starting up AP
    if (get_wifi_settings(ssid, password) != ESP_OK || setupOverride == 1) {
        xTaskCreate(ap_task, "ap_task", 2048, (void *) 0, 10, NULL);        
        xTaskCreatePinnedToCore(mongoose_ap_task, "mongoose_ap_task", 20480, NULL, 5, NULL, 0);
    } else {
        xTaskCreate(display_time_task, "display_time_task", 2048, (void *) 0, 10, NULL);
        xTaskCreate(sntp_task, "sntp_task", 2048, (void *) 0, 10, NULL);
        xTaskCreatePinnedToCore(&mongoose_task, "mongoose_task", 20480, NULL, 5, NULL, 0);

        setup_wifi(ssid, password);
    }
}

static void setup_wifi(char *ssid, char *password) {
    tcpip_adapter_init();
    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "Octarine");
    ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    wifi_config_t wifi_config;
    memcpy(wifi_config.sta.ssid, ssid, 32);
    memcpy(wifi_config.sta.password, password, 64);
    
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void setup_sntp(void) {    
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static esp_err_t ap_event_handler (void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_AP_START:       
        xEventGroupSetBits(event_group, AP_START_BIT);
        break;

        default:
        break;
    }

    return ESP_OK;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(event_group, WIFI_CONNECTED_BIT);

        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        // This is a workaround as ESP32 WiFi libs don't currently auto-reassociate.
        esp_wifi_connect();
        xEventGroupClearBits(event_group, WIFI_CONNECTED_BIT);
        break;

    default:
        break;
    }

    return ESP_OK;
}

static void mongoose_event_handler(struct mg_connection *connection, int event, void *event_data) { }
static void mongoose_ap_event_handler(struct mg_connection *nc, int ev, void *evData) { }

static void ap_task(void *args) {
    tcpip_adapter_init();
    esp_event_loop_init(ap_event_handler, NULL);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_AP);
    wifi_config_t ap_config = {
	   .ap = {
	      .ssid = "Octarine",
	      .ssid_len = 0,
	      .password = "",
	      .channel = 7,
	      .authmode = WIFI_AUTH_OPEN,
	      .ssid_hidden = 0,
	      .max_connection = 4,
	      .beacon_interval = 100
	   }
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
	esp_wifi_start();    
	
	vTaskDelete(NULL);
}

static void sntp_task(void* args) {
    xEventGroupWaitBits(event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    setup_sntp();

    time_t now;

    int retry = 1;
    const int retry_count = 10;

    do {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Getting system time (%d/%d)\n", retry, retry_count);
        time(&now);
    } while((long long)now == 0 && ++retry < retry_count);

    printf("Got time -> %lld\n", (long long)now);
    xEventGroupSetBits(event_group, TIME_UPDATED_BIT);

    vTaskDelete(NULL);
}

static void mongoose_task(void *data) {
    xEventGroupWaitBits(event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, NULL);

	struct mg_connection *connection = mg_bind(&mgr, ":80", mongoose_event_handler);

    mg_register_http_endpoint(connection, "/", root_endpoint);
    mg_register_http_endpoint(connection, "/config", config_endpoint);
    mg_register_http_endpoint(connection, "/config/time", config_time_endpoint);
    mg_register_http_endpoint(connection, "/config/wifi", config_wifi_endpoint);

	if (connection == NULL) {
		vTaskDelete(NULL);
		return;
	}
	mg_set_protocol_http_websocket(connection);

    for(;;) {
		mg_mgr_poll(&mgr, 1000);
	}
}

static void mongoose_ap_task(void *data) {
    xEventGroupWaitBits(event_group, AP_START_BIT, false, true, portMAX_DELAY);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    struct mg_connection *connection = mg_bind(&mgr, ":80", mongoose_ap_event_handler);

    mg_register_http_endpoint(connection, "/", ap_root_endpoint);
    mg_register_http_endpoint(connection, "/config", config_endpoint);
    mg_register_http_endpoint(connection, "/config/time", config_time_endpoint);
    mg_register_http_endpoint(connection, "/config/wifi", config_wifi_endpoint);

    if (connection == NULL) {
        vTaskDelete(NULL);
        return;
    }

    mg_set_protocol_http_websocket(connection);

    for(;;) {
        mg_mgr_poll(&mgr, 1000);
    }
}

static void display_time_task(void *data) {
    xEventGroupWaitBits(event_group, TIME_UPDATED_BIT, false, true, portMAX_DELAY);

    char time[32];
    struct timeval tv;

    for(;;) {
        gettimeofday(&tv, NULL);
        // sprintf(time, "%d.%d", (int)tv.tv_sec, (int)tv.tv_usec);
        sprintf(time, "%d", (int)tv.tv_sec);
        display_draw_center(time);

        vTaskDelay(333 / portTICK_PERIOD_MS);
    }
}

// static const char *TAG = "ssd1306";

// void task_test_SSD1306i2c(void *ignore) {
//     xEventGroupWaitBits(event_group, TIME_UPDATED_BIT, false, true, portMAX_DELAY);

//     char time[32];
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     // sprintf(time, "%d.%d", (int)tv.tv_sec, (int)tv.tv_usec);
//     sprintf(time, "%d", (int)tv.tv_sec);

// 	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
// 	u8g2_esp32_hal.sda   = PIN_SDA;
// 	u8g2_esp32_hal.scl  = PIN_SCL;
// 	u8g2_esp32_hal_init(u8g2_esp32_hal);


// 	u8g2_t u8g2; // a structure which will contain all the data for one display
// 	u8g2_Setup_sh1106_128x64_noname_f(
// 		&u8g2,
// 		U8G2_R0,
// 		//u8x8_byte_sw_i2c,
// 		u8g2_esp32_msg_i2c_cb,
// 		u8g2_esp32_msg_i2c_and_delay_cb);  // init u8g2 structure
// 	u8x8_SetI2CAddress(&u8g2.u8x8,0x78);

// 	ESP_LOGI(TAG, "u8g2_InitDisplay");
// 	u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,

// 	ESP_LOGI(TAG, "u8g2_SetPowerSave");
// 	u8g2_SetPowerSave(&u8g2, 0); // wake up display
// 	ESP_LOGI(TAG, "u8g2_ClearBuffer");
// 	// u8g2_ClearBuffer(&u8g2);
// 	// ESP_LOGI(TAG, "u8g2_DrawBox");
// 	// u8g2_DrawBox(&u8g2, 0, 26, 80,6);
// 	// u8g2_DrawFrame(&u8g2, 0,26,100,6);

// 	ESP_LOGI(TAG, "u8g2_SetFont");
//     // u8g2_SetFontDirection(&u8g2, 1);
//     u8g2_SetFont(&u8g2, u8g2_font_courR12_tr);
// 	ESP_LOGI(TAG, "u8g2_DrawStr");
//     u8g2_DrawStr(&u8g2, 0, 64, time);
// 	ESP_LOGI(TAG, "u8g2_SendBuffer");
// 	u8g2_SendBuffer(&u8g2);

// 	ESP_LOGI(TAG, "All done!");

// 	vTaskDelete(NULL);
// }