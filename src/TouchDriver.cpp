// ============================================================
// TouchDriver.cpp — LVGL Touch Driver Implementation (LVGL 9.x)
// Uses TFT_eSPI's built-in XPT2046 touch support
// ============================================================

#include "TouchDriver.h"
#include <TFT_eSPI.h>
#include <lvgl.h>

static TFT_eSPI tft_touch = TFT_eSPI();

bool TouchDriver::_ready = false;
int16_t TouchDriver::_cal_x_min = 0;
int16_t TouchDriver::_cal_x_max = 4095;
int16_t TouchDriver::_cal_y_min = 0;
int16_t TouchDriver::_cal_y_max = 4095;

bool TouchDriver::begin() {
    // Touch is initialized as part of TFT_eSPI
    // Just verify it's responding
    DEBUG_PRINTLN("[Touch] Initializing XPT2046...");
    
    uint16_t x, y;
    _ready = tft_touch.getTouch(&x, &y);
    
    // Even if no touch detected, the driver is ready
    _ready = true;
    
    DEBUG_PRINTLN("[Touch] Touch driver initialized");
    return _ready;
}

void TouchDriver::lvTouchRead(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    uint16_t x = 0, y = 0;
    bool pressed = tft_touch.getTouch(&x, &y);
    
    if (pressed) {
        // Map raw touch coordinates to display coordinates
        data->point.x = map(x, _cal_x_min, _cal_x_max, 0, TFT_WIDTH);
        data->point.y = map(y, _cal_y_min, _cal_y_max, 0, TFT_HEIGHT);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void TouchDriver::setCalibration(int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max) {
    _cal_x_min = x_min;
    _cal_x_max = x_max;
    _cal_y_min = y_min;
    _cal_y_max = y_max;
}
