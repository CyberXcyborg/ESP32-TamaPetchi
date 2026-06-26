#include "OTA_Delta.h"
#include "config.h"

#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#include "WebHandlers.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <SPIFFS.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#endif

#include <ArduinoJson.h>

// ============================================================
// Patch Header Structure (40 bytes)
// ============================================================
//   Offset  Size  Description
//   0       4     Magic: "TAMD"
//   4       4     New firmware size (uint32 LE)
//   8       32    SHA-256 hash of expected output
// ============================================================

static const uint8_t DELTA_MAGIC[4] = {'T', 'A', 'M', 'D'};
static const size_t DELTA_HEADER_SIZE = 40;

// ============================================================
// Module State
// ============================================================
static DeltaStatus g_deltaStatus = {DELTA_IDLE, "", 0, 0, 0, true};

// Stored patch data (in RAM for application)
// For large patches we stream from SPIFFS
static String g_patchFilePath = "";
static String lastManifestUrl = "";
static String lastCheckResult = "Not checked";
static unsigned long lastCheckTime = 0;
static bool deltaAvailable = false;
static String pendingFirmwareUrl = "";
static String pendingFirmwareMd5 = "";
static int pendingFirmwareSize = 0;

// ============================================================
// SHA-256 Helper (ESP32 only — uses mbedtls)
// ============================================================
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
static String computeSHA256(const uint8_t *data, size_t len) {
  uint8_t hash[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, data, len);
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  String result = "";
  for (int i = 0; i < 32; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02x", hash[i]);
    result += hex;
  }
  return result;
}

// Compute SHA-256 of a file in SPIFFS
static String computeFileSHA256(const String &path) {
  File f = SPIFFS.open(path.c_str(), "r");
  if (!f) return "";

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);

  uint8_t buf[1024];
  while (f.available()) {
    size_t n = f.read(buf, sizeof(buf));
    mbedtls_sha256_update(&ctx, buf, n);
  }
  f.close();

  uint8_t hash[32];
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  String result = "";
  for (int i = 0; i < 32; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02x", hash[i]);
    result += hex;
  }
  return result;
}
#endif // !UNIT_TEST

// ============================================================
// bsdiff-style Binary Delta Patch Application (ESP32 only)
// ============================================================
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)

struct DeltaHeader {
  uint8_t magic[4];
  uint32_t newSize;
  uint8_t sha256[32];
};

// Read delta header from a file in SPIFFS
static bool readDeltaHeader(const String &path, DeltaHeader &header) {
  File f = SPIFFS.open(path.c_str(), "r");
  if (!f || f.size() < (int)DELTA_HEADER_SIZE) return false;

  f.read(header.magic, 4);
  f.read((uint8_t *)&header.newSize, 4);
  f.read(header.sha256, 32);
  f.close();

  // Validate magic
  if (memcmp(header.magic, DELTA_MAGIC, 4) != 0) {
    Serial.println("[Delta] Invalid patch magic");
    return false;
  }
  return true;
}

