#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WebServer.h>

// ============================================================
// OTA Update Support
// ============================================================

// OTA configuration
#define OTA_PASSWORD      "tamapetchi"
#define OTA_PORT          3232
#define OTA_HOSTNAME      "TamaPetchi"

// Initialize OTA (call after WiFi is connected)
void setupOTA();

// Handle OTA in loop (call every iteration)
void handleOTA();

// Register OTA web upload endpoints on the web server
void registerOTARoutes(WebServer &server);

#endif // OTA_H
