#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <u8g2.h>
#include "apps/sntp/sntp.h"
#include "endpoints.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mongoose.h"
#include "nvs_flash.h"
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

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
static void mongoose_event_handler(struct mg_connection *nc, int ev, void *evData);


// Main

void app_main() {
    nvs_flash_init();

    event_group = xEventGroupCreate();

    char ssid[SSID_MAX];
    char password[PASSWORD_MAX];

    // Wifi credentials not found - starting up AP
    if (get_wifi_settings(ssid, password) != ESP_OK) {
        xTaskCreate(ap_task, "ap_task", 2048, (void *) 0, 10, NULL);
        xTaskCreatePinnedToCore(mongoose_ap_task, "mongoose_ap_task", 20480, NULL, 5, NULL, 0);
    } else {
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
