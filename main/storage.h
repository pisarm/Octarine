#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"

#define SSID_MAX 32
#define PASSWORD_MAX 64

esp_err_t get_wifi_settings(char *ssid, char *password);
esp_err_t set_wifi_settings(char *ssid, char *password);

#endif