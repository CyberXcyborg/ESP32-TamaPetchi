#include "PowerManager_v2.h"
#include "Pet.h"
#include <ArduinoJson.h>


#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#include <WiFi.h>
#endif

// ============================================================
// Module state
// ============================================================
static SleepModeV2 g_sleepModeV2 = SLEEP_LIGHT_V2;
static WakeIntervalV2 g_wakeIntervalV2 = WAKE_5MIN_V2;
static uint32_t g_lastActivityMs = 0;
static uint32_t g_lastSleepCheck = 0;
static bool g_watchdogTriggered = false;
static uint32_t g_lastWatchdogFeed = 0;

// Battery history for estimation (circular buffer of hourly samples)
static int g_batteryHistory[24];
static int g_batteryHistoryIndex = 0;
static int g_batteryHistoryCount = 0;
static uint32_t g_lastBatterySample = 0;

// ADC calibration
static float g_batteryMinV = BATTERY_VOLTAGE_MIN;
static float g_batteryMaxV = BATTERY_VOLTAGE_MAX;

// ============================================================
// Battery Monitoring (Enhanced with oversampling)
// ============================================================
int getBatteryPercentV2() {
#ifdef BATTERY_ADC_PIN
  // Oversample ADC for noise reduction
  long sum = 0;
  for (int i = 0; i < BATTERY_FILTER_SAMPLES; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
  }
  int raw = sum / BATTERY_FILTER_SAMPLES;

  // Map ADC reading to voltage (with voltage divider factor 2.0)
  float voltage = (raw / 4095.0f) * 3.3f * 2.0f;

  // Clamp to configured range
  if (voltage < g_batteryMinV) voltage = g_batteryMinV;
  if (voltage > g_batteryMaxV) voltage = g_batteryMaxV;

  int percent = (int)((voltage - g_batteryMinV) / (g_batteryMaxV - g_batteryMinV) * 100.0f);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
#else
  return -1;
#endif
}

bool isLowBatteryV2() {
  int pct = getBatteryPercentV2();
  return (pct >= 0 && pct < LOW_BATTERY_THRESHOLD);
}

bool isCriticalBatteryV2() {
  int pct = getBatteryPercentV2();
  return (pct >= 0 && pct < CRITICAL_BATTERY_THRESHOLD);
}

bool isChargingV2() {
#ifdef CHARGE_DETECT_PIN
  if (CHARGE_DETECT_PIN < 0) return false;
  // TP4056: LOW when charging, HIGH when full/not charging
  return (digitalRead(CHARGE_DETECT_PIN) == LOW);
#else
  return false;
#endif
}

// ============================================================
// Battery Fuel Gauge
// ============================================================
void setBatteryCalibration(float minV, float maxV) {
  if (minV > 0 && maxV > minV) {
    g_batteryMinV = minV;
    g_batteryMaxV = maxV;
  }
}

void updateBatteryHistoryV2(int percent) {
  if (percent < 0) return;
  uint32_t now = millis();
  // Sample every hour
  if (now - g_lastBatterySample >= 3600000UL || g_lastBatterySample == 0) {
    g_lastBatterySample = now;
    g_batteryHistory[g_batteryHistoryIndex] = percent;
    g_batteryHistoryIndex = (g_batteryHistoryIndex + 1) % 24;
    if (g_batteryHistoryCount < 24) g_batteryHistoryCount++;
  }
}

int getEstimatedBatteryHoursV2() {
  if (g_batteryHistoryCount < 2) return -1;

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

  int drainPerHour = oldest - newest;
  if (drainPerHour <= 0) return -1;

  int currentPct = getBatteryPercentV2();
  if (currentPct < 0) return -1;

  // Use float division for accurate estimate (avoids integer truncation)
  return (int)((float)currentPct / (float)drainPerHour);
}

// ============================================================
// Display Brightness (Battery-Aware)
// ============================================================
uint8_t getBrightnessForBattery() {
  int pct = getBatteryPercentV2();
  if (pct < 0) return 100;  // No battery monitor = full brightness

  if (pct < CRITICAL_BATTERY_THRESHOLD) {
    return DISPLAY_OFF_BATTERY;  // Near-off
  } else if (pct < LOW_BATTERY_THRESHOLD) {
    // Linear interpolation between OFF_BATTERY and DIM_BATTERY
    float ratio = (float)(pct - CRITICAL_BATTERY_THRESHOLD) /
                  (float)(LOW_BATTERY_THRESHOLD - CRITICAL_BATTERY_THRESHOLD);
    return (uint8_t)(DISPLAY_OFF_BATTERY +
                     ratio * (DISPLAY_DIM_BATTERY - DISPLAY_OFF_BATTERY));
  } else if (pct < 50) {
    // Linear interpolation between DIM_BATTERY and 100
    float ratio = (float)(pct - LOW_BATTERY_THRESHOLD) / (50 - LOW_BATTERY_THRESHOLD);
    return (uint8_t)(DISPLAY_DIM_BATTERY + ratio * (100 - DISPLAY_DIM_BATTERY));
  }
  return 100;
}

