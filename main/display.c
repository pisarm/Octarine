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
 
#include "display.h"
#include <u8g2.h>
#include "u8g2_esp32_hal.h"

// SDA - GPIO21
#define PIN_SDA 21

// SCL - GPIO22
#define PIN_SCL 22

static const int display_height = 64;
static const int display_width = 128;

static u8g2_t display;

display_state_t display_state;

char *display_time;

char *display_ip_addr;
char *display_mac_addr;

void display_init() {
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda = PIN_SDA;
	u8g2_esp32_hal.scl = PIN_SCL;
	
    u8g2_esp32_hal_init(u8g2_esp32_hal);
	u8g2_Setup_sh1106_128x64_noname_f(&display, U8G2_R0, u8g2_esp32_msg_i2c_cb, u8g2_esp32_msg_i2c_and_delay_cb);
	u8x8_SetI2CAddress(&display.u8x8, 0x78);
	u8g2_InitDisplay(&display); // display is in sleep mode after init
	u8g2_SetPowerSave(&display, 0); // wake up display

    u8g2_ClearDisplay(&display);
    u8g2_ClearBuffer(&display);

    display_state = DISPLAY_STATE_TIME;
}

void display_draw_center(char *string) {
    u8g2_ClearBuffer(&display);
    u8g2_SetFont(&display, u8g2_font_9x18B_tf);
    u8g2_SetFontPosCenter(&display);
    u8g2_uint_t width = u8g2_GetStrWidth(&display, string);

    u8g2_DrawStr(&display, (display_width-width)/2, display_height/2, string);
	u8g2_SendBuffer(&display);
}

void display_draw_line0_line1(char *line0, char *line1) {
    u8g2_ClearBuffer(&display);
    u8g2_SetFont(&display, u8g2_font_9x18B_tf);
    
    u8g2_uint_t line0_width = u8g2_GetStrWidth(&display, line0);
    u8g2_uint_t line1_width = u8g2_GetStrWidth(&display, line1);

    u8g2_SetFontPosBottom(&display);
    u8g2_DrawStr(&display, (display_width-line0_width)/2, (display_height/2), line0);

    u8g2_SetFontPosTop(&display);
    u8g2_DrawStr(&display, (display_width-line1_width)/2, (display_height/2), line1);
    
	u8g2_SendBuffer(&display);
}

// void display_draw_time(char *time) {
//     u8g2_ClearBuffer(&display);
//     u8g2_SetFont(&display, u8g2_font_freedoomr25_mn);
//     u8g2_SetFontPosCenter(&display);
//     u8g2_uint_t width = u8g2_GetStrWidth(&display, time);

//     u8g2_DrawStr(&display, (display_width-width)/2, display_height/2, time);
//     u8g2_SendBuffer(&display);
// }

void display_task(void *args) {
    for(;;) {
        switch (display_state) {
            case DISPLAY_STATE_TIME:
            if (display_time == NULL) {
                break;
            }

            u8g2_ClearBuffer(&display);
            u8g2_SetFont(&display, u8g2_font_freedoomr25_mn);
            u8g2_SetFontPosCenter(&display);
            u8g2_uint_t width = u8g2_GetStrWidth(&display, display_time);

            u8g2_DrawStr(&display, (display_width-width)/2, display_height/2, display_time);
            u8g2_SendBuffer(&display);
            break;

            case DISPLAY_STATE_STATUS:
            // if (display_ip_addr == NULL || display_mac_addr == NULL) {
            //     break;
            // }

            u8g2_ClearBuffer(&display);
            u8g2_SetFontPosBaseline(&display);

            u8g2_SetFont(&display, u8g2_font_6x10_mf);
            u8g2_DrawStr(&display, 0, 10, "IP");
            u8g2_DrawStr(&display, 0, 42, "MAC");

            u8g2_SetFont(&display, u8g2_font_freedoomr10_mu);
            u8g2_DrawStr(&display, 2, 28, display_ip_addr ?: "N/A");
            u8g2_DrawStr(&display, 2, 60, display_mac_addr ?: "N/A");

            u8g2_SendBuffer(&display);
            break;
        }

        vTaskDelay(33 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}