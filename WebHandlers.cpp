#include "WebHandlers.h"
#include "Pet.h"
#include "Storage.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// Module-level state
// ============================================================
static Pet*        g_pet        = nullptr;
static WebServer*  g_server     = nullptr;
bool showWakeMessage             = false;
unsigned long wakeMessageStartTime = 0;
String previousState             = "";

WebServer* getServer() { return g_server; }

// ============================================================
// Route Registration
// ============================================================
void registerHandlers(WebServer &server, Pet &pet) {
  g_pet    = &pet;
  g_server = &server;

  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/pet",    HTTP_GET,  handleGetPet);
  server.on("/feed",   HTTP_POST, handleFeed);
  server.on("/play",   HTTP_POST, handlePlay);
  server.on("/clean",  HTTP_POST, handleClean);
  server.on("/sleep",  HTTP_POST, handleSleep);
  server.on("/heal",   HTTP_POST, handleHeal);
  server.on("/reset",  HTTP_POST, handleReset);
  server.on("/update", HTTP_GET,  handleUpdate);
}

// ============================================================
// Utility
// ============================================================
static void sendJsonResponse(bool success, const String &message = "") {
  DynamicJsonDocument jsonDoc(256);
  jsonDoc["success"] = success;
  if (message.length() > 0) {
    jsonDoc["message"] = message;
  }
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

// ============================================================
// Route Handlers
// ============================================================
static void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    g_server->send(500, "text/plain", "Failed to open index.html");
    return;
  }
  g_server->streamFile(file, "text/html");
  file.close();
}

static void handleGetPet() {
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["hunger"]      = g_pet->hunger;
  jsonDoc["happiness"]   = g_pet->happiness;
  jsonDoc["health"]      = g_pet->health;
  jsonDoc["energy"]      = g_pet->energy;
  jsonDoc["cleanliness"] = g_pet->cleanliness;
  jsonDoc["age"]         = g_pet->age;
  jsonDoc["isAlive"]     = g_pet->isAlive;
  jsonDoc["state"]       = g_pet->state;

  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

static void handleFeed() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  feedPet(*g_pet);
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handlePlay() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  if (g_pet->energy < PLAY_ENERGY_MIN) { sendJsonResponse(false, "Pet is too tired to play"); return; }
  playPet(*g_pet);
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handleClean() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  cleanPet(*g_pet);
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handleSleep() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  sleepPet(*g_pet);
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handleHeal() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  healPet(*g_pet);
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handleReset() {
  initPet(*g_pet);
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handleUpdate() {
  handleGetPet();
}