void applyBatteryBrightness() {
  uint8_t bright = getBrightnessForBattery();
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#ifdef TFT_PIN_BL
  if (TFT_PIN_BL >= 0) {
    // PWM brightness on backlight pin
    analogWrite(TFT_PIN_BL, (bright * 255) / 100);
  }
#endif
#endif
}

// ============================================================
// Sleep Management
// ============================================================
bool shouldSleepV2(const PetEngine &pet, uint32_t idleTimeMs) {
  if (!pet.isAlive()) return false;
  if (isChargingV2()) return false;  // Don't sleep while charging
  if (idleTimeMs < IDLE_TIMEOUT_MS) return false;
  return true;
}

void setSleepModeV2(SleepModeV2 mode) {
  g_sleepModeV2 = mode;
}

SleepModeV2 getSleepModeV2() {
  return g_sleepModeV2;
}

void setWakeIntervalV2(WakeIntervalV2 interval) {
  g_wakeIntervalV2 = interval;
}

WakeIntervalV2 getWakeIntervalV2() {
  return g_wakeIntervalV2;
}

void enableWakeupSources() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  // Wake on timer
  uint32_t intervalSec = (uint32_t)g_wakeIntervalV2;
  esp_sleep_enable_timer_wakeup((uint64_t)intervalSec * 1000000ULL);
  // Wake on BOOT button (GPIO0)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)0, LOW);
#endif
}

// Compatibility wrapper for old Pet struct
void handlePowerManager(Pet &pet, unsigned long currentMillis) {
  // Convert v1 Pet fields to v2 pet for battery monitoring
  int battPct = getBatteryPercentV2();
  (void)currentMillis;
  pet.batteryLevel = battPct;
  if (battPct >= 0) {
    updateBatteryHistoryV2(battPct);
    if (battPct <= LOW_BATTERY_THRESHOLD) {
      pet.lowBatteryWarning = true;
    } else if (battPct > LOW_BATTERY_THRESHOLD + 5) {
      pet.lowBatteryWarning = false;
    }
  }
}

uint32_t getWakeupCause() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  return (uint32_t)esp_sleep_get_wakeup_cause();
#else
  return 0;
#endif
}

bool wasDeepSleepWakeV2() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  return (cause == ESP_SLEEP_WAKEUP_TIMER ||
          cause == ESP_SLEEP_WAKEUP_EXT0 ||
          cause == ESP_SLEEP_WAKEUP_EXT1);
#else
  return false;
#endif
}

void enterLightSleepV2(uint32_t durationMs) {
  if (durationMs == 0) {
    durationMs = (uint32_t)g_wakeIntervalV2 * 1000UL;
  }
  if (durationMs > MAX_SLEEP_DURATION_MS) {
    durationMs = MAX_SLEEP_DURATION_MS;
  }

#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  // Save activity timestamp to RTC before sleep
  // Configure wake-up sources
  esp_sleep_enable_timer_wakeup((uint64_t)durationMs * 1000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)0, LOW);

  // Reduce WiFi power before sleep
  reduceWiFiPowerV2();

  // Enter light sleep
  esp_light_sleep_start();

  // Restore after wake
  restoreWiFiPowerV2();
  markActivity();  // Reset idle timer after wake
  applyBatteryBrightness();
#endif
}

void enterDeepSleepV2(uint32_t durationMs) {
  if (durationMs == 0) {
    durationMs = (uint32_t)g_wakeIntervalV2 * 1000UL;
  }
  if (durationMs > MAX_SLEEP_DURATION_MS) {
    durationMs = MAX_SLEEP_DURATION_MS;
  }

#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  // Save state to RTC before deep sleep
  // Note: Full state save handled externally by caller
  esp_sleep_enable_timer_wakeup((uint64_t)durationMs * 1000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)0, LOW);
  esp_deep_sleep_start();
#endif
}

// ============================================================
// Idle Tracking
// ============================================================
void markActivity() {
  g_lastActivityMs = millis();
}

