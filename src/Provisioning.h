#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>

// ============================================================
// Manufacturing & Provisioning — Phase 13.4
//
// First-boot provisioning flow:
//   - On first boot (no WiFi credentials stored), start AP mode
//     'TamaPetchi-Setup' with captive portal for configuration
//   - Device ID derived from ESP32 MAC address (unique per device)
//   - WiFi credentials stored in SPIFFS, never in firmware
//   - Factory reset restores provisioning mode
// ============================================================

// Device ID (derived from MAC, set once at first boot)
// Stored in SPIFFS at /device_id.json
String getDeviceID();
void setDeviceID(const String &id);
bool hasDeviceID();

// Provisioning state
bool isProvisioned();          // true if WiFi creds + device ID exist
void startProvisioningMode();  // Start AP + captive portal
void stopProvisioningMode();   // Stop AP, switch to station mode
bool isInProvisioningMode();   // true when AP is active

// Factory reset: clear all stored data and enter provisioning
void factoryReset();

// Register provisioning web endpoints
void registerProvisioningRoutes();

// Get provisioning status as JSON
String getProvisioningStatusJson();

#endif // PROVISIONING_H
