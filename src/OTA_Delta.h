#ifndef OTA_DELTA_H
#define OTA_DELTA_H

#include <Arduino.h>
#include <WebServer.h>

// ============================================================
// OTA Delta Updates — Bandwidth-Optimized Firmware Updates
// ============================================================
// Reduces OTA bandwidth through:
// 1. Compressed firmware upload (gzip the .bin before sending)
// 2. Delta manifest check — query a manifest URL to see if a
//    smaller delta patch is available before full firmware upload
// 3. SPIFFS delta — update only changed web assets without
//    re-uploading the entire filesystem image
//
// Compile-time flag: DISABLE_OTA_DELTA to exclude
// ============================================================

// Check for delta manifest on boot (if manifest URL configured)
void checkDeltaManifest();

// Register delta web endpoints
void registerDeltaRoutes(WebServer &server);

// Get delta status as JSON
String getDeltaStatusJson();

#endif // OTA_DELTA_H
