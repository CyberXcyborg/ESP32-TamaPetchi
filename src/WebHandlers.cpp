#include "WebHandlers.h"
#include "Pet.h"
#include "Storage.h"
#include "Achievements.h"
#include "MultiPet.h"
#include "Stats.h"
#include "Notifications.h"
#include "PowerManager.h"
#include "WiFiManager.h"
#include "OTA.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// Module-level state
// ============================================================
static MultiPetState* g_multiPet = nullptr;
static WebServer*     g_server   = nullptr;
static GameStats*     g_stats    = nullptr;
bool showWakeMessage = false;
unsigned long wakeMessageStartTime = 0;
String previousState = "";

WebServer* getServer() { return g_server; }

// ============================================================
// Forward Declarations
// ============================================================
static void handleRoot();
static void handleGetPet();
static void handleFeed();
static void handlePlay();
static void handleClean();
static void handleSleep();
static void handleHeal();
static void handleReset();
static void handleSetName();
static void handleMute();
static void handleAchievements();
static void handleSetType();
static void handleStats();
static void handleGetNotifications();
static void handleClearNotifications();
static void handleGetPets();
static void handleSwitchPet();
static void handleCreatePet();
static void handleDeletePet();
static void handleBattery();
static void handleWiFiReset();

// ============================================================
// Route Registration
// ============================================================
void registerHandlers(WebServer &server, MultiPetState &multiPet, GameStats &stats) {
  g_multiPet = &multiPet;
  g_server   = &server;
  g_stats    = &stats;

  // Core routes
  server.on("/",             HTTP_GET,  handleRoot);
  server.on("/pet",          HTTP_GET,  handleGetPet);
  server.on("/feed",         HTTP_POST, handleFeed);
  server.on("/play",         HTTP_POST, handlePlay);
  server.on("/clean",        HTTP_POST, handleClean);
  server.on("/sleep",        HTTP_POST, handleSleep);
  server.on("/heal",         HTTP_POST, handleHeal);
  server.on("/reset",        HTTP_POST, handleReset);
  server.on("/name",         HTTP_POST, handleSetName);
  server.on("/mute",         HTTP_POST, handleMute);
  server.on("/achievements", HTTP_GET,  handleAchievements);
  server.on("/type",         HTTP_POST, handleSetType);

  // Phase 5: Statistics
  server.on("/stats",        HTTP_GET,  handleStats);

  // Phase 5: Notifications
  server.on("/notifications",        HTTP_GET,  handleGetNotifications);
  server.on("/notifications/clear", HTTP_POST, handleClearNotifications);

  // Phase 5: Multi-Pet
  server.on("/pets",             HTTP_GET,  handleGetPets);
  server.on("/pets/switch",     HTTP_POST, handleSwitchPet);
  server.on("/pets/create",     HTTP_POST, handleCreatePet);
  server.on("/pets/delete",     HTTP_POST, handleDeletePet);

  // Phase 5: Battery
  server.on("/battery",         HTTP_GET,  handleBattery);

  // Phase 5: WiFi
  server.on("/wifi/reset",     HTTP_POST, handleWiFiReset);

  // Phase 5: OTA
  registerOTARoutes(server);
}

// ============================================================
// Helpers
// ============================================================
static Pet* activePet() {
  if (!g_multiPet) return nullptr;
  return getActivePet(*g_multiPet);
}

static int activeSlot() {
  if (!g_multiPet) return 0;
  return g_multiPet->activePetIndex;
}