// Apply delta patch from a file
static bool applyDeltaFromFile(const String &patchPath) {
  Serial.printf("[Delta] Applying patch: %s\n", patchPath.c_str());

  DeltaHeader header;
  if (!readDeltaHeader(patchPath, header)) {
    g_deltaStatus.error = "Invalid patch header";
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }

  g_deltaStatus.newFirmwareSize = header.newSize;
  Serial.printf("[Delta] Expected new size: %u bytes\n", header.newSize);
  Serial.printf("[Delta] Expected SHA-256: ");
  for (int i = 0; i < 32; i++) {
    Serial.printf("%02x", header.sha256[i]);
  }
  Serial.println();

  // Get the currently running partition to read source firmware
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running) {
    g_deltaStatus.error = "Cannot find running partition";
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }

  size_t srcSize = running->size;
  Serial.printf("[Delta] Source partition: 0x%06x, size: %u\n", running->address, srcSize);

  // Allocate output buffer
  uint8_t *output = (uint8_t *)malloc(header.newSize);
  if (!output) {
    g_deltaStatus.error = "Cannot allocate output buffer";
    g_deltaStatus.state = DELTA_FAILED;
    Serial.printf("[Delta] Failed to allocate %u bytes\n", header.newSize);
    return false;
  }

  // Read the patch file after the header
  File patchFile = SPIFFS.open(patchPath.c_str(), "r");
  if (!patchFile) {
    free(output);
    g_deltaStatus.error = "Cannot open patch file";
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }
  patchFile.seek(DELTA_HEADER_SIZE);

  // Read source firmware into memory
  uint8_t *source = (uint8_t *)malloc(srcSize);
  if (!source) {
    free(output);
    patchFile.close();
    g_deltaStatus.error = "Cannot allocate source buffer";
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }

  esp_partition_read(running, 0, source, srcSize);

  // Apply patch: simplified bsdiff-style application
  size_t outPos = 0;
  size_t patchPos = DELTA_HEADER_SIZE;

  while (outPos < header.newSize && patchFile.available()) {
    uint32_t copyOffset, copyLen, addLen;

    // Read control triple
    if (patchFile.available() < 12) break;
    patchFile.read((uint8_t *)&copyOffset, 4);
    patchFile.read((uint8_t *)&copyLen, 4);
    patchFile.read((uint8_t *)&addLen, 4);
    patchPos += 12;

    // COPY: copy from source with XOR diff
    if (copyLen > 0) {
      if (outPos + copyLen > header.newSize) {
        Serial.println("[Delta] Patch overflow on COPY");
        break;
      }

      for (uint32_t i = 0; i < copyLen; i++) {
        uint8_t srcByte = source[(copyOffset + i) % srcSize];
        uint8_t diffByte = 0;
        if (patchFile.available()) {
          diffByte = patchFile.read();
          patchPos++;
        }
        output[outPos++] = srcByte ^ diffByte;
      }
    }

    // ADD: append extra bytes
    if (addLen > 0) {
      if (outPos + addLen > header.newSize) {
        Serial.println("[Delta] Patch overflow on ADD");
        break;
      }

      uint8_t extraBuf[256];
      uint32_t remaining = addLen;
      while (remaining > 0) {
        size_t chunk = (remaining > sizeof(extraBuf)) ? sizeof(extraBuf) : remaining;
        if ((size_t)patchFile.available() < chunk) break;
        patchFile.read(extraBuf, chunk);
        memcpy(output + outPos, extraBuf, chunk);
        outPos += chunk;
        remaining -= chunk;
        patchPos += chunk;
      }
    }

    g_deltaStatus.bytesProcessed = outPos;
  }

  patchFile.close();
  free(source);

  Serial.printf("[Delta] Applied %u bytes of %u expected\n", outPos, header.newSize);

  if (outPos != header.newSize) {
    free(output);
    g_deltaStatus.error = "Patch output size mismatch: got " + String(outPos) + " expected " + String(header.newSize);
    g_deltaStatus.state = DELTA_FAILED;
    Serial.println("[Delta] " + g_deltaStatus.error);
    return false;
  }

  // Verify SHA-256
  g_deltaStatus.state = DELTA_VERIFYING;
  String actualHash = computeSHA256(output, header.newSize);

  Serial.printf("[Delta] SHA-256 computed: %s\n", actualHash.c_str());

  uint8_t expectedHash[32];
  memcpy(expectedHash, header.sha256, 32);

  // Convert expected hex string to bytes for comparison
  bool hashMatch = true;
  for (int i = 0; i < 32; i++) {
    char hexStr[3] = {0};
    hexStr[0] = actualHash.charAt(i * 2);
    hexStr[1] = actualHash.charAt(i * 2 + 1);
    uint8_t expectedByte = (uint8_t)strtoul(hexStr, nullptr, 16);
    if (expectedByte != expectedHash[i]) {
      hashMatch = false;
      break;
    }
  }

  // Simpler approach: compare raw bytes
  String expectedHex = "";
  for (int i = 0; i < 32; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02x", expectedHash[i]);
    expectedHex += hex;
  }

  if (actualHash != expectedHex) {
    free(output);
    g_deltaStatus.error = "SHA-256 verification failed";
    g_deltaStatus.state = DELTA_FAILED;
    Serial.printf("[Delta] SHA-256 mismatch:\n  expected: %s\n  actual:   %s\n", expectedHex.c_str(), actualHash.c_str());
    return false;
  }

  Serial.println("[Delta] SHA-256 verified OK");

  // Write to the next OTA partition
  const esp_partition_t *updatePart = esp_ota_get_next_update_partition(NULL);
  if (!updatePart) {
    free(output);
    g_deltaStatus.error = "Cannot find update partition";
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }

  Serial.printf("[Delta] Writing to partition: 0x%06x, size: %u\n", updatePart->address, header.newSize);

  esp_ota_handle_t updateHandle = 0;
  esp_err_t err = esp_ota_begin(updatePart, header.newSize, &updateHandle);
  if (err != ESP_OK) {
    free(output);
    g_deltaStatus.error = "esp_ota_begin failed: " + String(esp_err_to_name(err));
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }

  // Write in chunks
  size_t written = 0;
  while (written < header.newSize) {
    size_t chunk = header.newSize - written;
    if (chunk > 4096) chunk = 4096;
    err = esp_ota_write(updateHandle, output + written, chunk);
    if (err != ESP_OK) {
      esp_ota_abort(updateHandle);
      free(output);
      g_deltaStatus.error = "esp_ota_write failed at offset " + String(written);
      g_deltaStatus.state = DELTA_FAILED;
      return false;
    }
    written += chunk;
  }

  free(output);

  err = esp_ota_end(updateHandle);
  if (err != ESP_OK) {
    g_deltaStatus.error = "esp_ota_end failed: " + String(esp_err_to_name(err));
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }

  // Set the boot partition to the new one
  err = esp_ota_set_boot_partition(updatePart);
  if (err != ESP_OK) {
    g_deltaStatus.error = "esp_ota_set_boot_partition failed: " + String(esp_err_to_name(err));
    g_deltaStatus.state = DELTA_FAILED;
    return false;
  }

  g_deltaStatus.state = DELTA_SUCCESS;
  g_deltaStatus.bytesProcessed = header.newSize;
  Serial.println("[Delta] Patch applied and boot partition set!");
  return true;
}

