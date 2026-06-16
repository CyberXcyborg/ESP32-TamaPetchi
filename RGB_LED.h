#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// RGB LED Status Indicator (Phase 6.3)
// Green=healthy, Yellow=warning, Red=critical, Blue=sleeping
// ============================================================

// RGB LED GPIO pins (common cathode RGB LED)
#define RGB_LED_RED_PIN    14
#define RGB_LED_GREEN_PIN  12
#define RGB_LED_BLUE_PIN   13

void setupRGBLED();
void updateRGBLED(const Pet &pet);

#endif // RGB_LED_H
