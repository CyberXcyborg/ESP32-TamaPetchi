#include "OTA_v2.h"
#include <ArduinoJson.h>

// ESP32-specific headers (not available in native test builds)
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <mbedtls/md.h>
#else
// Native test stubs
#include <cstdint>
#include <cstdlib>
#endif

// ============================================================
// Native Test Stubs
// ============================================================
#if defined(UNIT_TEST) || defined(__linux__) || defined(__APPLE__)

// Stub ESP-IDF types for native builds
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1

typedef enum {
  ESP_OTA_IMG_NEW = 0,
  ESP_OTA_IMG_PENDING_VERIFY,
  ESP_OTA_IMG_VALID,
  ESP_OTA_IMG_INVALID,
  ESP_OTA_IMG_ABORTED,
  ESP_OTA_IMG_UNDEFINED
} esp_ota_img_states_t;

struct esp_partition_t {
  uint32_t address;
  uint32_t size;
  const char *label;
};

struct esp_app_desc_t {
  const char *version;
  const char *project_name;
  const char *date;
  const char *time;
};

esp_err_t esp_ota_mark_app_valid_cancel_rollback() { return ESP_OK; }
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot() { return ESP_OK; }

const esp_partition_t *esp_ota_get_running_partition() {
  static esp_partition_t part = {0x10000, 0x100000, "factory"};
  return &part;
}
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *) {
  return NULL;  // No OTA partition in test
}
esp_err_t esp_ota_get_state_partition(const esp_partition_t *, esp_ota_img_states_t *state) {
  *state = ESP_OTA_IMG_VALID;
  return ESP_OK;
}
esp_err_t esp_ota_get_partition_description(const esp_partition_t *, esp_app_desc_t *desc) {
  desc->version = "2.0.0-test";
  desc->project_name = "TamaPetchi";
  desc->date = "2026-06-24";
  desc->time = "12:00:00";
  return ESP_OK;
}

// Simple CRC32 for native signature verification instead of mbedtls
static uint32_t native_crc32_table[256];
static bool native_crc32_init = false;
static void init_native_crc32() {
  if (native_crc32_init) return;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t c = i;
    for (int j = 0; j < 8; j++) {
      c = (c >> 1) ^ ((c & 1) ? 0xEDB88320 : 0);
    }
    native_crc32_table[i] = c;
  }
  native_crc32_init = true;
}
static uint32_t native_crc32(const uint8_t *data, size_t len) {
  init_native_crc32();
  uint32_t c = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    c = native_crc32_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
  }
  return ~c;
}
#endif

// ============================================================
// Module state
// ============================================================
static OTAStatus g_otaStatus;
static OTAProgressCallback g_progressCb = NULL;
static uint8_t g_sha256[32];

// RTC memory for crash recovery across reboots
typedef struct {
  uint32_t bootCount;
  uint32_t rollbackCount;
  uint32_t magic;
  uint8_t  sha256[32];
  bool     stable;
  uint32_t checksum;
} OTARtcData;

#define OTA_RTC_MAGIC 0x04414352  // "ROTA"

static OTARtcData __attribute__((section(".noinit"))) g_rtcRollback;

// ============================================================
// Checksum helpers
// ============================================================
static uint32_t calcRollbackChecksum(const OTARtcData &data) {
  uint32_t cs = data.bootCount ^ data.rollbackCount ^ data.magic;
  cs ^= (data.stable ? 1 : 0);
  for (int i = 0; i < 32; i++) {
    cs ^= data.sha256[i] << (i % 24);
  }
  return ~cs;
}

static bool validateRollbackRTC(const OTARtcData &data) {
  if (data.magic != OTA_RTC_MAGIC) return false;
  return (calcRollbackChecksum(data) == data.checksum);
}

static void writeRollbackRTC(OTARtcData &data) {
  data.magic = OTA_RTC_MAGIC;
  data.checksum = calcRollbackChecksum(data);
}

// ============================================================
// SHA-256 signature verification
// ============================================================
static void computeSHA256(const uint8_t *data, size_t len, uint8_t *out) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, data, len);
  mbedtls_md_finish(&ctx, out);
  mbedtls_md_free(&ctx);
}