// ============================================================
// Manifest-based Delta Check (ESP32 only)
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

  // Check firmware update availability
  if (doc["firmware"].is<JsonObject>()) {
    String fwUrl = doc["firmware"]["url"] | "";
    String fwMd5 = doc["firmware"]["md5"] | "";
    int fwSize = doc["firmware"]["size"] | 0;
    bool fwCompressed = doc["firmware"]["compressed"] | false;

    if (fwUrl.length() > 0) {
      pendingFirmwareUrl = fwUrl;
      pendingFirmwareMd5 = fwMd5;
      pendingFirmwareSize = fwSize;
      deltaAvailable = true;
      lastCheckResult += " | Firmware available (" + String(fwSize) + " bytes)";
      if (fwCompressed) lastCheckResult += " (compressed)";
      Serial.printf("[Delta] Firmware update available: %s (%d bytes)\n", version.c_str(), fwSize);
    }
  }

  // Check patch file availability
  if (doc["patch"].is<JsonObject>()) {
    String patchUrl = doc["patch"]["url"] | "";
    int patchSize = doc["patch"]["size"] | 0;
    lastCheckResult += " | Delta patch available (" + String(patchSize) + " bytes)";
    Serial.printf("[Delta] Delta patch available: %s\n", patchUrl.c_str());
  }
}

#endif // !UNIT_TEST

// ============================================================
// Full firmware fallback download (ESP32 only)
// ============================================================
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
static bool applyFullFirmware(const String &url) {
  Serial.printf("[Delta] Full firmware download: %s\n", url.c_str());
  return false; // Not implemented here - use the standard OTA /update endpoint
}
#endif

// ============================================================
// Public API
// ============================================================
void initOTADelta() {
  g_deltaStatus.state = DELTA_IDLE;
  g_deltaStatus.error = "";
  g_deltaStatus.patchSize = 0;
  g_deltaStatus.newFirmwareSize = 0;
  g_deltaStatus.bytesProcessed = 0;
  g_deltaStatus.rollbackOnFail = true;
  Serial.println("[Delta] Initialized");
}

