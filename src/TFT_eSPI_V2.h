// ============================================================
// TFT_eSPI.h — Display driver wrapper for ST7789
// Compatible with ESP32-S3 and LVGL
// ============================================================

#ifndef TFT_ESPI_V2_H
#define TFT_ESPI_V2_H

#include <TFT_eSPI.h>
#include "config_v2.h"

class TFT_eSPI_V2 : public TFT_eSPI {
public:
    TFT_eSPI_V2() : TFT_eSPI() {}
    
    void init_display() {
        // Initialize ST7789 with ESP32-S3 pin mapping
        init();
        setRotation(0);
    }
    
    void set_backlight(uint8_t brightness) {
        if (TFT_PIN_BL >= 0) {
            ledcSetup(0, 5000, 8);  // Channel 0, 5kHz, 8-bit
            ledcAttachPin(TFT_PIN_BL, 0);
            ledcWrite(0, brightness);
        }
    }
};

#endif // TFT_ESPI_V2_H
