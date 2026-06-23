// ============================================================
// TouchDriver.h — LVGL Touch Driver for XPT2046 (LVGL 9.x)
// ============================================================

#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <Arduino.h>
#include "config_v2.h"
#include <lvgl.h>

class TouchDriver {
public:
    static bool begin();
    
    // LVGL 9.x input device read callback
    static void lvTouchRead(lv_indev_t *indev, lv_indev_data_t *data);
    
    static void setCalibration(int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max);
    static bool isReady() { return _ready; }

private:
    static bool _ready;
    static int16_t _cal_x_min;
    static int16_t _cal_x_max;
    static int16_t _cal_y_min;
    static int16_t _cal_y_max;
};

#endif // TOUCH_DRIVER_H
