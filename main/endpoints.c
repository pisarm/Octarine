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

#include "endpoints.h"

#include "esp_system.h"
#include "storage.h"

#define CONTENT_TYPE_HTML "Content-Type: text/html"

void root_endpoint(struct mg_connection *connection, int event, void *event_data) {
    char page[] = "<html><body><h1>Octarine</h1></body></html>";
    int pageLength = strlen(page);

    mg_send_head(connection, 200, pageLength, CONTENT_TYPE_HTML);
    mg_printf(connection, "%s", page);
    connection->flags |= MG_F_SEND_AND_CLOSE;
}

void config_endpoint(struct mg_connection *connection, int event, void *event_data) {
    char page[] = "<html><body><h1>Octarine</h1><p>Welcome</p><p><a href='config/time'>Time</a></p><p><a href='config/wifi'>Wifi</a></p></body></html>";
    int pageLength = strlen(page);

    mg_send_head(connection, 200, pageLength, CONTENT_TYPE_HTML);
    mg_printf(connection, "%s", page);
    connection->flags |= MG_F_SEND_AND_CLOSE;
}
// // store_timezone("CET-1CEST,M3.5.0/2,M10.5.0/3");
void config_time_endpoint(struct mg_connection *connection, int event, void *event_data) {
    struct http_message *hm = (struct http_message *)event_data;

    if (mg_vcmp(&hm->method, "GET") == 0) {
        char page[] = "<html><body><h1>Octarine</h1><p>Enter default timezone.</p><p>CET-1CEST,M3.5.0/2,M10.5.0/3 is Europe/Copenhagen</p><br><form action=\"/config/time\" method=\"post\">timezone<br><input type=\"text\" name=\"timezone\" value=\"\"><br><input type=\"submit\" value=\"Set\"></form></body></html>";
        int pageLength = strlen(page);

        mg_send_head(connection, 200, pageLength, CONTENT_TYPE_HTML);
        mg_printf(connection, "%s", page);
        connection->flags |= MG_F_SEND_AND_CLOSE;
    } else if (mg_vcmp(&hm->method, "POST") == 0) {
        char page[] = "<html><body><h1>Octarine</h1><p>Time zone has been stored.</p></body></html>";
        int pageLength = strlen(page);

        char timezone[TIMEZONE_MAX];
        mg_get_http_var(&hm->body, "timezone", timezone, sizeof(timezone));

        if (store_timezone(timezone) != ESP_OK) {
            printf("error storing timezone\n");
        }

        mg_send_head(connection, 200, pageLength, CONTENT_TYPE_HTML);
        mg_printf(connection, "%s", page);
        connection->flags |= MG_F_SEND_AND_CLOSE;
    }
}

void config_wifi_endpoint(struct mg_connection *connection, int event, void *event_data) {
    struct http_message *hm = (struct http_message *)event_data;    

    if (mg_vcmp(&hm->method, "GET") == 0) {
        char page[] = "<html><body><h1>Octarine</h1><p>Enter the requisite WIFI credentials below.</p><br><form action=\"/config/wifi\" method=\"post\">SSID<br><input type=\"text\" name=\"ssid\" value=\"\"><br>Password<br><input type=\"text\" name=\"password\" value=\"\"><br><input type=\"submit\" value=\"Set\"></form></body></html>";
        int pageLength = strlen(page);

        mg_send_head(connection, 200, pageLength, CONTENT_TYPE_HTML);
        mg_printf(connection, "%s", page);
        connection->flags |= MG_F_SEND_AND_CLOSE;
      } else if (mg_vcmp(&hm->method, "POST") == 0) {
        char page[] = "<html><body><h1>Octarine</h1><p>Wifi credentials have been stored.</p><p>Your Octarine device will now reset.</p></body></html>";
        int pageLength = strlen(page);

        char ssid[SSID_MAX];
        char password[PASSWORD_MAX];
        mg_get_http_var(&hm->body, "ssid", ssid, sizeof(ssid));
        mg_get_http_var(&hm->body, "password", password, sizeof(password));

        //TODO: validation of entered values, send error message to user (not 200 OK)
        store_wifi_credentials(ssid, password);


        mg_send_head(connection, 200, pageLength, CONTENT_TYPE_HTML);
        mg_printf(connection, "%s", page);
        connection->flags |= MG_F_SEND_AND_CLOSE;

        esp_restart();
      }
}

// void config_root_endpoint(struct mg_connection *nc, int ev, void *ev_data) {
//     char page[] = "<html><body><h1>Octarine</h1><p>Welcome to your new and as yet unconfigured octarine unit.</p><p>Enter the requisite WIFI credentials below.</p><br><form action=\"/wifi\" method=\"post\">SSID<br><input type=\"text\" name=\"ssid\" value=\"\"><br>Password<br><input type=\"text\" name=\"password\" value=\"\"><br><input type=\"submit\" value=\"Set\"></form></body></html>";
//     int pageLength = strlen(page);

//     mg_send_head(nc, 200, pageLength, CONTENT_TYPE_HTML);
//     mg_printf(nc, "%s", page);
//     nc->flags |= MG_F_SEND_AND_CLOSE;
// }

// void get_time_endpoint(struct mg_connection *nc, int ev, void *ev_data) {
//     char page[256];
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     sprintf(page, "<html><body><h1>%d.%d secs</h1></body></html>", (int)tv.tv_sec, (int)tv.tv_usec);
//     int pageLength = strlen(page);
//     mg_send_head(nc, 200, pageLength, CONTENT_TYPE_HTML);
//     mg_printf(nc, "%s", page);
//     nc->flags |= MG_F_SEND_AND_CLOSE;
// }

/*
// printf("%.*s", hm->method.p);
//  printf("%p: %.*s %.*s\r\n", connection, (int) hm->method.len, hm->method.p, (int) hm->uri.len, hm->uri.p);
*/