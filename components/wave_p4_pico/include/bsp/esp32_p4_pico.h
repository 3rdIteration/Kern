#pragma once

#include "bsp/config.h"
#include "bsp/display.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "sdkconfig.h"

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
#include "esp_lv_adapter.h"
#include "lvgl.h"
#endif

/**************************************************************************************************
 *  BSP Capabilities
 **************************************************************************************************/

#define BSP_CAPS_DISPLAY 1
#define BSP_CAPS_TOUCH 1
#define BSP_CAPS_BUTTONS 0
#define BSP_CAPS_AUDIO 0
#define BSP_CAPS_AUDIO_SPEAKER 0
#define BSP_CAPS_AUDIO_MIC 0
#define BSP_CAPS_SDCARD 0
#define BSP_CAPS_IMU 0

/**************************************************************************************************
 *  Pinout
 *
 *  Default GPIO assignments for the ESP32-P4-Pico 40-pin Pico-compatible
 *  header wired to an ST7789 320x240 SPI display and a GT911 touch panel.
 *  Adjust these to match your actual wiring before building.
 *
 *  SPI display (ST7789):
 *    MOSI  -> GPIO13   CLK -> GPIO14   CS  -> GPIO15
 *    DC    -> GPIO16   RST -> GPIO17
 *  Backlight (active-high PWM):
 *    BL    -> GPIO10
 *  I2C touch (GT911):
 *    SDA   -> GPIO4    SCL -> GPIO5
 *    RST   -> GPIO6    INT -> GPIO7
 *
 **************************************************************************************************/

/* SPI LCD */
#define BSP_LCD_SPI_MOSI (GPIO_NUM_13)
#define BSP_LCD_SPI_CLK  (GPIO_NUM_14)
#define BSP_LCD_SPI_CS   (GPIO_NUM_15)
#define BSP_LCD_DC       (GPIO_NUM_16)
#define BSP_LCD_RST      (GPIO_NUM_17)
#define BSP_LCD_BACKLIGHT (GPIO_NUM_10)

/* I2C */
#define BSP_I2C_SDA (GPIO_NUM_4)
#define BSP_I2C_SCL (GPIO_NUM_5)

/* Touch (GT911) */
#define BSP_LCD_TOUCH_RST (GPIO_NUM_6)
#define BSP_LCD_TOUCH_INT (GPIO_NUM_7)

/* No camera motor on this board */
#define BSP_CAM_HAS_MOTOR 0

/**************************************************************************************************
 *
 * I2C interface
 *
 **************************************************************************************************/
#define BSP_I2C_NUM CONFIG_BSP_I2C_NUM

esp_err_t bsp_i2c_init(void);
esp_err_t bsp_i2c_deinit(void);
i2c_master_bus_handle_t bsp_i2c_get_handle(void);

/**************************************************************************************************
 *
 * LCD interface
 *
 **************************************************************************************************/

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)

lv_display_t *bsp_display_start(void);

/**
 * @brief Take LVGL mutex
 *
 * @param[in] timeout_ms Timeout in [ms]. 0 will block indefinitely.
 * @return true  Mutex was taken
 * @return false Mutex was NOT taken
 */
bool bsp_display_lock(uint32_t timeout_ms);

/**
 * @brief Give LVGL mutex
 */
void bsp_display_unlock(void);

#endif
