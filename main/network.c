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

#include "network.h"

#include <esp_log.h>
#include <mbedtls/platform.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <string.h>

static char tag[] = "network";

// static network_request_callback callback;
static network_config_t config;
 
// static network_response_data_t *response_data;

network_response_data_t* network_perform_request(network_config_t network_config) {
// void network_perform_request(network_config_t network_config, network_response_data_t **network_response_data, network_request_callback network_request_callback) {
    config = network_config;
    // callback = network_request_callback;

    network_response_data_t *network_response_data = malloc(sizeof(network_response_data_t));
    network_response_data->raw_data = malloc(1024 * sizeof(char));

    char buf[256];
    int ret, len;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    ESP_LOGI(tag, "Seeding the random number generator");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
        ESP_LOGE(tag, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }

    ESP_LOGI(tag, "Setting hostname for TLS session...");

    if((ret = mbedtls_ssl_set_hostname(&ssl, config.hostname)) != 0) {
        ESP_LOGE(tag, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }

        ESP_LOGI(tag, "Setting up the SSL/TLS structure...");

    if((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        ESP_LOGE(tag, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
       a warning if CA verification fails but it will continue to connect.
       You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        ESP_LOGE(tag, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

    mbedtls_net_init(&server_fd);

    ESP_LOGI(tag, "Connecting to %s:%s...", config.hostname, config.host_port);

    if ((ret = mbedtls_net_connect(&server_fd, config.hostname, config.host_port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        ESP_LOGE(tag, "mbedtls_net_connect returned -%x", -ret);
        goto exit;
    }

    ESP_LOGI(tag, "Connected.");

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    ESP_LOGI(tag, "Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(tag, "mbedtls_ssl_handshake returned -0x%x", -ret);
            goto exit;
        }
    }

    ESP_LOGI(tag, "Writing HTTP request...");
    
    while((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)config.request, strlen(config.request))) <= 0) {
        if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(tag, "mbedtls_ssl_write returned -0x%x", -ret);
            goto exit;
        }
    }

    len = ret;
    ESP_LOGI(tag, "%d bytes written", len);
    ESP_LOGI(tag, "Reading HTTP response...");

    for(;;) {
        len = sizeof(buf) - 1;
        bzero(buf, sizeof(buf));
        ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

        if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue;
        }
            
        if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            ret = 0;
            break;
        }

        if(ret < 0) {
            ESP_LOGE(tag, "mbedtls_ssl_read returned -0x%x", -ret);
            break;
        }

        if(ret == 0) {
            ESP_LOGI(tag, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGI(tag, "%d bytes read", len);

            // strcat(*(network_response_data->raw_data), buf);
            // Concatenate raw data into response
            strcat(network_response_data->raw_data, buf);        
    }

    mbedtls_ssl_close_notify(&ssl);

exit:
    mbedtls_ssl_session_reset(&ssl);
    mbedtls_net_free(&server_fd);

    if(ret != 0) {
        mbedtls_strerror(ret, buf, 100);
        ESP_LOGE(tag, "Last error was: -0x%x - %s", -ret, buf);
    }

    return network_response_data;
}
