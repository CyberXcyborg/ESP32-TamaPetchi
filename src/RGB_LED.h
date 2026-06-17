#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// RGB LED Status Indicator (Phase 6.3)
// Green=healthy, Yellow=warning, Red=critical, Blue=sleeping
// Compile-time flag: DISABLE_RGB_LED to remove
// ============================================================

#ifndef DISABLE_RGB_LED

// RGB LED GPIO pins (common cathode RGB LED)
#define RGB_LED_RED_PIN    14
#define RGB_LED_GREEN_PIN  12
#define RGB_LED_BLUE_PIN   13

void setupRGBLED();
void updateRGBLED(const Pet &pet);

#else

// Stubs when disabled
inline void setupRGBLED() {}
inline void updateRGBLED(const Pet &) {}

#endif // DISABLE_RGB_LED

#endif // RGB_LED_H
