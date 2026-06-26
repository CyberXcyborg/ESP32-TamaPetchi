#ifndef POWER_MANAGER_V2_H
#define POWER_MANAGER_V2_H

#include <Arduino.h>
#include "Pet_v2.h"
#include "config_v2.h"


// ESP32-specific headers (not available in native test builds)
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <driver/rtc_io.h>
#endif

// ============================================================
// Power Manager v2 — Phase 23
// Enhanced power management with:
// - Battery-aware display brightness
// - Charge state detection
// - Light sleep between frames (idle timeout)
// - Wake-on-button, wake-on-timer, wake-on-BLE
// - State preservation across light sleep (RTC memory)
// - Battery fuel gauge with oversampling
// ============================================================

// --- Sleep mode configuration ---
enum SleepModeV2 {
  SLEEP_NONE_V2 = 0,
  SLEEP_LIGHT_V2,       // Light sleep: <1mA, wake on WiFi/button/timer/BLE
  SLEEP_DEEP_V2         // Deep sleep: <10uA, wake on timer or GPIO only
};

// Wake interval presets (seconds)
enum WakeIntervalV2 {
  WAKE_1MIN_V2  = 60,
  WAKE_5MIN_V2  = 300,
  WAKE_15MIN_V2 = 900,
  WAKE_1HR_V2   = 3600
};

// --- Initialization ---
void setupPowerManagerV2();
void handlePowerManagerV2(PetV2::PetEngine &pet, uint32_t currentMillis);

// --- Battery (enhanced with oversampling) ---
int  getBatteryPercentV2();           // 0-100 with oversampling, or -1
bool isLowBatteryV2();
bool isCriticalBatteryV2();
bool isChargingV2();                  // Charge state via GPIO

// --- Battery fuel gauge ---
int  getEstimatedBatteryHoursV2();    // Estimated remaining hours, or -1
void updateBatteryHistoryV2(int percent);
void setBatteryCalibration(float minV, float maxV);

// --- Display brightness (battery-aware) ---
uint8_t getBrightnessForBattery();    // Returns 0-100 based on battery level
void applyBatteryBrightness();

// --- Sleep management ---
bool shouldSleepV2(const PetV2::PetEngine &pet, uint32_t idleTimeMs);
void enterLightSleepV2(uint32_t durationMs = 0);
void enterDeepSleepV2(uint32_t durationMs = 0);
void setSleepModeV2(SleepModeV2 mode);
SleepModeV2 getSleepModeV2();
void setWakeIntervalV2(WakeIntervalV2 interval);
WakeIntervalV2 getWakeIntervalV2();

// --- Wake source management ---
void enableWakeupSources();
bool wasDeepSleepWakeV2();
uint32_t getWakeupCause();

// --- State preservation (RTC memory) ---
void saveStateToRTC(const PetV2::PetEngine &pet);
bool restoreStateFromRTC(PetV2::PetEngine &pet);
void clearRTCState();

// --- Idle tracking ---
void markActivity();                  // Reset idle timer
uint32_t getIdleDurationMs();        // Time since last activity

// --- WiFi power management ---
void reduceWiFiPowerV2();
void restoreWiFiPowerV2();

// --- Watchdog ---
void setupWatchdog();
void feedWatchdog();
bool isWatchdogTriggered();

// --- JSON for web API ---
String getBatteryJsonV2(const PetV2::PetEngine &pet);
String getSleepModeJsonV2();
String getPowerStateJson(const PetV2::PetEngine &pet);

// --- Compatibility wrappers (Phase 13.5) ---
// Note: v1 Pet compatibility functions are in PowerManager_v2.cpp only
// to avoid circular include dependencies.
int  getBatteryPercent();
bool isLowBattery();
int  getEstimatedBatteryHours();
void updateBatteryHistory(int percent);
bool wasDeepSleepWake();
SleepModeV2 getSleepMode();
void setSleepMode(SleepModeV2 mode);
void setWakeInterval(WakeIntervalV2 interval);
WakeIntervalV2 getWakeInterval();
void setupPowerManager();

#endif // POWER_MANAGER_V2_H
