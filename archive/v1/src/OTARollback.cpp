#include "OTARollback.h"
#include "config.h"
#include "AppState.h"
#include "WebHandlers.h"
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <ArduinoJson.h>

// ============================================================
// RTC Memory for Boot Tracking
// ============================================================
// Use RTC slow memory — survives warm reset and deep sleep,
// cleared on power cycle (RTC memory is cleared by RTC power
// domain reset, which happens on full power cycle).
// We use a dedicated section in DRAM that the startup code
// doesn't zero (placed in .noinit section).
// ============================================================
static RTCRollbackData rtcData __attribute__((section(".noinit")));

// Module state
static bool rollbackAvailable = false;
static bool rollbackTriggered = false;
static unsigned long stableStartTime = 0;
static bool firmwareConfirmed = false;

// ============================================================
// Checksum helpers
// ============================================================
static uint32_t calcChecksum(const RTCRollbackData &data) {
  // Simple XOR checksum of all fields except checksum itself
  uint32_t cs = data.bootCount ^ data.rollbackCount ^ data.magic;
  return ~cs;  // Invert so zero-filled memory = invalid
}

static bool validateRTCData(const RTCRollbackData &data) {
  if (data.magic != RTC_ROLLBACK_MAGIC) return false;
  return (calcChecksum(data) == data.checksum);
}

static void writeRTCData(RTCRollbackData &data) {
  data.magic = RTC_ROLLBACK_MAGIC;
  data.checksum = calcChecksum(data);
}

// ============================================================
// OTA Partition Helpers
// ============================================================
static bool checkRollbackAvailable() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running) return false;

  esp_ota_img_states_t ota_state;
  esp_err_t err = esp_ota_get_state_partition(running, &ota_state);
  if (err != ESP_OK) return false;

  // Rollback is available if we have a valid previous partition
  // and the current firmware hasn't been confirmed yet
  if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
    return true;
  }

  // Also check if there's a previous OTA partition to roll back to
  const esp_partition_t *prev = esp_ota_get_next_update_partition(NULL);
  if (prev && prev != running) {
    esp_ota_img_states_t prev_state;
    if (esp_ota_get_state_partition(prev, &prev_state) == ESP_OK) {
      if (prev_state == ESP_OTA_IMG_VALID) {
        return true;
      }
    }
  }

  return false;
}

// ============================================================
// Public API
// ============================================================

void initRollbackSystem() {
  // Check if RTC data is valid
  if (validateRTCData(rtcData)) {
    // Increment boot count
    rtcData.bootCount++;
    writeRTCData(rtcData);
    Serial.printf("[Rollback] Boot count: %u\n", rtcData.bootCount);
  } else {
    // First boot after power cycle or corrupted data
    memset(&rtcData, 0, sizeof(rtcData));
    rtcData.bootCount = 1;
    rtcData.rollbackCount = 0;
    writeRTCData(rtcData);
    Serial.println("[Rollback] Fresh boot — RTC data initialized");
  }

  // Check rollback availability
  rollbackAvailable = checkRollbackAvailable();

  // Check if we've exceeded crash threshold
  if (rtcData.bootCount >= RTC_CRASH_THRESHOLD && rollbackAvailable && !firmwareConfirmed) {
    Serial.printf("[Rollback] CRASH THRESHOLD EXCEEDED (%u boots) — auto-rollback!\n", rtcData.bootCount);
    triggerRollback();
    return;
  }

  // Mark firmware as pending verify
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running) {
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
      if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        Serial.println("[Rollback] Firmware state: PENDING_VERIFY");
        Serial.println("[Rollback] Call confirmFirmwareStable() after validation period");
      } else if (ota_state == ESP_OTA_IMG_VALID) {
        Serial.println("[Rollback] Firmware state: VALID");
        firmwareConfirmed = true;
      } else if (ota_state == ESP_OTA_IMG_INVALID) {
        Serial.println("[Rollback] Firmware state: INVALID — rollback recommended");
      } else if (ota_state == ESP_OTA_IMG_ABORTED) {
        Serial.println("[Rollback] Firmware state: ABORTED");
      }
    }
  }

  stableStartTime = millis();
}

void confirmFirmwareStable() {
  if (firmwareConfirmed) return;

  esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
  if (err == ESP_OK) {
    firmwareConfirmed = true;
    // Reset boot count on successful confirmation
    rtcData.bootCount = 0;
    writeRTCData(rtcData);
    Serial.println("[Rollback] Firmware confirmed VALID — rollback cancelled");
  } else {
    Serial.printf("[Rollback] Failed to confirm firmware: %d\n", err);
  }
}

bool isRollbackAvailable() {
  return rollbackAvailable;
}

