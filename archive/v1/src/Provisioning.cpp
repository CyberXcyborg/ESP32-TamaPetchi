#include "Provisioning.h"
#include "config.h"
#include "WiFiManager.h"
#include "Storage.h"
#include "AppState.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// ============================================================
// Module state
// ============================================================
static bool g_provisioningMode = false;
static String g_deviceID = "";
static bool g_deviceIDLoaded = false;

// ============================================================
// Device ID — derived from ESP32 MAC
// ============================================================
// Format: "TAMA-XXXXXX" where XXXXXX is the last 3 bytes of MAC in HEX
void generateDeviceID() {
  if (g_deviceIDLoaded) return;
  uint64_t mac = ESP.getEfuseMac();
  uint32_t low = (uint32_t)(mac & 0xFFFFFF);
  g_deviceID = "TAMA-" + String(low, HEX);
  g_deviceID.toUpperCase();
  g_deviceIDLoaded = true;
}

String getDeviceID() {
  if (!g_deviceIDLoaded) {
    // Try loading from SPIFFS
    if (SPIFFS.exists("/device_id.json")) {
      File f = SPIFFS.open("/device_id.json", "r");
      if (f) {
        String content = f.readString();
        f.close();
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, content) == DeserializationError::Ok) {
          g_deviceID = doc["deviceID"] | "";
          g_deviceIDLoaded = true;
        }
      }
    }
    // If not found, generate from MAC
    if (!g_deviceIDLoaded || g_deviceID.length() == 0) {
      generateDeviceID();
      // Save to SPIFFS
      setDeviceID(g_deviceID);
    }
  }
  return g_deviceID;
}

void setDeviceID(const String &id) {
  g_deviceID = id;
  g_deviceIDLoaded = true;
  if (SPIFFS.exists("/device_id.json")) {
    SPIFFS.remove("/device_id.json");
  }
  File f = SPIFFS.open("/device_id.json", "w");
  if (f) {
    StaticJsonDocument<128> doc;
    doc["deviceID"] = id;
    String content;
    serializeJson(doc, content);
    f.print(content);
    f.close();
  }
}

bool hasDeviceID() {
  getDeviceID(); // ensure loaded
  return g_deviceID.length() > 0 && g_deviceID.startsWith("TAMA-");
}

// ============================================================
// Provisioning State
// ============================================================
bool isProvisioned() {
  // Check if WiFi credentials exist in SPIFFS (uses WiFiManager's config file)
  if (SPIFFS.exists("/wifi_config.json")) {
    File f = SPIFFS.open("/wifi_config.json", "r");
    if (f) {
      String content = f.readString();
      f.close();
      StaticJsonDocument<256> doc;
      if (deserializeJson(doc, content) == DeserializationError::Ok) {
        String ssid = doc["ssid"] | "";
        if (ssid.length() > 0) {
          return hasDeviceID();
        }
      }
    }
  }
  return false;
}

bool isInProvisioningMode() {
  return g_provisioningMode;
}

void startProvisioningMode() {
  if (g_provisioningMode) return;
  g_provisioningMode = true;
  startAPMode();
  Serial.println("[Provision] Provisioning mode started");
}

void stopProvisioningMode() {
  if (!g_provisioningMode) return;
  g_provisioningMode = false;
  // AP mode is handled by WiFiManager; stopAP if needed
  WiFi.softAPdisconnect(true);
  Serial.println("[Provision] Provisioning mode stopped");
}

// ============================================================
// Factory Reset
// ============================================================
void factoryReset() {
  Serial.println("[Provision] Factory reset initiated");

  // Remove all stored configuration
  SPIFFS.remove("/wifi_config.json");
  SPIFFS.remove("/device_id.json");
  SPIFFS.remove(PET_DATA_FILE);
  SPIFFS.remove(MULTI_PET_FILE);
  SPIFFS.remove(STATS_FILE);
  SPIFFS.remove(NOTIFICATION_FILE);
  SPIFFS.remove("/achievements.json");
  SPIFFS.remove("/lineage.json");
  SPIFFS.remove("/community_gallery.json");
  SPIFFS.remove("/accessibility.json");
  SPIFFS.remove("/backup_meta.json");

  // Reset state
  g_deviceID = "";
  g_deviceIDLoaded = false;
  g_provisioningMode = false;

  Serial.println("[Provision] All data cleared. Restarting...");
  delay(1000);
  ESP.restart();
}

