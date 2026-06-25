// ============================================================
// DisplayDriver.h — TFT Backlight PWM Control
// Provides setBrightness() API for auto-brightness integration
// with DayNightTheme ambient light sensor.
// ============================================================

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include "config_v2.h"

#ifdef ESP32
#include <esp32-hal-ledc.h>
#endif

// ============================================================
// DisplayDriver — Backlight PWM controller
// ============================================================
class DisplayDriver {
public:
    static DisplayDriver& getInstance();

    // Initialize backlight PWM on TFT_PIN_BL
    void begin();

    // Set brightness level (0-100)
    // 0 = off, 100 = maximum brightness
    void setBrightness(uint8_t level);

    // Get current brightness level (0-100)
    uint8_t getBrightness() const;

    // Enable/disable backlight (PWM on/off)
    void setBacklightEnabled(bool enabled);
    bool isBacklightEnabled() const;

    // Fade to target brightness over duration_ms
    void fadeTo(uint8_t targetLevel, uint32_t durationMs);

private:
    DisplayDriver();
    DisplayDriver(const DisplayDriver&) = delete;
    DisplayDriver& operator=(const DisplayDriver&) = delete;

    uint8_t _currentBrightness;
    uint8_t _targetBrightness;
    bool _enabled;

#ifdef ESP32
    // LEDC channel configuration for ESP32 PWM
    static const uint8_t LEDC_CHANNEL = 0;
    static const uint8_t LEDC_TIMER_BIT = 8;  // 8-bit resolution (0-255)
    static const uint32_t LEDC_FREQUENCY = 5000;  // 5 kHz PWM
    uint8_t _pwmMax;
#endif
};

#endif // DISPLAY_DRIVER_H
