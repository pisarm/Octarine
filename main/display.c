#include <u8g2.h>
#include "u8g2_esp32_hal.h"

// SDA - GPIO21
#define PIN_SDA 21

// SCL - GPIO22
#define PIN_SCL 22

static const int display_height = 64;
static const int display_width = 128;
static const char *splash_title = "Octarine";

static u8g2_t display;

void display_init() {
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda = PIN_SDA;
	u8g2_esp32_hal.scl = PIN_SCL;
	
    u8g2_esp32_hal_init(u8g2_esp32_hal);
	u8g2_Setup_sh1106_128x64_noname_f(&display, U8G2_R0, u8g2_esp32_msg_i2c_cb, u8g2_esp32_msg_i2c_and_delay_cb);
	u8x8_SetI2CAddress(&display.u8x8, 0x78);
	u8g2_InitDisplay(&display); // send init sequence to the display, display is in sleep mode after this,
	u8g2_SetPowerSave(&display, 0); // wake up display

    u8g2_ClearDisplay(&display);
    u8g2_ClearBuffer(&display);
}

void display_draw_center(char *string) {
    u8g2_ClearBuffer(&display);
    u8g2_SetFont(&display, u8g2_font_9x18B_tf);
    u8g2_SetFontPosCenter(&display);
    u8g2_uint_t width = u8g2_GetStrWidth(&display, string);

    u8g2_DrawStr(&display, (display_width-width)/2, display_height/2, string);
	u8g2_SendBuffer(&display);
}
