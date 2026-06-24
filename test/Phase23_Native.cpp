// Native test stubs for Phase 23 modules
// These provide stub implementations for functions that are ESP32-only,
// allowing test_phase23.cpp to link successfully.

#include "PowerManager_v2.h"
#include "OTA_v2.h"
#include "Pet_v2.h"
#include <cstdint>
#include <cstring>

// PetEngine stubs are in BLETradeGame_Native.cpp (shared across Phase 22.5 and 23)

// ============================================================
// PowerManager_v2 native stubs
// ============================================================

// Battery state for tests
static int g_stubBatteryPercent = 75;
static bool g_stubCharging = false;
static float g_stubMinV = BATTERY_VOLTAGE_MIN;
static float g_stubMaxV = BATTERY_VOLTAGE_MAX;

int getBatteryPercentV2() { return g_stubBatteryPercent; }
bool isLowBatteryV2() { return g_stubBatteryPercent >= 0 && g_stubBatteryPercent < LOW_BATTERY_THRESHOLD; }
bool isCriticalBatteryV2() { return g_stubBatteryPercent >= 0 && g_stubBatteryPercent < CRITICAL_BATTERY_THRESHOLD; }
bool isChargingV2() { return g_stubCharging; }

int getEstimatedBatteryHoursV2() { return 48; }  // Stub: 48 hours
void updateBatteryHistoryV2(int percent) { (void)percent; }
void setBatteryCalibration(float minV, float maxV) {
  if (minV > 0 && maxV > minV) {
    g_stubMinV = minV;
    g_stubMaxV = maxV;
  }
}

uint8_t getBrightnessForBattery() {
  if (g_stubBatteryPercent < 0) return 100;
  if (g_stubBatteryPercent < CRITICAL_BATTERY_THRESHOLD) return DISPLAY_OFF_BATTERY;
  if (g_stubBatteryPercent < LOW_BATTERY_THRESHOLD) return DISPLAY_DIM_BATTERY;
  return 100;
}
void applyBatteryBrightness() {}

bool shouldSleepV2(const PetEngine &pet, uint32_t idleTimeMs) {
  if (!pet.isAlive()) return false;
  if (g_stubCharging) return false;
  return (idleTimeMs >= IDLE_TIMEOUT_MS);
}

void setSleepModeV2(SleepModeV2) {}
SleepModeV2 getSleepModeV2() { return SLEEP_LIGHT_V2; }
void setWakeIntervalV2(WakeIntervalV2) {}
WakeIntervalV2 getWakeIntervalV2() { return WAKE_5MIN_V2; }

void enterLightSleepV2(uint32_t) {}
void enterDeepSleepV2(uint32_t) {}
void enableWakeupSources() {}
bool wasDeepSleepWakeV2() { return false; }
uint32_t getWakeupCause() { return 0; }

void saveStateToRTC(const PetEngine &) {}
bool restoreStateFromRTC(PetEngine &) { return false; }
void clearRTCState() {}

void markActivity() {}
uint32_t getIdleDurationMs() { return 0; }

void reduceWiFiPowerV2() {}
void restoreWiFiPowerV2() {}

void setupWatchdog() {}
void feedWatchdog() {}
bool isWatchdogTriggered() { return false; }

void setupPowerManagerV2() {}
void handlePowerManagerV2(PetEngine &, uint32_t) {}

String getBatteryJsonV2(const PetEngine &pet) {
  (void)pet;
  return "{\"batteryLevel\":75,\"lowBattery\":false,\"criticalBattery\":false,\"charging\":false,\"hasMonitor\":true,\"estimatedHours\":48,\"brightness\":100}";
}
String getSleepModeJsonV2() {
  return "{\"sleepMode\":\"light\",\"wakeInterval\":300,\"idleMs\":0,\"shouldSleep\":false,\"canLightSleep\":true,\"canDeepSleep\":true,\"deepSleepWake\":false,\"watchdogOk\":true}";
}
String getPowerStateJson(const PetEngine &pet) {
  (void)pet;
  return "{\"battery\":{},\"sleep\":{},\"uptimeMs\":0,\"idleMs\":0}";
}

// ============================================================
// OTA_v2 native stubs
// ============================================================

static OTAStatus g_otaStatus;

void setupOTAv2() {
  g_otaStatus.state = OTA_STATE_IDLE;
  g_otaStatus.progress = 0;
  g_otaStatus.rollbackAvailable = false;
  g_otaStatus.signatureValid = false;
}
void handleOTAv2() {}

bool startOTA(const String &) { return false; }
bool startOTABuffer(const uint8_t *, size_t) { return false; }
void cancelOTA() {}

bool verifySignature(const uint8_t *data, size_t len) {
  if (len < 4) return false;
  const uint8_t *sig = data + (len - 4);
  uint32_t expected = (uint32_t)sig[0] | ((uint32_t)sig[1] << 8) |
                      ((uint32_t)sig[2] << 16) | ((uint32_t)sig[3] << 24);
  // CRC32
  uint32_t c = 0xFFFFFFFF;
  for (size_t i = 0; i < len - 4; i++) {
    c ^= data[i];
    for (int j = 0; j < 8; j++) c = (c >> 1) ^ ((c & 1) ? 0xEDB88320 : 0);
  }
  return ((~c) == expected);
}

bool verifyCurrentFirmware() { return true; }

void setOTAProgressCallback(OTAProgressCallback) {}
OTAStatus getOTAStatus() { return g_otaStatus; }
int getOTAProgress() { return 0; }

String getOTAProgressJson() {
  return "{\"state\":\"idle\",\"progress\":0,\"bytesReceived\":0,\"totalBytes\":0,\"error\":\"\",\"rollbackAvailable\":false,\"signatureValid\":false}";
}

bool isRollbackAvailableV2() { return false; }
bool triggerRollbackV2() { return false; }
void confirmFirmwareStableV2() {}
String getRollbackStatusJsonV2() {
  return "{\"rollbackAvailable\":false,\"bootCount\":0,\"rollbackCount\":0,\"stable\":false,\"crashThreshold\":3}";
}

void registerOTAv2Routes(WebServer &) {}

void initCrashRecovery() {}
void markFirmwareStable() {}
bool checkFailedBoot() { return false; }
uint32_t getBootCountV2() { return 0; }
