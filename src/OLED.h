#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// OLED Display (SSD1306 128x64)
// Wrapped in #ifdef ENABLE_OLED / #endif for optional compilation
// ============================================================

#ifdef ENABLE_OLED

void setupOLED();
void updateOLED(const Pet &pet);

// Phase 10.4: Factory reset display
void showFactoryResetOLED();

#endif // ENABLE_OLED

#endif // OLED_H