static void sendJsonResponse(bool success, const String &message = "") {
  DynamicJsonDocument jsonDoc(256);
  jsonDoc["success"] = success;
  if (message.length() > 0) jsonDoc["message"] = message;
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
  Pet* pet = activePet();
  if (!pet) { g_server->send(500, "text/plain", "No active pet"); return; }

  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["hunger"]      = pet->hunger;
  jsonDoc["happiness"]   = pet->happiness;
  jsonDoc["health"]      = pet->health;
  jsonDoc["energy"]      = pet->energy;
  jsonDoc["cleanliness"] = pet->cleanliness;
  jsonDoc["age"]         = pet->age;
  jsonDoc["isAlive"]     = pet->isAlive;
  jsonDoc["state"]       = pet->state;

  const char* stageStr = "baby";
  switch (pet->stage) {
    case BABY:  stageStr = "baby";  break;
    case CHILD: stageStr = "child"; break;
    case ADULT: stageStr = "adult"; break;
    case ELDER: stageStr = "elder"; break;
  }
  jsonDoc["stage"]          = stageStr;
  jsonDoc["isNight"]        = pet->isNight;
  jsonDoc["virtualMinutes"] = pet->virtualMinutes;
  jsonDoc["name"]           = pet->name;
  jsonDoc["soundEnabled"]   = pet->soundEnabled;

  const char* typeStr = "blob";
  switch (pet->type) {
    case BLOB: typeStr = "blob"; break;
    case CAT:  typeStr = "cat";  break;
    case DOG:  typeStr = "dog";  break;
  }
  jsonDoc["type"] = typeStr;

  String achJson = getAchievementsJson(*pet);
  DynamicJsonDocument achDoc(512);
  deserializeJson(achDoc, achJson);
  jsonDoc["achievements"] = achDoc["achievements"];

  // Phase 5: notification count
  jsonDoc["notificationCount"] = getUnreadCount(activeSlot());

  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

static void handleFeed() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  feedPet(*pet);
  if (g_stats) { statsOnFeed(*g_stats); saveStats(*g_stats); }
  savePetData(*pet);
  checkAchievements(*pet);
  saveAchievements(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

static void handlePlay() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  if (pet->energy < PLAY_ENERGY_MIN) { sendJsonResponse(false, "Pet is too tired to play"); return; }
  playPet(*pet);
  if (g_stats) { statsOnPlay(*g_stats); saveStats(*g_stats); }
  savePetData(*pet);
  checkAchievements(*pet);
  saveAchievements(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

static void handleClean() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  cleanPet(*pet);
  if (g_stats) { statsOnClean(*g_stats); saveStats(*g_stats); }
  savePetData(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

static void handleSleep() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  sleepPet(*pet);
  if (g_stats) { statsOnSleep(*g_stats); saveStats(*g_stats); }
  savePetData(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

static void handleHeal() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  healPet(*pet);
  if (g_stats) { statsOnHeal(*g_stats); saveStats(*g_stats); }
  savePetData(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

static void handleReset() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  initPet(*pet);
  pet->feedCount     = 0;
  pet->playCount     = 0;
  pet->hasBeenNamed  = false;
  pet->elderAchieved = false;
  savePetData(*pet);
  saveAchievements(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

static void handleSetType() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }

  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  DeserializationError error = deserializeJson(jsonDoc, body);
  String typeStr = error ? "" : (jsonDoc["type"] | "");
  typeStr.toLowerCase();
  typeStr.trim();

  PetType newType = BLOB;
  if (typeStr == "cat")      newType = CAT;
  else if (typeStr == "dog") newType = DOG;

  pet->type = newType;
  savePetData(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

static void handleAchievements() {
  Pet* pet = activePet();
  if (!pet) { g_server->send(500, "text/plain", "No active pet"); return; }
  String achJson = getAchievementsJson(*pet);
  g_server->send(200, "application/json", achJson);
}

static void handleMute() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  pet->soundEnabled = !pet->soundEnabled;
  savePetData(*pet);
  saveMultiPetState(*g_multiPet);

  DynamicJsonDocument jsonDoc(256);
  jsonDoc["success"]      = true;
  jsonDoc["soundEnabled"] = pet->soundEnabled;
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

static void handleSetName() {
  Pet* pet = activePet();
  if (!pet) { sendJsonResponse(false, "No active pet"); return; }
  if (!pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }

  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  if (deserializeJson(jsonDoc, body)) {
    sendJsonResponse(false, "Invalid JSON");
    return;
  }

  String newName = jsonDoc["name"] | "";
  newName.trim();
  if (newName.length() == 0) { sendJsonResponse(false, "Name cannot be empty"); return; }
  if (newName.length() > 16) newName = newName.substring(0, 16);

  pet->name = newName;
  pet->hasBeenNamed = true;
  savePetData(*pet);
  saveAchievements(*pet);
  checkAchievements(*pet);
  saveMultiPetState(*g_multiPet);
  sendJsonResponse(true);
}

// ============================================================
// Phase 5: Statistics Handler
// ============================================================
static void handleStats() {
  if (!g_stats) { g_server->send(500, "text/plain", "Stats not initialized"); return; }
  String statsJson = getStatsJson(*g_stats);
  g_server->send(200, "application/json", statsJson);
}

// ============================================================
// Phase 5: Notification Handlers
// ============================================================
static void handleGetNotifications() {
  int slot = activeSlot();
  String notifJson = getNotificationsJson(slot);
  g_server->send(200, "application/json", notifJson);
}

static void handleClearNotifications() {
  int slot = activeSlot();
  clearNotifications(slot);
  sendJsonResponse(true);
}

// ============================================================
// Phase 5: Multi-Pet Handlers
// ============================================================
static void handleGetPets() {
  String json = getMultiPetJson(*g_multiPet);
  g_server->send(200, "application/json", json);
}

static void handleSwitchPet() {
  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  if (deserializeJson(jsonDoc, body)) {
    sendJsonResponse(false, "Invalid JSON");
    return;
  }
  int slotIndex = jsonDoc["slot"] | -1;
  if (switchPet(*g_multiPet, slotIndex)) {
    sendJsonResponse(true);
  } else {
    sendJsonResponse(false, "Invalid slot");
  }
}

static void handleCreatePet() {
  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  if (deserializeJson(jsonDoc, body)) {
    sendJsonResponse(false, "Invalid JSON");
    return;
  }
  String name = jsonDoc["name"] | "";
  String typeStr = jsonDoc["type"] | "blob";
  typeStr.toLowerCase();
  PetType type = BLOB;
  if (typeStr == "cat")      type = CAT;
  else if (typeStr == "dog") type = DOG;

  int slot = createPet(*g_multiPet, name, type);
  if (slot >= 0) {
    DynamicJsonDocument resp(256);
    resp["success"] = true;
    resp["slot"] = slot;
    String r;
    serializeJson(resp, r);
    g_server->send(200, "application/json", r);
  } else {
    sendJsonResponse(false, "No free pet slots (max 3)");
  }
}

static void handleDeletePet() {
  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  if (deserializeJson(jsonDoc, body)) {
    sendJsonResponse(false, "Invalid JSON");
    return;
  }
  int slotIndex = jsonDoc["slot"] | -1;
  if (deletePet(*g_multiPet, slotIndex)) {
    sendJsonResponse(true);
  } else {
    sendJsonResponse(false, "Cannot delete: invalid slot or last pet");
  }
}

// ============================================================
// Phase 5: Battery Handler
// ============================================================
static void handleBattery() {
  Pet* pet = activePet();
  if (!pet) { g_server->send(500, "text/plain", "No active pet"); return; }
  String json = getBatteryJson(*pet);
  g_server->send(200, "application/json", json);
}

// ============================================================
// Phase 5: WiFi Reset Handler
// ============================================================
static void handleWiFiReset() {
  resetWiFiCredentials();
  sendJsonResponse(true);
  delay(1000);
  ESP.restart();
}
