#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <WebServer.h>

// ============================================================
// MQTT Smart Home Integration
// ============================================================
// Publishes pet state, notifications, and accepts commands
// via MQTT broker. Home Assistant auto-discovery supported.
//
// Compile-time flag: DISABLE_MQTT to exclude entirely.
// ============================================================

// Initialize MQTT (call after WiFi is connected)
void setupMQTT();

// Handle MQTT in loop (call every iteration)
void handleMQTT();

// Publish current pet state topic
void mqttPublishState();

// Publish a notification topic
void mqttPublishNotification(const String &message);

// Register MQTT web endpoints
void registerMQTTRoutes(WebServer &server);

// Check if MQTT is connected
bool isMQTTConnected();

// Get MQTT status as JSON
String getMQTTStatusJson();

#endif // MQTT_H
