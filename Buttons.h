#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include "Pet.h"

// ============================================================
// Physical Button Support (Phase 6.3)
// Uses GPIO 0 (BOOT button) for feed/play/clean/sleep
// ============================================================

// Button GPIO pins
#define BUTTON_PIN        0     // GPIO 0 (BOOT button on most ESP32 dev boards)
#define BUTTON_LONG_PRESS_MS  2000  // 2 seconds = long press
#define BUTTON_DEBOUNCE_MS    50    // 50ms debounce

// Button actions
void setupButtons();
void checkButtons(Pet &pet);  // Call in loop() to poll buttons

#endif // BUTTONS_H
