#include "PowerManager.h"
#include "config.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

static unsigned long g_lastSleepCheck = 0;

// ============================================================
// Battery Monitoring
// ============================================================
void setupPowerManager() {
#ifdef BATTERY_ADC_PIN
  analogSetAttenuation(ADC_11db);
  pinMode(BATTERY_ADC_PIN, INPUT);
#endif
  enableSleepWakeup();
  Serial.println("[Power] Manager initialized");
}

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
// Deep Sleep
// ============================================================
void enableSleepWakeup() {
  esp_sleep_enable_ext0_wakeup((gpio_num_t)0, LOW);
  Serial.println("[Power] Wake-up enabled on GPIO0 (BOOT button)");
}

void enterDeepSleep() {
  Serial.println("[Power] Entering deep sleep...");
  Serial.flush();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_deep_sleep_start();
}

bool shouldSleep(const Pet &pet) {
  if (!pet.isAlive) return false;
  if (pet.state == "sleeping" && pet.isNight) return true;
  return false;
}

void handlePowerManager(Pet &pet, unsigned long currentMillis) {
  if (currentMillis - g_lastSleepCheck >= SLEEP_CHECK_INTERVAL) {
    g_lastSleepCheck = currentMillis;

    // Update battery level
    pet.batteryLevel = getBatteryPercent();
    if (pet.batteryLevel >= 0 && pet.batteryLevel <= LOW_BATTERY_THRESHOLD) {
      pet.lowBatteryWarning = true;
    } else if (pet.batteryLevel > LOW_BATTERY_THRESHOLD + 5) {
      pet.lowBatteryWarning = false;
    }

    // Check deep sleep
    if (shouldSleep(pet)) {
      enterDeepSleep();
    }
  }
}

// ============================================================
// WiFi Power Management
// ============================================================
void reduceWiFiPower() {
  esp_wifi_set_max_tx_power(40);
  WiFi.setSleep(true);
  Serial.println("[Power] WiFi power reduced");
}

void restoreWiFiPower() {
  esp_wifi_set_max_tx_power(80);
  WiFi.setSleep(false);
  Serial.println("[Power] WiFi power restored");
}

// ============================================================
// Battery JSON
// ============================================================
String getBatteryJson(const Pet &pet) {
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["batteryLevel"] = pet.batteryLevel;
  jsonDoc["lowBatteryWarning"] = pet.lowBatteryWarning;
  jsonDoc["hasBatteryMonitor"] = (pet.batteryLevel >= 0);
  String result;
  serializeJson(jsonDoc, result);
  return result;
}
