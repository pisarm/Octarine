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

#include "button.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "display.h"

static QueueHandle_t buttonQueue;

// Interrupt handler

static void button_handler(void *args) {
	gpio_num_t gpio = BUTTON_GPIO;
	xQueueSendToBackFromISR(buttonQueue, &gpio, NULL);
}

// Functions

void button_init() {
	buttonQueue = xQueueCreate(10, sizeof(gpio_num_t));

	gpio_config_t gpioConfig;
	gpioConfig.pin_bit_mask = GPIO_SEL_16;
	gpioConfig.mode = GPIO_MODE_INPUT;
	gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
	gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
	gpioConfig.intr_type = GPIO_INTR_POSEDGE;
	gpio_config(&gpioConfig);

    gpio_install_isr_service(0);
	gpio_isr_handler_add(BUTTON_GPIO, button_handler, NULL);
}

void button_task(void *args) {
    gpio_num_t gpio;
	for(;;) {
		printf("Waiting on interrupt queue\n");
		BaseType_t rc = xQueueReceive(buttonQueue, &gpio, portMAX_DELAY);
		printf("Woke from interrupt queue wait: %d\n", rc);
		if (display_state == DISPLAY_STATE_TIME) {
			display_state = DISPLAY_STATE_STATUS;
		} else {
			display_state = DISPLAY_STATE_TIME;
		}
	}
}