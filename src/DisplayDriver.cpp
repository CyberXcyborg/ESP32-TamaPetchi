// ============================================================
// DisplayDriver.cpp — LVGL Display Driver Implementation
// Uses TFT_eSPI for ST7789 SPI display
// ============================================================

#include "DisplayDriver.h"
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <esp_heap_caps.h>

static TFT_eSPI tft = TFT_eSPI();

lv_disp_draw_buf_t DisplayDriver::_draw_buf;
lv_disp_drv_t DisplayDriver::_disp_drv;
lv_disp_t *DisplayDriver::_disp = NULL;
lv_color_t *DisplayDriver::_buf1 = NULL;
lv_color_t *DisplayDriver::_buf2 = NULL;
bool DisplayDriver::_ready = false;

bool DisplayDriver::begin() {
    if (_ready) return true;
    
    DEBUG_PRINTLN("[Display] Initializing ST7789...");
    
    // Initialize TFT
    tft.begin();
    tft.setRotation(0);  // Portrait
    tft.fillScreen(TFT_BLACK);
    
    // Initialize LVGL display buffer
    size_t buf_size = LVGL_BUFFER_SIZE;
    
    // Try PSRAM first, fall back to regular heap
    _buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!_buf1) {
        DEBUG_PRINTLN("[Display] PSRAM alloc failed, using regular heap");
        _buf1 = (lv_color_t *)malloc(buf_size * sizeof(lv_color_t));
    }
    
    _buf2 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!_buf2) {
        _buf2 = (lv_color_t *)malloc(buf_size * sizeof(lv_color_t));
    }
    
    if (!_buf1 || !_buf2) {
        DEBUG_PRINTLN("[Display] ERROR: Failed to allocate LVGL buffers!");
        return false;
    }
    
    DEBUG_PRINTF("[Display] Buffers: %d pixels each\n", buf_size);
    
    // Initialize LVGL display buffer
    lv_disp_draw_buf_init(&_draw_buf, _buf1, _buf2, buf_size);
    
    // Initialize LVGL display driver
    lv_disp_drv_init(&_disp_drv);
    _disp_drv.hor_res = TFT_WIDTH;
    _disp_drv.ver_res = TFT_HEIGHT;
    _disp_drv.flush_cb = lvFlushCb;
    _disp_drv.draw_buf = &_draw_buf;
    _disp_drv.full_refresh = 0;
    _disp_drv.sw_rotate = 0;
    
    _disp = lv_disp_drv_register(&_disp_drv);
    
    if (_disp) {
        _ready = true;
        DEBUG_PRINTLN("[Display] LVGL display initialized successfully");
    } else {
        DEBUG_PRINTLN("[Display] ERROR: LVGL display registration failed!");
    }
    
    return _ready;
}

void DisplayDriver::lvFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    
    lv_disp_flush_ready(drv);
}

void DisplayDriver::setBrightness(uint8_t level) {
    if (TFT_PIN_BL >= 0) {
        ledcSetup(0, 5000, 8);
        ledcAttachPin(TFT_PIN_BL, 0);
        ledcWrite(0, level);
    }
}

void DisplayDriver::backlightOn() {
    setBrightness(255);
}

void DisplayDriver::backlightOff() {
    if (TFT_PIN_BL >= 0) {
        ledcWrite(0, 0);
    }
}
