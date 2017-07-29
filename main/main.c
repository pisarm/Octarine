/*-
 * Copyright (c) 2017 Flemming Pedersen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "apps/sntp/sntp.h"
#include "driver/gpio.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mongoose.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "mdns.h"

#include "button.h"
#include "display.h"
#include "endpoints.h"
#include "storage.h"

const int WIFI_CONNECTED_BIT = BIT0;
const int AP_STARTED_BIT = BIT1;
const int TIME_UPDATED_BIT = BIT2;

static EventGroupHandle_t g_event_group;

static char formatted_time[8];
static char formatted_ip_addr[32]; 
static char formatted_mac_addr[32];

// Function prototypes

static void setup_wifi(char *ssid, char *password);
static void setup_ap();
static void start_mdns_service();

static void sntp_task(void* args);
static void http_task(void *data);
static void update_display_time_task(void *data);
static void update_display_status_task(void *args);

static esp_err_t wifi_event_handler(void *, system_event_t *);
static void http_event_handler(struct mg_connection *, int, void *);

// Main

void app_main() {
    nvs_flash_init();
    button_init();
    display_init();        
    display_draw_center("Octarine");

    g_event_group = xEventGroupCreate();

    char ssid[STORAGE_WIFI_SSID_MAX];
    char password[STORAGE_WIFI_PASSWORD_MAX];
    int setupOverride = gpio_get_level(BUTTON_GPIO);

    // Wifi credentials not found - starting up AP
    if (load_wifi_credentials(ssid, password) != ESP_OK || setupOverride == 1) {
        display_draw_line0_line1("Connect to", "'Octarine' AP");   
        xTaskCreatePinnedToCore(&http_task, "http_task", 20480, NULL, 5, NULL, 0);

        setup_ap();
    } else {
        xTaskCreate(update_display_time_task, "update_display_time_task", 2048, (void *) 0, 10, NULL);
        xTaskCreate(update_display_status_task, "update_display_status_task", 2048, (void *) 0, 10, NULL);
        xTaskCreate(sntp_task, "sntp_task", 2048, (void *) 0, 10, NULL);
        xTaskCreate(button_task, "button_task", 2048, (void *) 0, 10, NULL);
        xTaskCreate(display_task, "display_task", 3072, (void *) 0, 7, NULL);
        xTaskCreatePinnedToCore(&http_task, "http_task", 20480, NULL, 5, NULL, 0);

        setup_wifi(ssid, password);
        start_mdns_service();
    }
}

static void setup_wifi(char *ssid, char *password) {
    tcpip_adapter_init();
    
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

    ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "Octarine") );
}

static void setup_ap() {
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
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
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_AP, &ap_config) );
	ESP_ERROR_CHECK( esp_wifi_start() );    
}

mdns_server_t * mdns = NULL;

static void start_mdns_service() {
    xEventGroupWaitBits(g_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    esp_err_t err = mdns_init(TCPIP_ADAPTER_IF_STA, &mdns);
    if (err) {
        return;
    }

    ESP_ERROR_CHECK( mdns_set_hostname(mdns, "Octarine") );
    ESP_ERROR_CHECK( mdns_set_instance(mdns, "Octarine") );
    ESP_ERROR_CHECK( mdns_service_add(mdns, "_http", "_tcp", 80) );
    ESP_ERROR_CHECK( mdns_service_instance_set(mdns, "_http", "_tcp", "Octarine Web Interface") );
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_START:       
        xEventGroupSetBits(g_event_group, AP_STARTED_BIT);
        break;

    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(g_event_group, WIFI_CONNECTED_BIT);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        // This is a workaround as ESP32 WiFi libs don't currently auto-reassociate.
        esp_wifi_connect();
        xEventGroupClearBits(g_event_group, WIFI_CONNECTED_BIT);
        break;

    default:
        break;
    }

    return ESP_OK;
}

static void http_event_handler(struct mg_connection *connection, int event, void *event_data) { }

static void sntp_task(void* args) {
    xEventGroupWaitBits(g_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now;
    int retry = 1;
    const int retry_count = 10;

    do {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        time(&now);
    } while((long long)now == 0 && ++retry < retry_count);

    xEventGroupSetBits(g_event_group, TIME_UPDATED_BIT);
    
    vTaskDelete(NULL);
}

static void http_task(void *data) {
    xEventGroupWaitBits(g_event_group, WIFI_CONNECTED_BIT | AP_STARTED_BIT, false, false, portMAX_DELAY);
    
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, NULL);

	struct mg_connection *connection = mg_bind(&mgr, ":80", http_event_handler);

    struct Endpoint* ptr = ENDPOINTS_TO_REGISTER;
    for (int i=0; i < ENDPOINT_COUNT; ++i, ++ptr ) {
        mg_register_http_endpoint(connection, ptr->uri_path, ptr->handler);
    }

	if (connection == NULL) {
		vTaskDelete(NULL);
		return;
	}
	mg_set_protocol_http_websocket(connection);

    for(;;) {
		mg_mgr_poll(&mgr, 1000);
	}
}

static void update_display_time_task(void *args) {
    xEventGroupWaitBits(g_event_group, TIME_UPDATED_BIT | WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    time_t now;
    struct tm timeinfo;
    
    for(;;) {
        char timezone[STORAGE_TIMEZONE_MAX];
        if (load_timezone(timezone) == ESP_OK) {
            setenv("TZ", timezone, 1);  
            tzset();
        } 

        time(&now);
        localtime_r(&now, &timeinfo);

        strftime(formatted_time, sizeof(formatted_time), "%H.%M", &timeinfo);
        display_time = formatted_time;

        vTaskDelay(666 / portTICK_PERIOD_MS);
    }
}

static void update_display_status_task(void *args) {
    xEventGroupWaitBits(g_event_group, TIME_UPDATED_BIT | WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    tcpip_adapter_ip_info_t ipinfo;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipinfo);
    sprintf(formatted_ip_addr, IPSTR, IP2STR(&ipinfo.ip));

    display_ip_addr = formatted_ip_addr;

    uint8_t mac_tmp[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_tmp);
    sprintf(formatted_mac_addr, MACSTR, MAC2STR(mac_tmp));
    strupr(formatted_mac_addr);

    display_mac_addr = formatted_mac_addr;

    vTaskDelete(NULL);
}
