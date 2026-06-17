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

// Deep sleep configuration
#define DEEP_SLEEP_PIN        0     // Wake from deep sleep on GPIO 0
#define DEEP_SLEEP_TIMEOUT    300000UL  // Auto deep-sleep after 5 min of no interaction (0 = disabled)

// RTC memory for deep sleep state preservation
RTC_DATA_ATTR static unsigned long rtcMagic;
RTC_DATA_ATTR static unsigned long rtcPetAge;
RTC_DATA_ATTR static int rtcPetHunger;
RTC_DATA_ATTR static int rtcPetHappiness;
RTC_DATA_ATTR static int rtcPetHealth;
RTC_DATA_ATTR static int rtcPetEnergy;
RTC_DATA_ATTR static int rtcPetCleanliness;
RTC_DATA_ATTR static bool rtcPetAlive;
RTC_DATA_ATTR static int rtcPetStage;
RTC_DATA_ATTR static int rtcPetType;
RTC_DATA_ATTR static int rtcPetWeather;
RTC_DATA_ATTR static bool rtcPetMusic;
RTC_DATA_ATTR static int rtcPetDifficulty;
#define RTC_MAGIC_VALUE 0xDEADBEEF

// Button actions
void setupButtons();
void checkButtons(Pet &pet);  // Call in loop() to poll buttons
void enterDeepSleep(Pet &pet);  // Save state and enter deep sleep
bool wasDeepSleepWake();       // Check if we woke from deep sleep
void restoreFromRTC(Pet &pet); // Restore pet state from RTC memory

#else

// Stubs when disabled
inline void setupButtons() {}
inline void checkButtons(Pet &) {}
inline void enterDeepSleep(Pet &) {}
inline bool wasDeepSleepWake() { return false; }
inline void restoreFromRTC(Pet &) {}

#endif // DISABLE_BUTTONS

#endif // BUTTONS_H