#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
void registerDeltaRoutes(WebServer &server) {
  // GET /api/ota/delta/status — current delta status
  server.on("/api/ota/delta/status", HTTP_GET, [&server]() {
    server.send(200, "application/json", getDeltaStatusJson());
  });

  // POST /api/ota/delta/check — trigger manifest check
  server.on("/api/ota/delta/check", HTTP_POST, [&server]() {
    if (!checkRateLimit(server.client().remoteIP().toString())) {
      server.send(429, "application/json", "{\"success\":false,\"error\":\"rate_limit\"}");
      return;
    }
    checkDeltaManifest();
    server.send(200, "application/json", "{\"success\":\"true\",\"result\":\"" + lastCheckResult + "\"}");
  });

  // POST /api/ota/delta — upload and apply delta patch
  server.on("/api/ota/delta", HTTP_POST, [&server]() {
    if (!checkRateLimit(server.client().remoteIP().toString())) {
      server.send(429, "application/json", "{\"success\":false,\"error\":\"rate_limit\"}");
      return;
    }
    if (g_deltaStatus.state == DELTA_APPLYING || g_deltaStatus.state == DELTA_RECEIVING || g_deltaStatus.state == DELTA_VERIFYING) {
      server.send(409, "application/json", "{\"success\":false,\"error\":\"delta_in_progress\"}");
      return;
    }

    if (g_patchFilePath.length() == 0) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"no_patch_uploaded\"}");
      return;
    }

    server.send(200, "application/json", "{\"success\":true,\"message\":\"Applying delta patch...\"}");

    // Apply the patch (this will block, then reboot on success)
    bool ok = applyDeltaFromFile(g_patchFilePath);

    if (ok) {
      Serial.println("[Delta] Rebooting into new firmware...");
      delay(1000);
      esp_ota_mark_app_valid_cancel_rollback();
      ESP.restart();
    } else {
      Serial.printf("[Delta] Patch failed: %s\n", g_deltaStatus.error.c_str());

      // Fallback: try full OTA if configured
      if (g_deltaStatus.rollbackOnFail && deltaAvailable && pendingFirmwareUrl.length() > 0) {
        Serial.println("[Delta] Attempting full OTA fallback...");
        // Would use standard OTA endpoint here
      }

      // Report failure but don't reboot
      StaticJsonDocument<256> doc;
      doc["success"] = false;
      doc["error"] = g_deltaStatus.error;
      String result;
      serializeJson(doc, result);
      Serial.printf("[Delta] Final result: %s\n", result.c_str());
    }
  }, [&server]() {
    // Upload handler for multipart form
    HTTPUpload &upload = server.upload();
    String patchPath = "/delta_patch.bin";

    if (upload.status == UPLOAD_FILE_START) {
      g_deltaStatus.state = DELTA_RECEIVING;
      g_deltaStatus.error = "";
      g_deltaStatus.bytesProcessed = 0;

      // Remove old patch file if exists
      if (SPIFFS.exists(patchPath)) {
        SPIFFS.remove(patchPath);
      }

      g_patchFilePath = patchPath;
      Serial.printf("[Delta] Receiving patch: %s\n", upload.filename.c_str());

    } else if (upload.status == UPLOAD_FILE_WRITE) {
      // Append to patch file
      File f = SPIFFS.open(patchPath, "a");
      if (f) {
        f.write(upload.buf, upload.currentSize);
        f.close();
        g_deltaStatus.patchSize += upload.currentSize;
        g_deltaStatus.bytesProcessed = g_deltaStatus.patchSize;
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      Serial.printf("[Delta] Patch received: %u bytes\n", g_deltaStatus.patchSize);
      g_deltaStatus.state = DELTA_IDLE; // Ready to be applied by the POST handler
    }
  });
}
#endif // !UNIT_TEST

String getDeltaStatusJson() {
  StaticJsonDocument<512> doc;

  // State as string
  const char *stateStr = "idle";
  switch (g_deltaStatus.state) {
    case DELTA_RECEIVING: stateStr = "receiving"; break;
    case DELTA_APPLYING:   stateStr = "applying"; break;
    case DELTA_VERIFYING:  stateStr = "verifying"; break;
    case DELTA_SUCCESS:    stateStr = "success"; break;
    case DELTA_FAILED:     stateStr = "failed"; break;
    default:               stateStr = "idle"; break;
  }

  doc["state"] = stateStr;
  doc["error"] = g_deltaStatus.error;
  doc["patchSize"] = g_deltaStatus.patchSize;
  doc["newFirmwareSize"] = g_deltaStatus.newFirmwareSize;
  doc["bytesProcessed"] = g_deltaStatus.bytesProcessed;
  doc["deltaAvailable"] = deltaAvailable;
  doc["lastCheck"] = lastCheckResult;
  doc["lastCheckTime"] = lastCheckTime;
  doc["firmwareUrl"] = pendingFirmwareUrl;
  doc["firmwareSize"] = pendingFirmwareSize;

#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  // Expected SHA-256 of current firmware for comparison
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running) {
    doc["currentPartitionAddress"] = running->address;
    doc["currentPartitionSize"] = running->size;
  }
#else
  doc["currentPartitionAddress"] = 0;
  doc["currentPartitionSize"] = 0;
#endif

  String result;
  serializeJson(doc, result);
  return result;
}

bool isDeltaInProgress() {
  return g_deltaStatus.state == DELTA_RECEIVING ||
         g_deltaStatus.state == DELTA_APPLYING ||
         g_deltaStatus.state == DELTA_VERIFYING;
}

DeltaStatus getDeltaStatus() {
  return g_deltaStatus;
}
