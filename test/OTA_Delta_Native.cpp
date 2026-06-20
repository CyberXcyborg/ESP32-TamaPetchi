// OTA_Delta_Native.cpp — Stub implementations for native test builds
// The real OTA_Delta.cpp is ESP32-only (uses mbedtls, esp_ota_ops, etc.)
// This file provides the public API functions for unit testing

#include "OTA_Delta.h"
#include "config.h"
#include <ArduinoJson.h>
#include <string>

// Static state for native builds
static DeltaStatus g_deltaStatus = {DELTA_IDLE, "", 0, 0, 0, true};
static bool deltaAvailable = false;
static std::string lastCheckResult = "Not checked";
static unsigned long lastCheckTime = 0;
static std::string pendingFirmwareUrl = "";
static int pendingFirmwareSize = 0;

void initOTADelta() {
  g_deltaStatus.state = DELTA_IDLE;
  g_deltaStatus.error = "";
  g_deltaStatus.patchSize = 0;
  g_deltaStatus.newFirmwareSize = 0;
  g_deltaStatus.bytesProcessed = 0;
  g_deltaStatus.rollbackOnFail = true;
}

String getDeltaStatusJson() {
  StaticJsonDocument<512> doc;

  const char *stateStr = "idle";
  switch (g_deltaStatus.state) {
    case DELTA_RECEIVING: stateStr = "receiving"; break;
    case DELTA_APPLYING:   stateStr = "applying"; break;
    case DELTA_VERIFYING:  stateStr = "verifying"; break;
    case DELTA_SUCCESS:    stateStr = "success"; break;
    case DELTA_FAILED:     stateStr = "failed"; break;
    default:               stateStr = "idle"; break;
  }

  doc["state"] = stateStr;
  doc["error"] = g_deltaStatus.error.c_str();
  doc["patchSize"] = g_deltaStatus.patchSize;
  doc["newFirmwareSize"] = g_deltaStatus.newFirmwareSize;
  doc["bytesProcessed"] = g_deltaStatus.bytesProcessed;
  doc["deltaAvailable"] = deltaAvailable;
  doc["lastCheck"] = lastCheckResult.c_str();
  doc["lastCheckTime"] = lastCheckTime;
  doc["firmwareUrl"] = pendingFirmwareUrl.c_str();
  doc["firmwareSize"] = pendingFirmwareSize;
  doc["currentPartitionAddress"] = 0;
  doc["currentPartitionSize"] = 0;

  String result;
  serializeJson(doc, result);
  return result;
}

bool isDeltaInProgress() {
  return g_deltaStatus.state == DELTA_RECEIVING ||
         g_deltaStatus.state == DELTA_APPLYING ||
         g_deltaStatus.state == DELTA_VERIFYING;
}

DeltaStatus getDeltaStatus() {
  return g_deltaStatus;
}
