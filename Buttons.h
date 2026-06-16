#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Physical Button Support (Phase 6.3)
// Uses GPIO 0 (BOOT button) for feed/play/clean/sleep
// Compile-time flag: DISABLE_BUTTONS to remove
// ============================================================

#ifndef DISABLE_BUTTONS

// Button GPIO pins
#define BUTTON_PIN        0     // GPIO 0 (BOOT button on most ESP32 dev boards)
#define BUTTON_LONG_PRESS_MS  2000  // 2 seconds = long press
#define BUTTON_DEBOUNCE_MS    50    // 50ms debounce

// Button actions
void setupButtons();
void checkButtons(Pet &pet);  // Call in loop() to poll buttons

#else

// Stubs when disabled
inline void setupButtons() {}
inline void checkButtons(Pet &) {}

#endif // DISABLE_BUTTONS

#endif // BUTTONS_H