uint32_t getIdleDurationMs() {
  return millis() - g_lastActivityMs;
}

// ============================================================
// State Preservation in RTC Memory
// ============================================================
// Layout: Pet vital state cached in RTC slow memory
// This survives light sleep and soft reset, cleared on power cycle
typedef struct {
  uint32_t magic;
  uint8_t  hunger;
  uint8_t  happiness;
  uint8_t  energy;
  uint8_t  cleanliness;
  uint8_t  health;
  uint8_t  stage;
  uint8_t  state;
  uint32_t age_minutes;
  uint32_t birth_timestamp;
  uint32_t checksum;
} RTCPetState;

#define RTC_PET_MAGIC 0x54414D41  // "TAMA"

static RTCPetState __attribute__((section(".noinit"))) g_rtcPetState;

static uint32_t calcPetStateChecksum(const RTCPetState &s) {
  uint32_t cs = s.magic ^ s.hunger ^ s.happiness ^ s.energy ^
                s.cleanliness ^ s.health ^ s.stage ^ s.state;
  cs ^= (uint32_t)(s.age_minutes ^ s.birth_timestamp);
  return ~cs;
}

static bool validateRTCPetState(const RTCPetState &s) {
  if (s.magic != RTC_PET_MAGIC) return false;
  return (calcPetStateChecksum(s) == s.checksum);
}

void saveStateToRTC(const PetEngine &pet) {
  const PetData &data = pet.getData();
  g_rtcPetState.magic = RTC_PET_MAGIC;
  g_rtcPetState.hunger = data.hunger;
  g_rtcPetState.happiness = data.happiness;
  g_rtcPetState.energy = data.energy;
  g_rtcPetState.cleanliness = data.cleanliness;
  g_rtcPetState.health = data.health;
  g_rtcPetState.stage = data.stage;
  g_rtcPetState.state = data.state;
  g_rtcPetState.age_minutes = data.age_minutes;
  g_rtcPetState.birth_timestamp = data.birth_timestamp;
  g_rtcPetState.checksum = calcPetStateChecksum(g_rtcPetState);
}

bool restoreStateFromRTC(PetEngine &pet) {
  if (!validateRTCPetState(g_rtcPetState)) return false;

  // Build JSON from RTC state and restore via PetEngine::fromJson
  // This restores vital stats (but not name — that comes from LittleFS)
  StaticJsonDocument<256> doc;
  doc["name"] = pet.getData().name;  // Preserve current name
  doc["hunger"] = g_rtcPetState.hunger;
  doc["happiness"] = g_rtcPetState.happiness;
  doc["energy"] = g_rtcPetState.energy;
  doc["cleanliness"] = g_rtcPetState.cleanliness;
  doc["health"] = g_rtcPetState.health;
  doc["age_minutes"] = g_rtcPetState.age_minutes;
  doc["stage"] = g_rtcPetState.stage;
  doc["state"] = g_rtcPetState.state;
  doc["generation"] = pet.getData().generation;  // Preserve current generation

  String json;
  serializeJson(doc, json);
  return pet.fromJson(json);
}

void clearRTCState() {
  memset(&g_rtcPetState, 0, sizeof(g_rtcPetState));
}

// ============================================================
// WiFi Power Management
// ============================================================
void reduceWiFiPowerV2() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  esp_wifi_set_max_tx_power(40);  // ~25mW instead of 100mW
  WiFi.setSleep(true);
#endif
}

void restoreWiFiPowerV2() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  esp_wifi_set_max_tx_power(80);
  WiFi.setSleep(false);
#endif
}

// ============================================================
// Watchdog Timer
// ============================================================
void setupWatchdog() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  // Note: esp_task_wdt_init() is called by Arduino framework
  // We add our timer-based watchdog in this module for flexibility
#endif
  g_watchdogTriggered = false;
  g_lastWatchdogFeed = millis();
}

void feedWatchdog() {
  g_lastWatchdogFeed = millis();
  g_watchdogTriggered = false;
}

bool isWatchdogTriggered() {
  return g_watchdogTriggered;
}

