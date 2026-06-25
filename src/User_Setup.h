// ============================================================
// User_Setup.h — TFT_eSPI configuration for ESP32-S3 + ST7789
// Place in TFT_eSPI library folder or use -DUSER_SETUP_LOADED
// ============================================================

#ifndef USER_SETUP_H
#define USER_SETUP_H

// Driver
#define ST7789_DRIVER

// Display dimensions
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// ESP32-S3 Pin configuration
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   14
#define TFT_RST  13
#define TFT_BL   9

// SPI frequency
#define SPI_FREQUENCY  40000000

// Color order (try swapping if colors look wrong)
// #define TFT_RGB_ORDER TFT_RGB
// #define TFT_RGB_ORDER TFT_BGR

// Fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define SPI_FREQUENCY  40000000

// Touch (XPT2046)
#define TOUCH_CS 15
#define SPI_TOUCH_FREQUENCY  2500000

#endif // USER_SETUP_H