bool triggerRollback() {
  if (!rollbackAvailable) {
    Serial.println("[Rollback] No rollback available");
    return false;
  }

  Serial.println("[Rollback] Initiating rollback...");

  esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();
  if (err == ESP_OK) {
    // This function reboots the ESP32, so we won't reach here
    Serial.println("[Rollback] Rollback initiated — rebooting...");
    rtcData.rollbackCount++;
    writeRTCData(rtcData);
    return true;
  } else {
    Serial.printf("[Rollback] Rollback failed: %d\n", err);
    return false;
  }
}

String getRollbackStatusJson() {
  StaticJsonDocument<512> doc;

  const esp_partition_t *running = esp_ota_get_running_partition();
  doc["rollbackAvailable"] = rollbackAvailable;
  doc["bootCount"] = rtcData.bootCount;
  doc["rollbackCount"] = rtcData.rollbackCount;
  doc["crashThreshold"] = RTC_CRASH_THRESHOLD;
  doc["firmwareConfirmed"] = firmwareConfirmed;

  if (running) {
    doc["runningPartition"] = running->label;
    doc["runningAddress"] = String("0x") + String(running->address, HEX);

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
      switch (ota_state) {
        case ESP_OTA_IMG_NEW:           doc["firmwareState"] = "new"; break;
        case ESP_OTA_IMG_PENDING_VERIFY: doc["firmwareState"] = "pending_verify"; break;
        case ESP_OTA_IMG_VALID:         doc["firmwareState"] = "valid"; break;
        case ESP_OTA_IMG_INVALID:       doc["firmwareState"] = "invalid"; break;
        case ESP_OTA_IMG_ABORTED:       doc["firmwareState"] = "aborted"; break;
        case ESP_OTA_IMG_UNDEFINED:     doc["firmwareState"] = "undefined"; break;
        default:                        doc["firmwareState"] = "unknown"; break;
      }
    } else {
      doc["firmwareState"] = "error";
    }

    // Get app description
    esp_app_desc_t app_desc;
    if (esp_ota_get_partition_description(running, &app_desc) == ESP_OK) {
      doc["firmwareVersion"] = app_desc.version;
      doc["projectName"] = app_desc.project_name;
      doc["compileDate"] = app_desc.date;
      doc["compileTime"] = app_desc.time;
    }
  }

  // Uptime
  doc["uptimeSeconds"] = millis() / 1000;

  String result;
  serializeJson(doc, result);
  return result;
}

uint32_t getBootCount() {
  return rtcData.bootCount;
}

bool isFirmwareConfirmed() {
  return firmwareConfirmed;
}

unsigned long getStableStartTime() {
  return stableStartTime;
}

// ============================================================
// Web API Routes
// ============================================================
void registerRollbackRoutes() {
  extern WebServer &APP_STATE_server;
  // We use APP_STATE.server directly

  // GET /api/ota/rollback — get rollback status
  APP_STATE.server.on("/api/ota/rollback", HTTP_GET, []() {
    APP_STATE.server.send(200, "application/json", getRollbackStatusJson());
  });

  // POST /api/ota/rollback — trigger manual rollback
  APP_STATE.server.on("/api/ota/rollback", HTTP_POST, []() {
    if (!checkRateLimit(APP_STATE.server.client().remoteIP().toString())) {
      APP_STATE.server.send(429, "application/json",
        "{\"success\":false,\"error\":\"rate_limit\",\"message\":\"Too many requests\"}");
      return;
    }

    if (triggerRollback()) {
      APP_STATE.server.send(200, "application/json",
        "{\"success\":true,\"message\":\"Rollback initiated — device rebooting\"}");
    } else {
      APP_STATE.server.send(400, "application/json",
        "{\"success\":false,\"message\":\"No rollback available or rollback failed\"}");
    }
  });

  // POST /api/ota/confirm — confirm current firmware is stable
  APP_STATE.server.on("/api/ota/confirm", HTTP_POST, []() {
    if (!checkRateLimit(APP_STATE.server.client().remoteIP().toString())) {
      APP_STATE.server.send(429, "application/json",
        "{\"success\":false,\"error\":\"rate_limit\",\"message\":\"Too many requests\"}");
      return;
    }

    confirmFirmwareStable();
    StaticJsonDocument<256> resp;
    resp["success"] = true;
    resp["message"] = "Firmware confirmed stable";
    resp["firmwareConfirmed"] = firmwareConfirmed;
    String result;
    serializeJson(resp, result);
    APP_STATE.server.send(200, "application/json", result);
  });

  // GET /api/ota/status — enhanced OTA status with rollback info
  // This extends the existing /ota/status endpoint
  APP_STATE.server.on("/api/ota/status", HTTP_GET, []() {
    APP_STATE.server.send(200, "application/json", getRollbackStatusJson());
  });
}
