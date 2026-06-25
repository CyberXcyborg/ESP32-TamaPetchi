#ifndef OTA_V2_H
#define OTA_V2_H

#include <Arduino.h>
#include "config_v2.h"

class WebServer;  // Forward declaration

// ============================================================
// OTA v2 — Phase 23
// Enhanced OTA with:
// - A/B partition support (native ESP-IDF)
// - Signature verification (simple hash-based)
// - Progress tracking for UI
// - Rollback on failed boot (auto-revert)
// - Web upload endpoints with progress
// ============================================================

// OTA progress callback
typedef void (*OTAProgressCallback)(int percent, const char *status);

// OTA session state
enum OTASessionState {
  OTA_STATE_IDLE = 0,
  OTA_STATE_DOWNLOADING,
  OTA_STATE_VERIFYING,
  OTA_STATE_FLASHING,
  OTA_STATE_COMPLETE,
  OTA_STATE_ERROR
};

// OTA status info (for web UI)
struct OTAStatus {
  OTASessionState state;
  int progress;           // 0-100
  uint32_t bytesReceived;
  uint32_t totalBytes;
  String errorMsg;
  String currentVersion;
  String targetVersion;
  bool rollbackAvailable;
  bool signatureValid;
};

// --- Initialization ---
void setupOTAv2();
void handleOTAv2();

// --- OTA Operations ---
bool startOTA(const String &url);           // OTA from URL
bool startOTABuffer(const uint8_t *data, size_t len);  // OTA from buffer
void cancelOTA();

// --- Signature Verification ---
bool verifySignature(const uint8_t *data, size_t len);
bool verifyCurrentFirmware();

// --- Progress UI ---
void setOTAProgressCallback(OTAProgressCallback cb);
OTAStatus getOTAStatus();
int getOTAProgress();  // 0-100
String getOTAProgressJson();

// --- Rollback ---
bool isRollbackAvailableV2();
bool triggerRollbackV2();
void confirmFirmwareStableV2();
String getRollbackStatusJsonV2();

// --- Web Endpoints ---
void registerOTAv2Routes(WebServer &server);

// --- Crash Recovery ---
void initCrashRecovery();
void markFirmwareStable();
bool checkFailedBoot();
uint32_t getBootCountV2();

#endif // OTA_V2_H
