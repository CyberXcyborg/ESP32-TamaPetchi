// ============================================================
// DisplayDriver.h — LVGL Display Driver for ST7789 (LVGL 9.x)
// ============================================================

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include "config_v2.h"
#include <lvgl.h>

class DisplayDriver {
public:
    static bool begin();
    static void setBrightness(uint8_t level);
    static void backlightOn();
    static void backlightOff();
    
    // LVGL 9.x flush callback
    static void lvFlushCb(lv_disp_t *disp, const lv_area_t *area, uint8_t *px_map);
    
    // Getters
    static uint16_t getWidth() { return TFT_WIDTH; }
    static uint16_t getHeight() { return TFT_HEIGHT; }
    static lv_disp_t* getDisplay() { return _disp; }
    
private:
    static lv_disp_t *_disp;
    static lv_color_t *_buf1;
    static lv_color_t *_buf2;
    
    static bool _ready;
};

#endif // DISPLAY_DRIVER_H
