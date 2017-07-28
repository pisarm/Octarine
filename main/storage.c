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

#include "storage.h"

#include "nvs.h"

#define STORAGE "storage"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASSWORD_KEY "password"
#define TIMEZONE_KEY "timezone"
#define TRAFFIC_DESTINATION_KEY "traffic_destnation"
#define TRAFFIC_ORIGIN_KEY "traffic_origin"

esp_err_t load(char *storage_key, char *storage_value, size_t length) {
    nvs_handle storage_handle;
    nvs_open(STORAGE, NVS_READONLY, &storage_handle);
    
    esp_err_t status = nvs_get_str(storage_handle, storage_key, storage_value, &length);
    if (status != ESP_OK) {    
        nvs_close(storage_handle);
        return status;
    }

    nvs_close(storage_handle);

    return ESP_OK;
}

esp_err_t store(char *storage_key, char *storage_value) {
    nvs_handle storage_handle;
    nvs_open(STORAGE, NVS_READWRITE, &storage_handle);

    esp_err_t status = nvs_set_str(storage_handle, storage_key, storage_value);
    if (status != ESP_OK) {
        return status;
    }

    nvs_commit(storage_handle);
    nvs_close(storage_handle);

    return ESP_OK;
}

esp_err_t load_timezone(char *timezone) {
    size_t length = STORAGE_TIMEZONE_MAX;    
    return load(TIMEZONE_KEY, timezone, length);
}

esp_err_t store_timezone(char *timezone) {
    return store(TIMEZONE_KEY, timezone);
}

esp_err_t load_traffic_destination(char *destination) {
    size_t length = STORAGE_TRAFFIC_DESTINATION_MAX;
    return load(TRAFFIC_DESTINATION_KEY, destination, length);
}

esp_err_t store_traffic_destination(char *destination) {
    return store(TRAFFIC_DESTINATION_KEY, destination);
}

esp_err_t load_traffic_origin(char *origin) {
    size_t length = STORAGE_TRAFFIC_ORIGIN_MAX;
    return load(TRAFFIC_ORIGIN_KEY, origin, length);
}

esp_err_t store_traffic_origin(char *origin) {
    return store(TRAFFIC_ORIGIN_KEY, origin);
}

esp_err_t load_wifi_credentials(char *ssid, char *password) {
    nvs_handle storage_handle;
    nvs_open(STORAGE, NVS_READONLY, &storage_handle);

    size_t length = STORAGE_WIFI_SSID_MAX;
    esp_err_t status = nvs_get_str(storage_handle, WIFI_SSID_KEY, ssid, &length);
    if (status != ESP_OK) {    
        nvs_close(storage_handle);
        return status;
    }

    length = STORAGE_WIFI_PASSWORD_MAX;
    status = nvs_get_str(storage_handle, WIFI_PASSWORD_KEY, password, &length);
    if (status != ESP_OK) {
        nvs_close(storage_handle);
        return status;
    }

    nvs_close(storage_handle);

    return ESP_OK;
}

esp_err_t store_wifi_credentials(char *ssid, char *password) {
    nvs_handle storage_handle;
    nvs_open(STORAGE, NVS_READWRITE, &storage_handle);

    esp_err_t status = nvs_set_str(storage_handle, WIFI_SSID_KEY, ssid);
    if (status != ESP_OK) {
        return status;
    }

    status = nvs_set_str(storage_handle, WIFI_PASSWORD_KEY, password);
    if (status != ESP_OK) {
        return status;
    }

    nvs_commit(storage_handle);
    nvs_close(storage_handle);

    return ESP_OK;
}
