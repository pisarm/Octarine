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

#include "weather.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

// #include "mbedtls/platform.h"
// #include "mbedtls/ctr_drbg.h"
// #include "mbedtls/debug.h"
// #include "mbedtls/entropy.h"
// #include "mbedtls/error.h"
// #include "mbedtls/net.h"
// #include "mbedtls/ssl.h"

#include <stdio.h>
#include <string.h>


#include <cJSON.h>

#include "network.h"

#define USER_AGENT "Octarine"
#define WEB_HOST "api.darksky.net"
#define WEB_PORT "443"

static char tag[] = "weather";

static weather_config_t config;
static weather_data_callback callback;

void parse_weather_data(network_response_data_t *response);

static char *request_message_template = 
    "GET /forecast/%s/%f,%f?exclude=[minutely,hourly,daily,alerts,flags]&units=si HTTP/1.1\r\n"
    "User-Agent: "USER_AGENT"\r\n"
    "Host: "WEB_HOST"\r\n"
    "Connection: close\r\n"
    "Accept-Language: en-us\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "\r\n";

static weather_data_t weather_data;
// static network_response_data_t *network_response_data;

void init_weather(weather_config_t weather_config, weather_data_callback weather_data_callback) {
    config = weather_config;
    callback = weather_data_callback;
}

// void updated_response_data(network_response_data_t *response_data) {
void updated_response_data() {    
    // printf("pew\n%s\n", network_response_data->raw_data);
    // char *raw_json = strstr(response_data->raw_data, "\r\n\r\n");
    // if (raw_json) { raw_json += 4; }

    // cJSON *root = cJSON_Parse(raw_json);

    // cJSON *currently = cJSON_GetObjectItem(root, "currently");
    
    // cJSON *temperature_item = cJSON_GetObjectItem(currently, "temperature");
    // weather_data.temperature = temperature_item->valuedouble;

    // cJSON *summary_item = cJSON_GetObjectItem(currently, "summary");
    // weather_data.summary = summary_item->valuestring;

    // cJSON *precip_probability_item = cJSON_GetObjectItem(currently, "precipProbability");
    // weather_data.precip_probability = precip_probability_item->valuedouble;

    // cJSON *uv_index_item = cJSON_GetObjectItem(currently, "uvIndex");
    // weather_data.uv_index = uv_index_item->valueint;

    // cJSON_Delete(root);

    // callback(&weather_data);    
}

void weather_request_task(void *args) {
    char *request = malloc(384 * sizeof(char));
    sprintf(request, request_message_template, config.secret_key, config.latitude, config.longitude);

    network_config_t network_config = {
        .hostname = WEB_HOST,
        .host_port = WEB_PORT,
        .request = request
    };

    ESP_LOGI(tag, "Fetching weather data");    

    network_response_data_t *response = network_perform_request(network_config);

    ESP_LOGI(tag, "Weather data fetched")

    free(request);
    //TODO: handle reponse not being correct
    parse_weather_data(response);

    free(response);
    free(response->raw_data);
    
    vTaskDelete(NULL);
}

void parse_weather_data(network_response_data_t *response) {
    ESP_LOGI(tag, "Parsing weather data");
    
    char *raw_json = strstr(response->raw_data, "\r\n\r\n");
    if (raw_json) { raw_json += 4; }

    cJSON *root = cJSON_Parse(raw_json);

    cJSON *currently = cJSON_GetObjectItem(root, "currently");
    ESP_LOGI(tag, "| Currently");
    
    cJSON *temperature_item = cJSON_GetObjectItem(currently, "temperature");
    weather_data.temperature = temperature_item->valuedouble;
    ESP_LOGI(tag, "|-> Temperature (%f)", weather_data.temperature);
    
    cJSON *summary_item = cJSON_GetObjectItem(currently, "summary");
    weather_data.summary = summary_item->valuestring;
    ESP_LOGI(tag, "|-> Summary (%s)", weather_data.summary);

    cJSON *precip_probability_item = cJSON_GetObjectItem(currently, "precipProbability");
    weather_data.precip_probability = precip_probability_item->valuedouble;
    ESP_LOGI(tag, "|-> PrecipProbability (%f)", weather_data.precip_probability);
    
    cJSON *uv_index_item = cJSON_GetObjectItem(currently, "uvIndex");
    weather_data.uv_index = uv_index_item->valueint;
    ESP_LOGI(tag, "|-> UVIndex (%d)", weather_data.uv_index);

    cJSON_Delete(root);

    ESP_LOGI(tag, "Weather data parsed")
}