bool verifySignature(const uint8_t *data, size_t len) {
  if (len < 4) return false;

  // Simple signature: last 4 bytes are CRC32 of preceding data
  const uint8_t *sig = data + (len - 4);
  uint32_t expected = (uint32_t)sig[0] | ((uint32_t)sig[1] << 8) |
                      ((uint32_t)sig[2] << 16) | ((uint32_t)sig[3] << 24);

#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  // ESP32: use SHA-256
  if (len < 32) return false;
  const uint8_t *shaSig = data + (len - 32);
  uint8_t computed[32];
  computeSHA256(data, len - 32, computed);
  return (memcmp(shaSig, computed, 32) == 0);
#else
  // Native: use CRC32
  uint32_t computed = native_crc32(data, len - 4);
  return (computed == expected);
#endif
}

bool verifyCurrentFirmware() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running) return false;

  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK) return false;

  return (state == ESP_OTA_IMG_VALID);
}

// ============================================================
// Progress tracking
// ============================================================
static void updateProgress(int percent, const char *status) {
  g_otaStatus.progress = percent;
  if (status) {
    g_otaStatus.errorMsg = status;
  }
  if (g_progressCb) {
    g_progressCb(percent, status);
  }
}

void setOTAProgressCallback(OTAProgressCallback cb) {
  g_progressCb = cb;
}

int getOTAProgress() {
  return g_otaStatus.progress;
}

OTAStatus getOTAStatus() {
  return g_otaStatus;
}

String getOTAProgressJson() {
  StaticJsonDocument<512> doc;
  const char *stateStr;
  switch (g_otaStatus.state) {
    case OTA_STATE_DOWNLOADING: stateStr = "downloading"; break;
    case OTA_STATE_VERIFYING:  stateStr = "verifying";  break;
    case OTA_STATE_FLASHING:   stateStr = "flashing";    break;
    case OTA_STATE_COMPLETE:   stateStr = "complete";    break;
    case OTA_STATE_ERROR:      stateStr = "error";       break;
    default:                   stateStr = "idle";        break;
  }
  doc["state"] = stateStr;
  doc["progress"] = g_otaStatus.progress;
  doc["bytesReceived"] = g_otaStatus.bytesReceived;
  doc["totalBytes"] = g_otaStatus.totalBytes;
  doc["error"] = g_otaStatus.errorMsg;
  doc["currentVersion"] = g_otaStatus.currentVersion;
  doc["targetVersion"] = g_otaStatus.targetVersion;
  doc["rollbackAvailable"] = g_otaStatus.rollbackAvailable;
  doc["signatureValid"] = g_otaStatus.signatureValid;
  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// OTA from buffer (for web upload)
// ============================================================
bool startOTABuffer(const uint8_t *data, size_t len) {
  if (g_otaStatus.state != OTA_STATE_IDLE) {
    return false;
  }

  g_otaStatus.state = OTA_STATE_VERIFYING;
  g_otaStatus.bytesReceived = 0;
  g_otaStatus.totalBytes = len;
  g_otaStatus.errorMsg = "";
  g_otaStatus.signatureValid = false;
  updateProgress(0, "Verifying signature");

  // Verify signature
  if (!verifySignature(data, len)) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "Signature verification failed";
    updateProgress(0, "Signature invalid");
    return false;
  }
  g_otaStatus.signatureValid = true;

  // Get next OTA partition
  const esp_partition_t *updatePart = esp_ota_get_next_update_partition(NULL);
  if (!updatePart) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "No OTA partition found";
    return false;
  }

  g_otaStatus.state = OTA_STATE_FLASHING;
  updateProgress(10, "Starting flash");

  esp_ota_handle_t otaHandle;
  esp_err_t err = esp_ota_begin(updatePart, OTA_SIZE_UNKNOWN, &otaHandle);
  if (err != ESP_OK) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "OTA begin failed: " + String(esp_err_to_name(err));
    return false;
  }

  // Write firmware (skip signature at end)
