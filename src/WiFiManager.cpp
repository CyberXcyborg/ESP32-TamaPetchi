#include "WiFiManager.h"
#include "config.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Esp.h>

static const char* WIFI_CONFIG_FILE = "/wifi_config.json";

bool setupWiFi() {
  String ssid = WIFI_SSID;
  String password = WIFI_PASSWORD;

  if (SPIFFS.exists(WIFI_CONFIG_FILE)) {
    File file = SPIFFS.open(WIFI_CONFIG_FILE, "r");
    if (file) {
      StaticJsonDocument<256> jsonDoc;
      if (!deserializeJson(jsonDoc, file)) {
        ssid = jsonDoc["ssid"] | WIFI_SSID;
        password = jsonDoc["password"] | WIFI_PASSWORD;
      }
      file.close();
    }
  }

  Serial.printf("Connecting to: %s\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }

  Serial.println("\nWiFi failed, starting AP...");
  startAPMode();
  return false;
}

void startAPMode() {
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  Serial.printf("AP: %s, IP: %s\n", WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
}

bool isWiFiConnected() { return WiFi.status() == WL_CONNECTED; }

void resetWiFiCredentials() {
  if (SPIFFS.exists(WIFI_CONFIG_FILE)) SPIFFS.remove(WIFI_CONFIG_FILE);
  WiFi.disconnect();
}

String getIPAddress() {
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return WiFi.softAPIP().toString();
}

bool saveWiFiCredentials(const String &ssid, const String &password) {
  File file = SPIFFS.open(WIFI_CONFIG_FILE, "w");
  if (!file) return false;

  StaticJsonDocument<256> jsonDoc;
  jsonDoc["ssid"] = ssid;
  jsonDoc["password"] = password;

  serializeJson(jsonDoc, file);
  file.close();
  return true;
}

void registerWiFiRoutes(WebServer &server) {
  // POST /wifi/reset — clear stored WiFi config and restart AP mode
  server.on("/wifi/reset", HTTP_POST, [&server]() {
    resetWiFiCredentials();
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["success"] = true;
    jsonDoc["message"] = "WiFi credentials cleared. Restarting in AP mode.";
    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
    delay(1000);
    ESP.restart();
  });

  // POST /wifi/connect — save new credentials and try to connect
  server.on("/wifi/connect", HTTP_POST, [&server]() {
    String body = server.arg("plain");
    StaticJsonDocument<256> jsonDoc;
    DeserializationError err = deserializeJson(jsonDoc, body);
    if (err) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
      return;
    }
    String ssid = jsonDoc["ssid"] | "";
    String password = jsonDoc["password"] | "";
    if (ssid.length() == 0) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"SSID required\"}");
      return;
    }
    saveWiFiCredentials(ssid, password);
    StaticJsonDocument<256> resp;
    resp["success"] = true;
    resp["message"] = "WiFi credentials saved. Rebooting...";
    String response;
    serializeJson(resp, response);
    server.send(200, "application/json", response);
    delay(1000);
    ESP.restart();
  });

  // GET /wifi/status — current WiFi connection status
  server.on("/wifi/status", HTTP_GET, [&server]() {
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["connected"] = isWiFiConnected();
    jsonDoc["ip"] = getIPAddress();
    jsonDoc["ssid"] = WiFi.SSID();
    jsonDoc["rssi"] = WiFi.RSSI();
    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
  });
}
