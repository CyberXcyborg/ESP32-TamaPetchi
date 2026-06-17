#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WebServer.h>

// ============================================================
// WiFi Manager with AP fallback
// ============================================================

// Try to connect to stored WiFi, fall back to AP mode
bool setupWiFi();

// Start AP mode for configuration
void startAPMode();

// Check if WiFi is connected
bool isWiFiConnected();

// Reset stored WiFi credentials
void resetWiFiCredentials();

// Get current IP address as String
String getIPAddress();

// Save WiFi credentials (called from web handler)
bool saveWiFiCredentials(const String &ssid, const String &password);

// Register WiFi web endpoints
void registerWiFiRoutes(WebServer &server);

#endif // WIFIMANAGER_H
