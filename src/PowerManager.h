#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "Pet.h"

// ESP32-specific headers (not available in native test builds)
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#include <esp_sleep.h>
#include <esp_system.h>
#endif

// ============================================================
// Power Management — Phase 13.5 Enhanced
// Deep sleep, light sleep, wake-on-timer, wake-on-gpio,
// battery estimation, and state preservation across sleep cycles.
// ============================================================

// --- Sleep mode configuration ---
enum SleepMode {
  SLEEP_NONE = 0,
  SLEEP_LIGHT,       // Light sleep: <1mA, wake on WiFi/button/timer
  SLEEP_DEEP         // Deep sleep: <10uA, wake on timer or GPIO only
};

// Wake-on-timer intervals (seconds)
enum WakeInterval {
  WAKE_5MIN  = 300,
  WAKE_15MIN = 900,
  WAKE_1HR   = 3600
};

// --- Initialization ---
void setupPowerManager();
void handlePowerManager(Pet &pet, unsigned long currentMillis);

// --- Battery ---
int  getBatteryPercent();          // 0-100, or -1 if not available
bool isLowBattery();
String getBatteryJson(const Pet &pet);

// --- Battery estimation (Phase 13.5) ---
int  getEstimatedBatteryHours();   // Estimated remaining hours, or -1
void updateBatteryHistory(int percent);  // Track for estimation

// enterDeepSleep() is defined in Buttons.cpp (Pet &pet version)
// PowerManager handles sleep decisions, Buttons.cpp handles the actual sleep
bool shouldSleep(const Pet &pet);
void enableSleepWakeup();

// --- Light sleep (Phase 13.5) ---
void enterLightSleep(unsigned long durationMs = 0);
void setSleepMode(SleepMode mode);
SleepMode getSleepMode();
void setWakeInterval(WakeInterval interval);
WakeInterval getWakeInterval();

// --- State preservation (Phase 13.5) ---
// Note: saveStateToRTC/restoreStateFromRTC are in Buttons.cpp
// PowerManager provides battery estimation and sleep mode config only
bool wasDeepSleepWake();              // Defined in Buttons.cpp

// --- WiFi power ---
void reduceWiFiPower();
void restoreWiFiPower();

// --- Sleep mode JSON for web API ---
String getSleepModeJson();

#endif // POWER_MANAGER_H
