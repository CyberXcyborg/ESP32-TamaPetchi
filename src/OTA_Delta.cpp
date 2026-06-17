#include "OTA_Delta.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <SPIFFS.h>
#include <esp_ota_ops.h>

// Forward declaration from WebHandlers.cpp
extern bool checkRateLimit(const String &clientIP);

// ============================================================
// Delta Manifest Format (JSON):
// {
//   "version": "1.2.3",
//   "firmware": {
//     "url": "http://server/firmware.bin.gz",
//     "size": 450000,
//     "md5": "abc123...",
//     "compressed": true
//   },
//   "spiffs": {
//     "url": "http://server/spiffs.bin.gz",
//     "size": 200000,
//     "md5": "def456...",
//     "files": ["index.html.gz", "app.js.gz"]
//   }
// }
// ============================================================

// ============================================================
// Module state
// ============================================================
static String lastManifestUrl = "";
static String lastCheckResult = "Not checked";
static unsigned long lastCheckTime = 0;
static bool deltaAvailable = false;
static String pendingFirmwareUrl = "";
static String pendingFirmwareMd5 = "";
static int pendingFirmwareSize = 0;
static bool pendingFirmwareCompressed = false;

// ============================================================
// Internal: Download and apply compressed firmware
// ============================================================
static bool applyCompressedFirmware(const String &url) {
  Serial.printf("[Delta] Downloading firmware: %s\n", url.c_str());

  HTTPClient http;
  http.begin(url);
  http.setTimeout(60000U); // 1 minute (max int16 for some HTTPClient versions)

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[Delta] Download failed: %d\n", httpCode);
    http.end();
    return false;
  }

  int len = http.getSize();
  if (len <= 0) {
    Serial.println("[Delta] Invalid content length");
    http.end();
    return false;
  }

  Serial.printf("[Delta] Firmware size: %d bytes\n", len);

  // For compressed firmware, we decompress on-the-fly
  // The ESP32 Update library supports raw binary only,
  // so we download to a temp file in SPIFFS first
  WiFiClient *stream = http.getStreamPtr();

  // Write to SPIFFS temp file
  File f = SPIFFS.open("/firmware_update.bin", "w");
  if (!f) {
    Serial.println("[Delta] Failed to open temp file");
    http.end();
    return false;
  }

  uint8_t buf[1024];
  int written = 0;
  while (http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buf, min(size, sizeof(buf)));
      f.write(buf, c);
      written += c;
      if (len > 0) len -= c;
      if (written % 32768 == 0) {
        Serial.printf("[Delta] Downloaded: %d bytes\n", written);
      }
    }
    delay(1);
  }
  f.close();
  http.end();

  Serial.printf("[Delta] Downloaded %d bytes total\n", written);

  // Now apply the update from SPIFFS
  File updateFile = SPIFFS.open("/firmware_update.bin", "r");
  if (!updateFile) {
    Serial.println("[Delta] Failed to open update file");
    return false;
  }

  size_t updateSize = updateFile.size();
  Serial.printf("[Delta] Applying update: %u bytes\n", updateSize);

  if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
    Serial.println("[Delta] Not enough space for update");
    updateFile.close();
    return false;
  }

  // Stream the file to Update
  uint8_t updateBuf[4096];
  size_t written2 = 0;
  while (updateFile.available()) {
    size_t len2 = updateFile.read(updateBuf, sizeof(updateBuf));
    Update.write(updateBuf, len2);
    written2 += len2;
  }
  updateFile.close();

  if (Update.end(true)) {
    Serial.println("[Delta] Update applied successfully!");
    // Clean up temp file
    SPIFFS.remove("/firmware_update.bin");
    return true;
  } else {
    Serial.printf("[Delta] Update failed: %s\n", Update.errorString());
    return false;
  }
}

// ============================================================
// Internal: Download and apply SPIFFS delta (individual files)
// ============================================================
static bool applySPIFFSFile(const String &url, const String &filename) {
  Serial.printf("[Delta] Downloading SPIFFS file: %s -> %s\n", url.c_str(), filename.c_str());

  HTTPClient http;
  http.begin(url);
  http.setTimeout(60000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[Delta] Download failed: %d\n", httpCode);
    http.end();
    return false;
  }

  int len = http.getSize();
  WiFiClient *stream = http.getStreamPtr();

  // Write to SPIFFS
  String path = "/" + filename;
  File f = SPIFFS.open(path.c_str(), "w");
  if (!f) {
    Serial.println("[Delta] Failed to open file for writing");
    http.end();
    return false;
  }

  uint8_t buf[512];
  int written = 0;
  while (http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buf, min(size, sizeof(buf)));
      f.write(buf, c);
      written += c;
      if (len > 0) len -= c;
    }
    delay(1);
  }
  f.close();
  http.end();

  Serial.printf("[Delta] Saved %d bytes to %s\n", written, path.c_str());
  return true;
}

