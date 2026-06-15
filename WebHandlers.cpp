#include "WebHandlers.h"
#include "Pet.h"
#include "Storage.h"
#include "Achievements.h"
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

  // Phase 2: new fields
  const char* stageStr = "baby";
  switch (g_pet->stage) {
    case BABY:  stageStr = "baby";  break;
    case CHILD: stageStr = "child"; break;
    case ADULT: stageStr = "adult"; break;
    case ELDER: stageStr = "elder"; break;
  }
  jsonDoc["stage"]          = stageStr;
  jsonDoc["isNight"]        = g_pet->isNight;
  jsonDoc["virtualMinutes"] = g_pet->virtualMinutes;

  // Phase 3: name + sound + type
  jsonDoc["name"]         = g_pet->name;
  jsonDoc["soundEnabled"] = g_pet->soundEnabled;
  const char* typeStr = "blob";
  switch (g_pet->type) {
    case BLOB: typeStr = "blob"; break;
    case CAT:  typeStr = "cat";  break;
    case DOG:  typeStr = "dog";  break;
  }
  jsonDoc["type"] = typeStr;

  // Phase 3: achievements array
  String achJson = getAchievementsJson(*g_pet);
  DynamicJsonDocument achDoc(512);
  deserializeJson(achDoc, achJson);
  jsonDoc["achievements"] = achDoc["achievements"];

  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

static void handleFeed() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  feedPet(*g_pet);
  savePetData(*g_pet);
  checkAchievements(*g_pet);
  saveAchievements(*g_pet);
  sendJsonResponse(true);
}

static void handlePlay() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  if (g_pet->energy < PLAY_ENERGY_MIN) { sendJsonResponse(false, "Pet is too tired to play"); return; }
  playPet(*g_pet);
  savePetData(*g_pet);
  checkAchievements(*g_pet);
  saveAchievements(*g_pet);
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
  // Reset achievement tracking
  g_pet->feedCount     = 0;
  g_pet->playCount     = 0;
  g_pet->hasBeenNamed  = false;
  g_pet->elderAchieved = false;
  savePetData(*g_pet);
  saveAchievements(*g_pet);
  sendJsonResponse(true);
}

static void handleUpdate() {
  handleGetPet();
}

static void handleSetType() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }

  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  DeserializationError error = deserializeJson(jsonDoc, body);

  if (error) {
    // Also support plain text body
    body = g_server->arg("plain");
    if (body.length() == 0) {
      // Try to get from form data
    }
  }

  String typeStr = jsonDoc["type"] | "";
  typeStr.toLowerCase();
  typeStr.trim();

  PetType newType = BLOB;
  if (typeStr == "cat") {
    newType = CAT;
  } else if (typeStr == "dog") {
    newType = DOG;
  } else {
    newType = BLOB;
  }

  // Reset pet with new type
  g_pet->type = newType;
  initPet(*g_pet);
  g_pet->type = newType; // preserve type after reset
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handleAchievements() {
  String achJson = getAchievementsJson(*g_pet);
  g_server->send(200, "application/json", achJson);
}

static void handleMute() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }

  g_pet->soundEnabled = !g_pet->soundEnabled;
  savePetData(*g_pet);

  DynamicJsonDocument jsonDoc(256);
  jsonDoc["success"] = true;
  jsonDoc["soundEnabled"] = g_pet->soundEnabled;
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

static void handleSetName() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }

  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  DeserializationError error = deserializeJson(jsonDoc, body);

  if (error) {
    sendJsonResponse(false, "Invalid JSON");
    return;
  }

  String newName = jsonDoc["name"] | "";
  newName.trim();
  if (newName.length() == 0) {
    sendJsonResponse(false, "Name cannot be empty");
    return;
  }
  if (newName.length() > 16) {
    newName = newName.substring(0, 16);
  }

  g_pet->name = newName;
  g_pet->hasBeenNamed = true;
  savePetData(*g_pet);
  saveAchievements(*g_pet);
  checkAchievements(*g_pet);
  sendJsonResponse(true);
}
