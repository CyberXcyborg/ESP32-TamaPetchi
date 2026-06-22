// ============================================================
// TouchDriver.h — LVGL Touch Driver for XPT2046
// ============================================================

#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <Arduino.h>
#include "config_v2.h"

class TouchDriver {
public:
    static bool begin();
    static bool isReady() { return _ready; }
    
    // LVGL callback
    static void lvTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data);
    
private:
    static bool _ready;
    
    // Calibration data
    static int16_t _cal_x_min;
    static int16_t _cal_x_max;
    static int16_t _cal_y_min;
    static int16_t _cal_y_max;
};

#endif // TOUCH_DRIVER_H
