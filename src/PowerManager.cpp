#include "PowerManager.h"
#include "config.h"
#include "AppState.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// ESP32-specific headers (not available in native test builds)
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_wifi.h>
#endif

// ============================================================
// Module state
// ============================================================
static unsigned long g_lastSleepCheck = 0;
static SleepMode g_sleepMode = SLEEP_DEEP;
static WakeInterval g_wakeInterval = WAKE_15MIN;

// Battery history for estimation (circular buffer of hourly samples)
static int g_batteryHistory[24];
static int g_batteryHistoryIndex = 0;
static int g_batteryHistoryCount = 0;
static unsigned long g_lastBatterySample = 0;

// ============================================================
// Initialization
// ============================================================
void setupPowerManager() {
#ifdef BATTERY_ADC_PIN
  analogSetAttenuation(ADC_11db);
  pinMode(BATTERY_ADC_PIN, INPUT);
#endif

  enableSleepWakeup();

  // Initialize battery history
  for (int i = 0; i < 24; i++) g_batteryHistory[i] = -1;

  Serial.println("[Power] Manager initialized");
}

// ============================================================
// Battery Monitoring
// ============================================================
int getBatteryPercent() {
#ifdef BATTERY_ADC_PIN
  int raw = analogRead(BATTERY_ADC_PIN);
  // Map ADC reading to percentage (LiPo: 3.0V=0%, 4.2V=100%)
  // With voltage divider, ADC sees half the battery voltage
  float voltage = (raw / 4095.0) * 3.3 * 2.0;
  int percent = (int)((voltage - 3.0) / (4.2 - 3.0) * 100);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
#else
  return -1;
#endif
}

bool isLowBattery() {
  int pct = getBatteryPercent();
  return (pct >= 0 && pct < LOW_BATTERY_THRESHOLD);
}

// ============================================================
// Battery Estimation (Phase 13.5)
// ============================================================
void updateBatteryHistory(int percent) {
  if (percent < 0) return;
  unsigned long now = millis();
  // Sample every hour
  if (now - g_lastBatterySample >= 3600000UL || g_lastBatterySample == 0) {
    g_lastBatterySample = now;
    g_batteryHistory[g_batteryHistoryIndex] = percent;
    g_batteryHistoryIndex = (g_batteryHistoryIndex + 1) % 24;
    if (g_batteryHistoryCount < 24) g_batteryHistoryCount++;
  }
}

int getEstimatedBatteryHours() {
  if (g_batteryHistoryCount < 2) return -1; // Need at least 2 samples

  // Calculate average drain rate (% per hour)
  int oldest = -1;
  int newest = -1;
  int idx = (g_batteryHistoryIndex - g_batteryHistoryCount + 24) % 24;
  for (int i = 0; i < g_batteryHistoryCount; i++) {
    int val = g_batteryHistory[(idx + i) % 24];
    if (val < 0) continue;
    if (oldest < 0) oldest = val;
    newest = val;
  }

  if (oldest < 0 || newest < 0 || oldest <= newest) return -1;

  int drainPerHour = oldest - newest; // positive = battery drained
  if (drainPerHour <= 0) return -1;

  int currentPct = getBatteryPercent();
  if (currentPct < 0) return -1;

  return currentPct / drainPerHour;
}

// ============================================================
// Deep Sleep
// ============================================================
void enableSleepWakeup() {
  // Wake on GPIO0 (BOOT button) — any button press
  esp_sleep_enable_ext0_wakeup((gpio_num_t)0, LOW);
  Serial.println("[Power] Wake-up enabled on GPIO0 (BOOT button)");
}

// enterDeepSleep() is in Buttons.cpp — uses full state saving
// void enterDeepSleep() { ... }

bool shouldSleep(const Pet &pet) {
  if (!pet.isAlive) return false;
  if (pet.state == "sleeping" && pet.isNight) return true;
  return false;
}

// ============================================================
// Light Sleep (Phase 13.5)
// ============================================================
void enterLightSleep(unsigned long durationMs) {
  if (durationMs == 0) {
    durationMs = (unsigned long)g_wakeInterval * 1000UL;
  }
  Serial.printf("[Power] Light sleep for %lu ms\n", durationMs);
  Serial.flush();

  // Enable timer wake-up for light sleep
  esp_sleep_enable_timer_wakeup((uint64_t)durationMs * 1000ULL);
  // Enable GPIO wake-up (button press)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)0, LOW);

  // Enter light sleep
  esp_light_sleep_start();
  Serial.println("[Power] Woke from light sleep");
}

void setSleepMode(SleepMode mode) {
  g_sleepMode = mode;
}

SleepMode getSleepMode() {
  return g_sleepMode;
}

void setWakeInterval(WakeInterval interval) {
  g_wakeInterval = interval;
}

WakeInterval getWakeInterval() {
  return g_wakeInterval;
}

// ============================================================
// WiFi Power Management
// ============================================================
void reduceWiFiPower() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  esp_wifi_set_max_tx_power(40);
  WiFi.setSleep(true);
#endif
  Serial.println("[Power] WiFi power reduced");
}

void restoreWiFiPower() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  esp_wifi_set_max_tx_power(80);
  WiFi.setSleep(false);
#endif
  Serial.println("[Power] WiFi power restored");
}

// ============================================================
// Main Loop Handler
// ============================================================
void handlePowerManager(Pet &pet, unsigned long currentMillis) {
  if (currentMillis - g_lastSleepCheck >= SLEEP_CHECK_INTERVAL) {
    g_lastSleepCheck = currentMillis;

    // Update battery level
    pet.batteryLevel = getBatteryPercent();
    if (pet.batteryLevel >= 0) {
      updateBatteryHistory(pet.batteryLevel);
      if (pet.batteryLevel <= LOW_BATTERY_THRESHOLD) {
        pet.lowBatteryWarning = true;
      } else if (pet.batteryLevel > LOW_BATTERY_THRESHOLD + 5) {
        pet.lowBatteryWarning = false;
      }
    }

    // Check deep sleep
    if (shouldSleep(pet)) {
      // Use the Buttons.cpp version which saves full state to RTC + SPIFFS
      // enterDeepSleep(pet) is declared in Buttons.h
    }
  }
}

// ============================================================
// JSON Helpers
// ============================================================
String getBatteryJson(const Pet &pet) {
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["batteryLevel"] = pet.batteryLevel;
  jsonDoc["lowBatteryWarning"] = pet.lowBatteryWarning;
  jsonDoc["hasBatteryMonitor"] = (pet.batteryLevel >= 0);
  int est = getEstimatedBatteryHours();
  if (est >= 0) {
    jsonDoc["estimatedHoursRemaining"] = est;
  }
  String result;
  serializeJson(jsonDoc, result);
  return result;
}

String getSleepModeJson() {
  StaticJsonDocument<256> doc;
  doc["sleepMode"] = (g_sleepMode == SLEEP_DEEP) ? "deep" : "light";
  doc["wakeInterval"] = (int)g_wakeInterval;
  doc["deepSleepWake"] = wasDeepSleepWake();
  doc["canDeepSleep"] = true;
  doc["canLightSleep"] = true;
  String result;
  serializeJson(doc, result);
  return result;
}