#if !defined(UNIT_TEST) && !defined(__linux__) && !defined(__APPLE__)
  size_t writeLen = len - 32;  // Exclude SHA-256 signature
#else
  size_t writeLen = len - 4;   // Exclude CRC32 signature
#endif
  updateProgress(20, "Writing firmware");

  err = esp_ota_write(otaHandle, data, writeLen);
  if (err != ESP_OK) {
    esp_ota_abort(otaHandle);
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "OTA write failed: " + String(esp_err_to_name(err));
    return false;
  }

  updateProgress(80, "Finalizing");

  err = esp_ota_end(otaHandle);
  if (err != ESP_OK) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "OTA end failed: " + String(esp_err_to_name(err));
    return false;
  }

  // Set boot partition
  err = esp_ota_set_boot_partition(updatePart);
  if (err != ESP_OK) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "Set boot failed: " + String(esp_err_to_name(err));
    return false;
  }

  g_otaStatus.state = OTA_STATE_COMPLETE;
  g_otaStatus.bytesReceived = len;
  updateProgress(100, "Complete — reboot to apply");

  return true;
}

// ============================================================
// OTA from URL
// ============================================================
bool startOTA(const String &url) {
  if (g_otaStatus.state != OTA_STATE_IDLE) {
    return false;
  }

  g_otaStatus.state = OTA_STATE_DOWNLOADING;
  g_otaStatus.errorMsg = "";
  updateProgress(0, "Connecting");

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "HTTP error: " + String(httpCode);
    http.end();
    return false;
  }

  int len = http.getSize();
  if (len <= 0) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "Invalid content length";
    http.end();
    return false;
  }

  g_otaStatus.totalBytes = len;
  g_otaStatus.bytesReceived = 0;

  // Read into buffer
  std::vector<uint8_t> buffer(len);
  WiFiClient *stream = http.getStreamPtr();
  int read = 0;
  while (stream->available() && read < len) {
    int chunk = stream->readBytes(&buffer[read], len - read);
    read += chunk;
    g_otaStatus.bytesReceived = read;
    int pct = (int)((float)read / len * 10);  // 0-10% for download
    updateProgress(pct, "Downloading");
  }
  http.end();

  if (read != len) {
    g_otaStatus.state = OTA_STATE_ERROR;
    g_otaStatus.errorMsg = "Download incomplete";
    return false;
  }

  // Now flash from buffer
  return startOTABuffer(buffer.data(), buffer.size());
}

void cancelOTA() {
  if (g_otaStatus.state == OTA_STATE_IDLE) return;
  g_otaStatus.state = OTA_STATE_IDLE;
  g_otaStatus.progress = 0;
  g_otaStatus.errorMsg = "Cancelled";
  updateProgress(0, "Cancelled");
}

// ============================================================
// Crash Recovery & Rollback
// ============================================================
void initCrashRecovery() {
  if (validateRollbackRTC(g_rtcRollback)) {
    g_rtcRollback.bootCount++;
    writeRollbackRTC(g_rtcRollback);
  } else {
    memset(&g_rtcRollback, 0, sizeof(g_rtcRollback));
    g_rtcRollback.bootCount = 1;
    g_rtcRollback.rollbackCount = 0;
    g_rtcRollback.stable = false;
    writeRollbackRTC(g_rtcRollback);
  }

  // Check rollback availability
  g_otaStatus.rollbackAvailable = isRollbackAvailableV2();

  // Check crash threshold
  if (g_rtcRollback.bootCount >= 3 && g_otaStatus.rollbackAvailable && !g_rtcRollback.stable) {
    triggerRollbackV2();
  }
}

bool isRollbackAvailableV2() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running) return false;

  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK) return false;

  if (state == ESP_OTA_IMG_PENDING_VERIFY) return true;

  // Check if there's a valid previous partition
  const esp_partition_t *prev = esp_ota_get_next_update_partition(NULL);
  if (prev && prev != running) {
    esp_ota_img_states_t prevState;
    if (esp_ota_get_state_partition(prev, &prevState) == ESP_OK) {
      if (prevState == ESP_OTA_IMG_VALID) return true;
    }
  }
  return false;
}

