// ============================================================
// DisplayDriver.cpp — TFT Backlight PWM Control Implementation
// ============================================================

#include "DisplayDriver.h"

#ifdef ESP32
#include <esp32-hal-ledc.h>
#endif

// ============================================================
// Singleton
// ============================================================
DisplayDriver& DisplayDriver::getInstance() {
    static DisplayDriver instance;
    return instance;
}

DisplayDriver::DisplayDriver()
    : _currentBrightness(100),
      _targetBrightness(100),
      _enabled(true)
#ifdef ESP32
      , _pwmMax(255)
#endif
{
}

// ============================================================
// Initialization
// ============================================================
void DisplayDriver::begin() {
#ifdef ESP32
    // Configure LEDC PWM channel for backlight
    ledcSetup(LEDC_CHANNEL, LEDC_FREQUENCY, LEDC_TIMER_BIT);
    ledcAttachPin(TFT_PIN_BL, LEDC_CHANNEL);
    setBrightness(_currentBrightness);
#else
    // Native/test stub: just track state
    _currentBrightness = 100;
    _enabled = true;
#endif
}

// ============================================================
// Brightness Control
// ============================================================
void DisplayDriver::setBrightness(uint8_t level) {
    _currentBrightness = constrain(level, 0, 100);

#ifdef ESP32
    // Map 0-100 to 0-pwmMax for PWM duty
    uint32_t duty = (uint32_t)_currentBrightness * _pwmMax / 100;
    ledcWrite(LEDC_CHANNEL, duty);
#endif
}

uint8_t DisplayDriver::getBrightness() const {
    return _currentBrightness;
}

// ============================================================
// Backlight Enable/Disable
// ============================================================
void DisplayDriver::setBacklightEnabled(bool enabled) {
    _enabled = enabled;
#ifdef ESP32
    if (!enabled) {
        ledcWrite(LEDC_CHANNEL, 0);
    } else {
        uint32_t duty = (uint32_t)_currentBrightness * _pwmMax / 100;
        ledcWrite(LEDC_CHANNEL, duty);
    }
#endif
}

bool DisplayDriver::isBacklightEnabled() const {
    return _enabled;
}

// ============================================================
// Fade Animation
// ============================================================
void DisplayDriver::fadeTo(uint8_t targetLevel, uint32_t durationMs) {
    _targetBrightness = constrain(targetLevel, 0, 100);

#ifdef ESP32
    // Simple fade: step through brightness levels
    int8_t step = (_targetBrightness > _currentBrightness) ? 1 : -1;
    uint8_t steps = abs((int)_targetBrightness - (int)_currentBrightness);
    if (steps == 0) return;

    uint32_t stepDelay = durationMs / steps;
    if (stepDelay < 10) stepDelay = 10;

    while (_currentBrightness != _targetBrightness) {
        if (_currentBrightness + step < 0 || _currentBrightness + step > 100) break;
        setBrightness(_currentBrightness + step);
        delay(stepDelay);
    }
#endif
}