// ============================================================
// Main Loop Handler (Phase 23)
// ============================================================
void handlePowerManagerV2(PetEngine &pet, uint32_t currentMillis) {
  // Feed watchdog every tick
  feedWatchdog();

  // Check watchdog timeout
  if (currentMillis - g_lastWatchdogFeed > WATCHDOG_TIMEOUT_MS) {
    g_watchdogTriggered = true;
  }

  if (currentMillis - g_lastSleepCheck < SLEEP_CHECK_INTERVAL) {
    return;
  }
  g_lastSleepCheck = currentMillis;

  // Update battery level with oversampling
  int battPct = getBatteryPercentV2();
  if (battPct >= 0) {
    updateBatteryHistoryV2(battPct);
    applyBatteryBrightness();

    // Critical battery: force deep sleep
    if (isCriticalBatteryV2()) {
#ifdef SERIAL_DEBUG
      Serial.printf("[PowerV2] CRITICAL battery %d%% — entering deep sleep\n", battPct);
#endif
      enterDeepSleepV2(MAX_SLEEP_DURATION_MS);
      return;
    }
  }

  // Check for light sleep eligibility
  uint32_t idle = getIdleDurationMs();
  if (shouldSleepV2(pet, idle)) {
    saveStateToRTC(pet);
    enterLightSleepV2();
  }
}

// ============================================================
// JSON Helpers
// ============================================================
String getBatteryJsonV2(const PetEngine &pet) {
  StaticJsonDocument<256> doc;
  int pct = getBatteryPercentV2();
  doc["batteryLevel"] = pct;
  doc["lowBattery"] = isLowBatteryV2();
  doc["criticalBattery"] = isCriticalBatteryV2();
  doc["charging"] = isChargingV2();
  doc["hasMonitor"] = (pct >= 0);

  int est = getEstimatedBatteryHoursV2();
  if (est >= 0) {
    doc["estimatedHours"] = est;
  }

  doc["brightness"] = getBrightnessForBattery();

  String result;
  serializeJson(doc, result);
  return result;
}

String getSleepModeJsonV2() {
  StaticJsonDocument<256> doc;
  const char *modeStr;
  switch (g_sleepModeV2) {
    case SLEEP_LIGHT_V2: modeStr = "light"; break;
    case SLEEP_DEEP_V2:  modeStr = "deep";   break;
    default:             modeStr = "none";   break;
  }
  doc["sleepMode"] = modeStr;
  doc["wakeInterval"] = (int)g_wakeIntervalV2;
  doc["idleMs"] = getIdleDurationMs();
  doc["shouldSleep"] = getIdleDurationMs() >= IDLE_TIMEOUT_MS;
  doc["canLightSleep"] = true;
  doc["canDeepSleep"] = true;
  doc["deepSleepWake"] = wasDeepSleepWakeV2();
  doc["watchdogOk"] = !g_watchdogTriggered;
  String result;
  serializeJson(doc, result);
  return result;
}

String getPowerStateJson(const PetEngine &pet) {
  StaticJsonDocument<512> doc;
  doc["battery"] = getBatteryJsonV2(pet);
  doc["sleep"] = getSleepModeJsonV2();
  doc["uptimeMs"] = millis();
  doc["idleMs"] = getIdleDurationMs();
  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Initialization
// ============================================================
void setupPowerManagerV2() {
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  analogSetAttenuation(ADC_11db);
#ifdef BATTERY_ADC_PIN
  pinMode(BATTERY_ADC_PIN, INPUT);
#endif
#ifdef CHARGE_DETECT_PIN
  if (CHARGE_DETECT_PIN >= 0) {
    pinMode(CHARGE_DETECT_PIN, INPUT_PULLUP);
  }
#endif
#endif

  // Initialize battery history
  for (int i = 0; i < 24; i++) g_batteryHistory[i] = -1;

  g_lastActivityMs = millis();
  g_lastSleepCheck = 0;
  g_lastWatchdogFeed = millis();
  g_watchdogTriggered = false;

  setupWatchdog();
  enableWakeupSources();
}

// ============================================================
// Compatibility wrappers (Phase 13.5) — delegate to v2
// ============================================================
int  getBatteryPercent() { return getBatteryPercentV2(); }
bool isLowBattery() { return isLowBatteryV2(); }
int  getEstimatedBatteryHours() { return getEstimatedBatteryHoursV2(); }
void updateBatteryHistory(int percent) { updateBatteryHistoryV2(percent); }
bool wasDeepSleepWake() { return wasDeepSleepWakeV2(); }
SleepModeV2 getSleepMode() { return getSleepModeV2(); }
void setSleepMode(SleepModeV2 mode) { setSleepModeV2(mode); }
void setWakeInterval(WakeIntervalV2 interval) { setWakeIntervalV2(interval); }
WakeIntervalV2 getWakeInterval() { return getWakeIntervalV2(); }
void setupPowerManager() { setupPowerManagerV2(); }
