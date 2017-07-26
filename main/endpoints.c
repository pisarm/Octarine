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
#include "html.h"
#include "storage.h"

#define CONTENT_TYPE_HTML "Content-Type: text/html"

inline int length_for_content(const char *content) {
    return strlen(content) + strlen(html_template) - HTML_TEMPLATE_FORMATTER_COUNT;
}

void root_endpoint(struct mg_connection *connection, int event, void *event_data) {
    // int html_length = strlen(html_content_root) + strlen(html_template) - HTML_TEMPLATE_FORMATTER_COUNT;

    mg_send_head(connection, 200, length_for_content(html_content_root), CONTENT_TYPE_HTML);
    mg_printf(connection, html_template, html_content_root);
    connection->flags |= MG_F_SEND_AND_CLOSE;
}

void config_time_endpoint(struct mg_connection *connection, int event, void *event_data) {
    struct http_message *hm = (struct http_message *)event_data;

    if (mg_vcmp(&hm->method, "GET") == 0) {
        int html_length = strlen(html_content_config_time) + strlen(html_template) - HTML_TEMPLATE_FORMATTER_COUNT;

        mg_send_head(connection, 200, html_length, CONTENT_TYPE_HTML);
        mg_printf(connection, html_template, html_content_config_time);
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
        mg_send_head(connection, 200, length_for_content(html_content_config_wifi), CONTENT_TYPE_HTML);
        mg_printf(connection, html_template, html_content_config_wifi);
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

void about_endpoint(struct mg_connection *connection, int event, void *event_data) {
    char *content = malloc(512);    //FIXME: this is a guess - calculate this to avoid surprises
    sprintf(content, html_content_about, esp_get_idf_version(), esp_get_free_heap_size());

    mg_send_head(connection, 200, length_for_content(content), CONTENT_TYPE_HTML);
    mg_printf(connection, html_template, content);
    connection->flags |= MG_F_SEND_AND_CLOSE;
    
    free(content);
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