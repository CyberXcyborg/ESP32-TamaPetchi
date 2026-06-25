// ============================================================
// DisplayDriver.cpp — LVGL Display Driver Implementation (LVGL 9.x)
// Uses TFT_eSPI for ST7789 SPI display
// ============================================================

#include "DisplayDriver.h"
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>

static TFT_eSPI tft = TFT_eSPI();

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
    
    // LVGL 9.x display creation and configuration
    _disp = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    if (!_disp) {
        DEBUG_PRINTLN("[Display] ERROR: lv_display_create failed!");
        return false;
    }
    
    // Set display buffers (partial render mode)
    lv_display_set_buffers(_disp, _buf1, _buf2, buf_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // Set flush callback
    lv_display_set_flush_cb(_disp, lvFlushCb);
    
    _ready = true;
    DEBUG_PRINTLN("[Display] LVGL 9.x display initialized successfully");

    // Initialize backlight — run once to avoid reconfiguring PWM channel on every brightness change
    if (TFT_PIN_BL >= 0) {
        ledcSetup(0, 5000, 8);
        ledcAttachPin(TFT_PIN_BL, 0);
    }

    return _ready;
}

void DisplayDriver::lvFlushCb(lv_disp_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    
    lv_display_flush_ready(disp);
}

void DisplayDriver::setBrightness(uint8_t level) {
    // ledcSetup/ledcAttachPin called once in begin() — no need to reconfigure
    if (TFT_PIN_BL >= 0) {
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
