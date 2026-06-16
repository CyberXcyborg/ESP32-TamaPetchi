/usr/bin/bash: warning: setlocale: LC_ALL: cannot change locale (en_US.UTF-8): No such file or directory
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
  server.on("/update",      HTTP_GET,  handleUpdate);
  server.on("/name",        HTTP_POST, handleSetName);
  server.on("/mute",        HTTP_POST, handleMute);
  server.on("/achievements", HTTP_GET, handleAchievements);
  server.on("/type",        HTTP_POST, handleSetType);
  server.on("/revive",      HTTP_POST, handleRevive);
  server.on("/game/start",  HTTP_POST, handleGameStart);
  server.on("/game/action", HTTP_POST, handleGameAction);
  server.on("/game/state",  HTTP_GET,  handleGameState);
  server.on("/music",       HTTP_POST, handleSetMusic);
  server.on("/difficulty",  HTTP_POST, handleSetDifficulty);
}

// ============================================================
// Utility
// ============================================================
static void sendJsonResponse(bool success, const String &message = "") {
  if (!g_server) {
    Serial.println("ERROR: g_server is null in sendJsonResponse");
    return;
  }
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
  if (!g_server) { Serial.println("ERROR: g_server null in handleRoot"); return; }
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    g_server->send(500, "text/plain", "Failed to open index.html");
    return;
  }
  g_server->streamFile(file, "text/html");
  file.close();
}

