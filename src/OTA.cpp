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

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    otaProgress = 0;
    otaError = "";
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("OTA Update Start: " + type);
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    otaProgress = 100;
    Serial.println("\nOTA Update Complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaProgress = (progress / (total / 100));
    Serial.printf("OTA Progress: %u%%\r", otaProgress);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    String errorMsg = "OTA Error: ";
    switch (error) {
      case OTA_AUTH_ERROR:    errorMsg += "Auth Failed"; break;
      case OTA_BEGIN_ERROR:   errorMsg += "Begin Failed"; break;
      case OTA_CONNECT_ERROR: errorMsg += "Connect Failed"; break;
      case OTA_RECEIVE_ERROR: errorMsg += "Receive Failed"; break;
      case OTA_END_ERROR:     errorMsg += "End Failed"; break;
      default:                errorMsg += "Unknown"; break;
    }
    otaError = errorMsg;
    Serial.println(errorMsg);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void handleOTA() {
  ArduinoOTA.handle();
}

void registerOTARoutes(WebServer &server) {
  // POST /update — firmware upload via multipart form
  server.on("/update", HTTP_POST, [&server]() {
    if (Update.hasError()) {
      server.send(500, "application/json", "{\"success\":false,\"message\":\"" + otaError + "\"}");
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
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
        otaError = "Not enough space for update";
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
        otaError = "Write failed";
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u bytes\n", upload.totalSize);
      } else {
        Update.printError(Serial);
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
