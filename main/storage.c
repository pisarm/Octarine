#include "storage.h"

#include "nvs.h"

#define STORAGE "storage"
#define STORAGE_SSID "ssid"
#define STORAGE_PASSWORD "password"

esp_err_t get_wifi_settings(char *ssid, char *password) {
    nvs_handle storage_handle;
    nvs_open(STORAGE, NVS_READONLY, &storage_handle);

    esp_err_t err;
    size_t length;

    length = SSID_MAX;
    err = nvs_get_str(storage_handle, STORAGE_SSID, ssid, &length);
    if (err != ESP_OK) {    
        nvs_close(storage_handle);
        return err;
    }

    length = PASSWORD_MAX;
    err = nvs_get_str(storage_handle, STORAGE_PASSWORD, password, &length);
    if (err != ESP_OK) {
        nvs_close(storage_handle);
        return err;
    }

    nvs_close(storage_handle);

    return ESP_OK;
}

esp_err_t set_wifi_settings(char *ssid, char *password) {
    nvs_handle storage_handle;
    nvs_open(STORAGE, NVS_READWRITE, &storage_handle);

    esp_err_t err;

    err = nvs_set_str(storage_handle, STORAGE_SSID, ssid);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(storage_handle, STORAGE_PASSWORD, password);
    if (err != ESP_OK) {
        return err;
    }

    nvs_commit(storage_handle);
    nvs_close(storage_handle);

    return ESP_OK;
}