static void handleGetPet() {
  if (!g_pet || !g_server) {
    Serial.println("ERROR: null pointer in handleGetPet");
    if (g_server) g_server->send(500, "text/plain", "Internal error");
    return;
  }
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

  // Phase 4: new fields
  jsonDoc["isDying"]      = g_pet->isDying;
  jsonDoc["dyingStartTime"] = g_pet->dyingStartTime;
  jsonDoc["isEvolving"]   = g_pet->isEvolving;
  jsonDoc["weather"]      = getWeatherName(g_pet->weather);
  jsonDoc["musicEnabled"] = g_pet->musicEnabled;
  jsonDoc["difficulty"]   = getDifficultyName(g_pet->difficulty);
  jsonDoc["gameCooldown"] = g_pet->gameCooldown;
  jsonDoc["activeGame"]   = g_pet->activeGame;

  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

static void handleFeed() {
  if (!g_pet) { sendJsonResponse(false, "Internal error"); return; }
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  // Phase 6: Clamp stat before action to prevent overflow
  g_pet->hunger = constrain(g_pet->hunger, STAT_MIN, STAT_MAX);
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
  if (!g_pet->isAlive && !g_pet->isDying) { sendJsonResponse(false, "Pet is not alive"); return; }
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

static void handleGameStart() {
  if (!g_pet) { sendJsonResponse(false, "Internal error"); return; }
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  if (g_pet->gameCooldown > 0) { sendJsonResponse(false, "Game on cooldown: " + String(g_pet->gameCooldown) + "s"); return; }
  if (g_pet->energy < 10) { sendJsonResponse(false, "Pet is too tired to play"); return; }

  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  DeserializationError error = deserializeJson(jsonDoc, body);
  if (error) { sendJsonResponse(false, "Invalid JSON body"); return; }

  // Phase 6: Validate game type is an integer in valid range
  if (!jsonDoc["game"].is<int>()) { sendJsonResponse(false, "Game type must be an integer"); return; }
  int gameType = jsonDoc["game"].as<int>();
  if (gameType < 1 || gameType > 3) { sendJsonResponse(false, "Invalid game type — must be 1, 2, or 3"); return; }
  startGame(*g_pet, gameType);
  savePetData(*g_pet);

  String gameState = getGameStateJSON(*g_pet);
  DynamicJsonDocument resp(256);
  resp["success"] = true;
  DynamicJsonDocument gs(512);
  deserializeJson(gs, gameState);
  resp["gameState"] = gs;
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

static void handleGameAction() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  if (g_pet->activeGame == 0) { sendJsonResponse(false, "No active game"); return; }

  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  DeserializationError error = deserializeJson(jsonDoc, body);
  int input = jsonDoc["input"] | -1;

  if (g_pet->activeGame == 1) {
    checkMemoryInput(*g_pet, input);
  } else if (g_pet->activeGame == 2) {
    // Catch game: input is X coordinate, check proximity to target
    int targetX = g_pet->catchTargetX;
    int targetY = g_pet->catchTargetY;
    int dist = abs(input - targetX);
    if (dist < 15) {
      g_pet->gameScore++;
    }
    updateCatchTarget(*g_pet);
  } else if (g_pet->activeGame == 3) {
    // Quiz game
    int answer = jsonDoc["answer"] | -1;
    // Correct answer is always 0 for now (simplified)
    if (answer == 0) {
      g_pet->gameScore++;
    }
    g_pet->gameRound++;
    if (g_pet->gameRound >= 5) {
      endGame(*g_pet, g_pet->gameScore >= 3);
    }
  }

  savePetData(*g_pet);
  String gameState = getGameStateJSON(*g_pet);
  DynamicJsonDocument resp(256);
  resp["success"] = true;
  DynamicJsonDocument gs(512);
  deserializeJson(gs, gameState);
  resp["gameState"] = gs;
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

static void handleGameState() {
  String gameState = getGameStateJSON(*g_pet);
  g_server->send(200, "application/json", gameState);
}

static void handleSetMusic() {
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }
  g_pet->musicEnabled = !g_pet->musicEnabled;
  savePetData(*g_pet);
  DynamicJsonDocument jsonDoc(256);
  jsonDoc["success"] = true;
  jsonDoc["musicEnabled"] = g_pet->musicEnabled;
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

static void handleSetDifficulty() {
  if (!g_pet) { sendJsonResponse(false, "Internal error"); return; }
  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  DeserializationError error = deserializeJson(jsonDoc, body);
  String diffStr = jsonDoc["difficulty"] | "normal";
  diffStr.toLowerCase();
  diffStr.trim();

  // Phase 6: Validate difficulty is one of the expected values
  if (diffStr == "easy") g_pet->difficulty = 0;
  else if (diffStr == "hard") g_pet->difficulty = 2;
  else if (diffStr == "normal") g_pet->difficulty = 1;
  else {
    sendJsonResponse(false, "Invalid difficulty: use easy, normal, or hard");
    return;
  }

  savePetData(*g_pet);
  DynamicJsonDocument resp(256);
  resp["success"] = true;
  resp["difficulty"] = getDifficultyName(g_pet->difficulty);
  String response;
  serializeJson(resp, response);
  g_server->send(200, "application/json", response);
}

static void handleRevive() {
  if (g_pet->isAlive) { sendJsonResponse(false, "Pet is already alive"); return; }
  if (g_pet->isDying) { sendJsonResponse(false, "Window has closed — pet is dead"); return; }
  if (!canRevive(*g_pet)) {
    unsigned long remaining = (300000 - (millis() - g_pet->lastReviveTime)) / 1000;
    sendJsonResponse(false, "Revive on cooldown: " + String(remaining) + "s remaining");
    return;
  }
  revivePet(*g_pet);
  savePetData(*g_pet);
  sendJsonResponse(true);
}

static void handleSetType() {
  if (!g_pet) { sendJsonResponse(false, "Internal error"); return; }
  if (!g_pet->isAlive) { sendJsonResponse(false, "Pet is not alive"); return; }

  String body = g_server->arg("plain");
  DynamicJsonDocument jsonDoc(256);
  DeserializationError error = deserializeJson(jsonDoc, body);

  if (error) {
    sendJsonResponse(false, "Invalid JSON");
    return;
  }

  String typeStr = jsonDoc["type"] | "";
  typeStr.toLowerCase();
  typeStr.trim();

  // Phase 6: Validate type is one of the expected values
  PetType newType = BLOB;
  if (typeStr == "cat") {
    newType = CAT;
  } else if (typeStr == "dog") {
    newType = DOG;
  } else if (typeStr == "blob") {
    newType = BLOB;
  } else {
    sendJsonResponse(false, "Invalid type — must be blob, cat, or dog");
    return;
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

  // Phase 6: Sanitize name — remove control characters and HTML special chars
  String sanitized = "";
  for (unsigned int i = 0; i < newName.length(); i++) {
    char c = newName[i];
    // Allow printable ASCII (32-126) only
    if (c >= 32 && c <= 126) {
      sanitized += c;
    }
  }
  if (sanitized.length() == 0) {
    sendJsonResponse(false, "Name contains only invalid characters");
    return;
  }

  g_pet->name = sanitized;
  g_pet->hasBeenNamed = true;
  savePetData(*g_pet);
  saveAchievements(*g_pet);
  checkAchievements(*g_pet);
  sendJsonResponse(true);
}