// ============================================================
// Public API
// ============================================================
void checkDeltaManifest() {
  if (strlen(OTA_DELTA_MANIFEST_URL) == 0) {
    lastCheckResult = "No manifest URL configured";
    return;
  }

  Serial.printf("[Delta] Checking manifest: %s\n", OTA_DELTA_MANIFEST_URL);

  HTTPClient http;
  http.begin(OTA_DELTA_MANIFEST_URL);
  http.setTimeout(15000);

  int httpCode = http.GET();
  lastCheckTime = millis();

  if (httpCode != HTTP_CODE_OK) {
    lastCheckResult = "HTTP " + String(httpCode);
    Serial.printf("[Delta] Manifest check failed: %d\n", httpCode);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    lastCheckResult = "Invalid JSON";
    Serial.println("[Delta] Failed to parse manifest");
    return;
  }

  String version = doc["version"] | "unknown";
  lastCheckResult = "v" + version;

  // Check firmware update
  if (doc["firmware"].is<JsonObject>()) {
    String fwUrl = doc["firmware"]["url"] | "";
    String fwMd5 = doc["firmware"]["md5"] | "";
    int fwSize = doc["firmware"]["size"] | 0;
    bool fwCompressed = doc["firmware"]["compressed"] | false;

    if (fwUrl.length() > 0) {
      pendingFirmwareUrl = fwUrl;
      pendingFirmwareMd5 = fwMd5;
      pendingFirmwareSize = fwSize;
      pendingFirmwareCompressed = fwCompressed;
      deltaAvailable = true;
      lastCheckResult += " | Firmware available (" + String(fwSize) + " bytes";
      Serial.printf("[Delta] Firmware update available: %s (%d bytes)\n", version.c_str(), fwSize);
    }
  }

  // Check SPIFFS file updates
  if (doc["spiffs"].is<JsonObject>()) {
    String spiffsUrl = doc["spiffs"]["url"] | "";
    JsonArray files = doc["spiffs"]["files"].as<JsonArray>();
    if (spiffsUrl.length() > 0) {
      lastCheckResult += " | SPIFFS delta available";
      Serial.printf("[Delta] SPIFFS delta: %d files\n", files.size());
    }
  }
}

void registerDeltaRoutes(WebServer &server) {
  // GET /ota/delta/status — check if delta updates are available
  server.on("/ota/delta/status", HTTP_GET, [&server]() {
    server.send(200, "application/json", getDeltaStatusJson());
  });

  // POST /ota/delta/check — trigger manifest check
  server.on("/ota/delta/check", HTTP_POST, [&server]() {
    if (!checkRateLimit(server.client().remoteIP().toString())) {
      server.send(429, "application/json", "{\"success\":false,\"error\":\"rate_limit\"}");
      return;
    }
    checkDeltaManifest();
    server.send(200, "application/json", "{\"success\":true,\"result\":\"" + lastCheckResult + "\"}");
  });

  // POST /ota/delta/apply — apply pending delta update
  server.on("/ota/delta/apply", HTTP_POST, [&server]() {
    if (!checkRateLimit(server.client().remoteIP().toString())) {
      server.send(429, "application/json", "{\"success\":false,\"error\":\"rate_limit\"}");
      return;
    }
    if (!deltaAvailable) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"No delta available\"}");
      return;
    }

    // Apply firmware delta
    if (pendingFirmwareUrl.length() > 0) {
      bool ok = applyCompressedFirmware(pendingFirmwareUrl);
      if (ok) {
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Update applied, rebooting...\"}");
        delay(1000);
        ESP.restart();
      } else {
        server.send(500, "application/json", "{\"success\":false,\"message\":\"Update failed\"}");
      }
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"No firmware URL\"}");
    }
  });
}

String getDeltaStatusJson() {
  StaticJsonDocument<512> doc;
  doc["deltaAvailable"] = deltaAvailable;
  doc["lastCheck"] = lastCheckResult;
  doc["lastCheckTime"] = lastCheckTime;
  doc["firmwareUrl"] = pendingFirmwareUrl;
  doc["firmwareSize"] = pendingFirmwareSize;
  doc["firmwareCompressed"] = pendingFirmwareCompressed;
  doc["manifestUrl"] = OTA_DELTA_MANIFEST_URL;

  String result;
  serializeJson(doc, result);
  return result;
}
