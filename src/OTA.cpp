#include "OTA.h"
#include "config.h"
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <Esp.h>

// Module-level state for OTA progress tracking
static int otaProgress = 0;
static bool otaInProgress = false;
static String otaError = "";

// Convert Update error code to human-readable string
static String getUpdateErrorString() {
  if (!Update.hasError()) return "No error";
  switch (Update.getError()) {
    case UPDATE_ERROR_OK:         return "No error";
    case UPDATE_ERROR_WRITE:      return "Flash write failed — check flash chip";
    case UPDATE_ERROR_ERASE:      return "Flash erase failed — sector may be locked";
    case UPDATE_ERROR_READ:       return "Flash read failed — data corruption";
    case UPDATE_ERROR_SPACE:      return "Not enough flash space — firmware too large";
    case UPDATE_ERROR_SIZE:       return "Bad firmware size — invalid binary";
    case UPDATE_ERROR_STREAM:     return "Stream read timeout — upload interrupted";
    case UPDATE_ERROR_MD5:        return "MD5 checksum mismatch — corrupted upload";
    case UPDATE_ERROR_MAGIC_BYTE: return "Invalid magic byte — not a firmware binary";
    case UPDATE_ERROR_ACTIVATE:   return "Partition activation failed — rollback triggered";
    case UPDATE_ERROR_NO_PARTITION: return "No OTA partition found — check partition table";
    case UPDATE_ERROR_BAD_ARGUMENT: return "Invalid argument to Update library";
    case UPDATE_ERROR_ABORT:      return "Update aborted by user or system";
    default:                      return "Unknown flash error (code: " + String(Update.getError()) + ")";
  }
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    otaProgress = 0;
    otaError = "";
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    DEBUG_PRINTLN("OTA Update Start: " + type);
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    otaProgress = 100;
    DEBUG_PRINTLN("\nOTA Update Complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaProgress = (progress / (total / 100));
    DEBUG_PRINTF("OTA Progress: %u%%\r", otaProgress);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    String errorMsg = "OTA Error: ";
    switch (error) {
      case OTA_AUTH_ERROR:    errorMsg += "Authentication failed — check OTA_PASSWORD"; break;
      case OTA_BEGIN_ERROR:   errorMsg += "Begin failed — " + getUpdateErrorString(); break;
      case OTA_CONNECT_ERROR: errorMsg += "WiFi connect failed — check signal strength"; break;
      case OTA_RECEIVE_ERROR: errorMsg += "Receive failed — upload interrupted or corrupted"; break;
      case OTA_END_ERROR:     errorMsg += "End failed — " + getUpdateErrorString(); break;
      default:                errorMsg += "Unknown error (code: " + String(error) + ")"; break;
    }
    otaError = errorMsg;
    DEBUG_PRINTLN(errorMsg);
  });

  ArduinoOTA.begin();
  DEBUG_PRINTLN("OTA Ready");
}

void handleOTA() {
  ArduinoOTA.handle();
}

void registerOTARoutes(WebServer &server) {
  // POST /update — firmware upload via multipart form (full OTA)
  server.on("/update", HTTP_POST, [&server]() {
    if (Update.hasError()) {
      String detailedError = otaError.isEmpty() ? getUpdateErrorString() : otaError;
      String json = "{\"success\":false,\"message\":\"" + detailedError + "\",\"code\":" + String(Update.getError()) + "}";
      server.send(500, "application/json", json);
    } else {
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Update successful! Rebooting...\"}");
      delay(1000);
      ESP.restart();
    }
  }, [&server]() {
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      otaInProgress = true;
      otaProgress = 0;
      otaError = "";
      DEBUG_PRINTF("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        String err = getUpdateErrorString();
        otaError = "Begin failed: " + err;
        DEBUG_PRINTLN("OTA Begin Error: " + err);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        String err = getUpdateErrorString();
        otaError = "Write failed: " + err;
        DEBUG_PRINTLN("OTA Write Error: " + err);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        DEBUG_PRINTF("Update Success: %u bytes\n", upload.totalSize);
      } else {
        String err = getUpdateErrorString();
        otaError = "End failed: " + err;
        DEBUG_PRINTLN("OTA End Error: " + err);
      }
      otaInProgress = false;
    }
  });

  // GET /ota/status — current OTA progress
  server.on("/ota/status", HTTP_GET, [&server]() {
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["inProgress"] = otaInProgress;
    jsonDoc["progress"] = otaProgress;
    jsonDoc["error"] = otaError;
    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
  });
}