bool triggerRollbackV2() {
  if (!g_otaStatus.rollbackAvailable) return false;

  esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();
  if (err == ESP_OK) {
    g_rtcRollback.rollbackCount++;
    writeRollbackRTC(g_rtcRollback);
    return true;
  }
  return false;
}

void confirmFirmwareStableV2() {
  if (g_rtcRollback.stable) return;

  esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
  if (err == ESP_OK) {
    g_rtcRollback.stable = true;
    g_rtcRollback.bootCount = 0;
    writeRollbackRTC(g_rtcRollback);
  }
}

void markFirmwareStable() {
  confirmFirmwareStableV2();
}

bool checkFailedBoot() {
  return (g_rtcRollback.bootCount >= 2);
}

uint32_t getBootCountV2() {
  return g_rtcRollback.bootCount;
}

String getRollbackStatusJsonV2() {
  StaticJsonDocument<512> doc;
  doc["rollbackAvailable"] = g_otaStatus.rollbackAvailable;
  doc["bootCount"] = g_rtcRollback.bootCount;
  doc["rollbackCount"] = g_rtcRollback.rollbackCount;
  doc["stable"] = g_rtcRollback.stable;
  doc["crashThreshold"] = 3;

  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running) {
    doc["runningPartition"] = running->label;
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
      switch (state) {
        case ESP_OTA_IMG_NEW:           doc["firmwareState"] = "new"; break;
        case ESP_OTA_IMG_PENDING_VERIFY: doc["firmwareState"] = "pending_verify"; break;
        case ESP_OTA_IMG_VALID:         doc["firmwareState"] = "valid"; break;
        case ESP_OTA_IMG_INVALID:       doc["firmwareState"] = "invalid"; break;
        case ESP_OTA_IMG_ABORTED:       doc["firmwareState"] = "aborted"; break;
        case ESP_OTA_IMG_UNDEFINED:     doc["firmwareState"] = "undefined"; break;
        default:                        doc["firmwareState"] = "unknown"; break;
      }
    }
  }
  doc["uptimeSeconds"] = millis() / 1000;
  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Web Endpoints
// ============================================================
void registerOTAv2Routes(WebServer &server) {
  // GET /api/ota/v2/status
  server.on("/api/ota/v2/status", HTTP_GET, [&server]() {
    server.send(200, "application/json", getOTAProgressJson());
  });

  // GET /api/ota/v2/rollback
  server.on("/api/ota/v2/rollback", HTTP_GET, [&server]() {
    server.send(200, "application/json", getRollbackStatusJsonV2());
  });

  // POST /api/ota/v2/rollback
  server.on("/api/ota/v2/rollback", HTTP_POST, [&server]() {
    if (triggerRollbackV2()) {
      server.send(200, "application/json",
        "{\"success\":true,\"message\":\"Rollback initiated\"}");
    } else {
      server.send(400, "application/json",
        "{\"success\":false,\"message\":\"No rollback available\"}");
    }
  });

  // POST /api/ota/v2/confirm
  server.on("/api/ota/v2/confirm", HTTP_POST, [&server]() {
    confirmFirmwareStableV2();
    server.send(200, "application/json",
      "{\"success\":true,\"message\":\"Firmware confirmed stable\"}");
  });
}

// ============================================================
// Initialization
// ============================================================
void setupOTAv2() {
  memset(&g_otaStatus, 0, sizeof(g_otaStatus));
  g_otaStatus.state = OTA_STATE_IDLE;
  g_otaStatus.progress = 0;
  g_otaStatus.rollbackAvailable = false;
  g_otaStatus.signatureValid = false;

  // Get current version
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running) {
    esp_app_desc_t appDesc;
    if (esp_ota_get_partition_description(running, &appDesc) == ESP_OK) {
      g_otaStatus.currentVersion = appDesc.version;
    }
  }

  initCrashRecovery();
}

void handleOTAv2() {
  // Periodic tasks: check for pending confirmations, etc.
  // Mainly idle — OTA is triggered via web API or loop
}
