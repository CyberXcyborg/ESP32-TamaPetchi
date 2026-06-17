#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"

// ============================================================
// OTA Update Support
// ============================================================

// OTA configuration is in config.h (OTA_PASSWORD, OTA_PORT)

// Initialize OTA (call after WiFi is connected)
void setupOTA();

// Handle OTA in loop (call every iteration)
void handleOTA();

// Register OTA web upload endpoints on the web server
void registerOTARoutes(WebServer &server);

#endif // OTA_H
