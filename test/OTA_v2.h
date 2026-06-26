// Minimal stub of OTA_v2.h for native unit tests
// The full implementation is provided in test/Phase23_Native.cpp
// The real header is archived in archive/v1/src/OTA_v2.h
#ifndef OTA_V2_H
#define OTA_V2_H

#include <Arduino.h>

class WebServer;

enum OTASessionState {
  OTA_STATE_IDLE = 0,
  OTA_STATE_DOWNLOADING,
  OTA_STATE_VERIFYING,
  OTA_STATE_FLASHING,
  OTA_STATE_COMPLETE,
  OTA_STATE_ERROR
};

struct OTAStatus {
  OTASessionState state;
  int progress;
  uint32_t bytesReceived;
  uint32_t totalBytes;
  String errorMsg;
  String currentVersion;
  String targetVersion;
  bool rollbackAvailable;
  bool signatureValid;
};

typedef void (*OTAProgressCallback)(int percent, const char *status);

void setupOTAv2();
void handleOTAv2();
bool startOTA(const String &url);
bool startOTABuffer(const uint8_t *data, size_t len);
void cancelOTA();
bool verifySignature(const uint8_t *data, size_t len);
bool verifyCurrentFirmware();
void setOTAProgressCallback(OTAProgressCallback cb);
OTAStatus getOTAStatus();
int getOTAProgress();
String getOTAProgressJson();
bool isRollbackAvailableV2();
bool triggerRollbackV2();
void confirmFirmwareStableV2();
String getRollbackStatusJsonV2();
void registerOTAv2Routes(WebServer &server);
void initCrashRecovery();
void markFirmwareStable();
bool checkFailedBoot();
uint32_t getBootCountV2();

#endif // OTA_V2_H