// ============================================================
// Web Routes
// ============================================================
void registerProvisioningRoutes() {
  // Capture AppState reference for use in lambdas
  AppState &state = APP_STATE;

  // GET /api/provisioning/status
  state.server.on("/api/provisioning/status", HTTP_GET, [&state]() {
    state.server.send(200, "application/json", getProvisioningStatusJson());
  });

  // POST /api/provisioning/set — set WiFi credentials during provisioning
  state.server.on("/api/provisioning/set", HTTP_POST, [&state]() {
    if (!g_provisioningMode) {
      state.server.send(400, "application/json",
        "{\"success\":false,\"error\":\"not_in_provisioning_mode\"}");
      return;
    }
    if (!state.server.hasArg("plain")) {
      state.server.send(400, "application/json",
        "{\"success\":false,\"error\":\"missing_body\"}");
      return;
    }
    String body = state.server.arg("plain");
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
      state.server.send(400, "application/json",
        "{\"success\":false,\"error\":\"invalid_json\"}");
      return;
    }
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    if (ssid.length() == 0) {
      state.server.send(400, "application/json",
        "{\"success\":false,\"error\":\"missing_ssid\"}");
      return;
    }
    // Save credentials
    if (saveWiFiCredentials(ssid, password)) {
      state.server.send(200, "application/json",
        "{\"success\":true,\"message\":\"Credentials saved. Connecting...\"}");
      // Try connecting after a short delay
      delay(500);
      stopProvisioningMode();
      WiFi.begin(ssid.c_str(), password.c_str());
    } else {
      state.server.send(500, "application/json",
        "{\"success\":false,\"error\":\"save_failed\"}");
    }
  });

  // POST /api/provisioning/reset — factory reset
  state.server.on("/api/provisioning/reset", HTTP_POST, [&state]() {
    state.server.send(200, "application/json",
      "{\"success\":true,\"message\":\"Factory reset initiated\"}");
    delay(500);
    factoryReset();
  });

  // POST /api/provisioning/deviceid — set custom device ID
  state.server.on("/api/provisioning/deviceid", HTTP_POST, [&state]() {
    if (!state.server.hasArg("plain")) {
      state.server.send(400, "application/json",
        "{\"success\":false,\"error\":\"missing_body\"}");
      return;
    }
    String body = state.server.arg("plain");
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
      state.server.send(400, "application/json",
        "{\"success\":false,\"error\":\"invalid_json\"}");
      return;
    }
    String id = doc["deviceID"] | "";
    if (id.length() < 3 || id.length() > 32) {
      state.server.send(400, "application/json",
        "{\"success\":false,\"error\":\"invalid_id_length\"}");
      return;
    }
    setDeviceID(id);
    state.server.send(200, "application/json",
      "{\"success\":true,\"deviceID\":\"" + getDeviceID() + "\"}");
  });

  Serial.println("[Provision] Routes registered");
}

// ============================================================
// Status JSON
// ============================================================
String getProvisioningStatusJson() {
  StaticJsonDocument<512> doc;
  doc["provisioned"] = isProvisioned();
  doc["provisioningMode"] = g_provisioningMode;
  doc["deviceID"] = getDeviceID();
  doc["hasDeviceID"] = hasDeviceID();
  doc["apSSID"] = WIFI_AP_SSID;
  doc["ipAddress"] = g_provisioningMode ? WiFi.softAPIP().toString() : getIPAddress();
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["flashSize"] = ESP.getFlashChipSize();
  doc["firmwareVersion"] = "1.3.0";

  String result;
  serializeJson(doc, result);
  return result;
}
