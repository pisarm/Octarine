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

#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"

#define STORAGE_TIMEZONE_MAX 32
#define STORAGE_WIFI_SSID_MAX 32
#define STORAGE_WIFI_PASSWORD_MAX 64
#define STORAGE_TRANSIT_DESTINATION_MAX 64
#define STORAGE_TRANSIT_ORIGIN_MAX 64

esp_err_t load_timezone(char *timezone);
esp_err_t store_timezone(char *timezone);

esp_err_t load_transit_destination(char *destination);
esp_err_t store_transit_destination(char *destination);

esp_err_t load_transit_origin(char *origin);
esp_err_t store_transit_origin(char *origin);

esp_err_t load_wifi_credentials(char *ssid, char *password);
esp_err_t store_wifi_credentials(char *ssid, char *password);

#endif
