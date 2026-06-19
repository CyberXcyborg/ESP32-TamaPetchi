#include "WebHandlers.h"
#include "Pet.h"
#include "Storage.h"
#include "Achievements.h"
#include "config.h"
#include "MultiPet.h"
#include "OTA.h"
#include "WiFiManager.h"
#include "Stats.h"
#include "Notifications.h"
#include "PowerManager.h"
#include "WebSocket.h"
#include "i18n.h"
#ifndef DISABLE_IR_REMOTE
#include "IRRemote.h"
#endif
#ifndef DISABLE_MQTT
#include "MQTT.h"
#endif
#ifndef DISABLE_OTA_DELTA
#include "OTA_Delta.h"
#endif
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Forward declarations for all route handlers
void handleRoot();
void handleGetPet();
void handleFeed();
void handlePlay();
void handleClean();
void handleSleep();
void handleHeal();
void handleReset();
void handleUpdate();
void handleSetName();
void handleMute();
void handleAchievements();
void handleSetType();
void handleRevive();
void handleGameStart();
void handleGameAction();
void handleGameState();
void handleSetMusic();
void handleSetDifficulty();
void handleGetMelodyConfig();
void handleSetMelodyConfig();
void handleGetPets();
void handleCreatePet();
void handleSwitchPet();
void handleDeletePet();
void handleGetStats();
void handleGetNotifications();

// Phase 10.3: i18n
void handleGetLanguage();
void handleSetLanguage();
void handleGetLocale();

// Phase 10.4: Factory Reset
void handleFactoryReset();

// ============================================================
// Module-level state (minimal)
// g_server is a convenience macro for APP_STATE.server to avoid
// 100+ line changes. In a future cleanup, call sites can be
// migrated to use APP_STATE.server directly.
// ============================================================
#define g_server (&APP_STATE.server)

// ============================================================
// Rate Limiting (Phase 7.2) — Token bucket per IP
// ============================================================
#define RATE_LIMIT_MAX_TOKENS    10   // Max burst requests
#define RATE_LIMIT_REFILL_RATE   1    // Tokens per second
#define RATE_LIMIT_BUCKET_MAX    32   // Max tracked IPs (to limit memory)
#define RATE_LIMIT_CLEANUP_MS    60000 // Cleanup every 60s

struct RateBucket {
  String ip;
  int tokens;
  unsigned long lastRefill;
  unsigned long lastRequest;
};

static RateBucket rateBuckets[RATE_LIMIT_BUCKET_MAX];
static unsigned long lastRateCleanup = 0;

bool checkRateLimit(const String &clientIP) {
  if (clientIP.length() == 0) return true; // Allow if no IP

  unsigned long now = millis();

  // Find existing bucket or empty slot
  int slot = -1;
  int oldestSlot = 0;
  unsigned long oldestTime = now;

  for (int i = 0; i < RATE_LIMIT_BUCKET_MAX; i++) {
    if (rateBuckets[i].ip == clientIP) {
      slot = i;
      break;
    }
    if (rateBuckets[i].ip.length() == 0) {
      slot = i;
      break;
    }
    if (rateBuckets[i].lastRequest < oldestTime) {
      oldestTime = rateBuckets[i].lastRequest;
      oldestSlot = i;
    }
  }

  // No slot found — evict oldest
  if (slot == -1) {
    slot = oldestSlot;
  }

  // Initialize new bucket
  if (rateBuckets[slot].ip != clientIP) {
    rateBuckets[slot].ip = clientIP;
    rateBuckets[slot].tokens = RATE_LIMIT_MAX_TOKENS;
    rateBuckets[slot].lastRefill = now;
  }

  // Refill tokens based on elapsed time
  unsigned long elapsed = now - rateBuckets[slot].lastRefill;
  int tokensToAdd = elapsed / 1000 * RATE_LIMIT_REFILL_RATE;
  if (tokensToAdd > 0) {
    rateBuckets[slot].tokens = min(rateBuckets[slot].tokens + tokensToAdd, RATE_LIMIT_MAX_TOKENS);
    rateBuckets[slot].lastRefill = now;
  }

  rateBuckets[slot].lastRequest = now;

  // Check if request is allowed
  if (rateBuckets[slot].tokens > 0) {
    rateBuckets[slot].tokens--;
    return true;
  }

  return false; // Rate limited
}

void cleanupRateBuckets() {
  unsigned long now = millis();
  if (now - lastRateCleanup < RATE_LIMIT_CLEANUP_MS) return;
  lastRateCleanup = now;

  for (int i = 0; i < RATE_LIMIT_BUCKET_MAX; i++) {
    // Remove buckets inactive for > 5 minutes
    if (rateBuckets[i].ip.length() > 0 && (now - rateBuckets[i].lastRequest) > 300000UL) {
      rateBuckets[i].ip = "";
      rateBuckets[i].tokens = 0;
    }
  }
}

// ============================================================
// WebSocket Integration (Phase 10.2) — replaces SSE
// ============================================================

// ============================================================
// Route Registration
// ============================================================
void registerHandlers(WebServer &server, Pet &pet) {
  // g_server macro points to APP_STATE.server (set in setup())

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

  // Phase 6: Buzzer melody configuration
  server.on("/melodies",        HTTP_GET,  handleGetMelodyConfig);
  server.on("/melodies/config", HTTP_POST, handleSetMelodyConfig);

  // Phase 5: WiFi Manager routes
  registerWiFiRoutes(server);

  // Phase 5: OTA routes
  registerOTARoutes(server);

  // Phase 5: Multi-Pet routes
  server.on("/pets",         HTTP_GET,  handleGetPets);
  server.on("/pets/create",  HTTP_POST, handleCreatePet);
  server.on("/pets/switch",  HTTP_POST, handleSwitchPet);
  server.on("/pets/delete",  HTTP_POST, handleDeletePet);

  // Phase 5: Stats route
  server.on("/stats",        HTTP_GET,  handleGetStats);

  // Phase 5: Notifications route
  server.on("/notifications", HTTP_GET, handleGetNotifications);

  // Phase 7.5: Mood & Scheduled Feeding routes
  server.on("/mood",              HTTP_GET,  handleGetMood);
  server.on("/scheduled-feed",    HTTP_GET,  handleGetScheduledFeed);
  server.on("/scheduled-feed",    HTTP_POST, handleSetScheduledFeed);

  // Phase 7.5: IR Remote routes
#ifndef DISABLE_IR_REMOTE
  server.on("/ir/status",  HTTP_GET,  handleGetIRStatus);
  server.on("/ir/config",  HTTP_POST, handleSetIRRemote);
#endif

  // Phase 7.5: MQTT routes
#ifndef DISABLE_MQTT
  registerMQTTRoutes(server);
#endif

  // Phase 7.5: OTA Delta routes
#ifndef DISABLE_OTA_DELTA
  registerDeltaRoutes(server);
#endif

  // Phase 10.3: i18n routes
  server.on("/api/settings/lang", HTTP_GET, handleGetLanguage);
  server.on("/api/settings/lang", HTTP_POST, handleSetLanguage);
  server.on("/api/locales/current", HTTP_GET, handleGetLocale);

  // Phase 10.4: Factory reset route
  server.on("/api/settings/factory-reset", HTTP_POST, handleFactoryReset);
}

// ============================================================
// Utility — Phase 10.6: Structured error responses
// ============================================================

// Include ErrorCode.h for error code macros
#include "ErrorCode.h"

// Basic success/error response
static void sendJsonResponse(bool success, const String &message = "") {
  if (!g_server) {
    Serial.println("ERROR: g_server is null in sendJsonResponse");
    return;
  }
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["success"] = success;
  if (message.length() > 0) {
    jsonDoc["message"] = message;
  }
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

// Structured error response with error code (Phase 10.6)
static void sendErrorResponse(const char* errorCode, const String &message, int httpCode = 400) {
  if (!g_server) {
    Serial.printf("ERROR: g_server null in sendErrorResponse (%s)\n", errorCode);
    return;
  }
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["success"] = false;
  jsonDoc["error"] = errorCode;
  if (message.length() > 0) {
    jsonDoc["message"] = message;
  }
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(httpCode, "application/json", response);
}

// ============================================================
// Route Handlers
// ============================================================
void handleRoot() {
  if (!g_server) { Serial.println("ERROR: g_server null in handleRoot"); return; }

  // Phase 7.3: Try gzip-compressed version first
  File file = SPIFFS.open("/index.html.gz", "r");
  if (file) {
    g_server->sendHeader("Content-Encoding", "gzip");
    g_server->sendHeader("Cache-Control", "max-age=86400");
    g_server->streamFile(file, "text/html");
    file.close();
    return;
  }

  // Fallback to uncompressed
  file = SPIFFS.open("/index.html", "r");
  if (!file) {
    g_server->send(500, "text/plain", "Failed to open index.html");
    return;
  }
  g_server->sendHeader("Cache-Control", "max-age=3600");
  g_server->streamFile(file, "text/html");
  file.close();
}

void handleGetPet() {
  AppState& state = APP_STATE;
  if (!g_server) {
    Serial.println("ERROR: null pointer in handleGetPet");
    return;
  }
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["hunger"]      = state.pet.hunger;
  jsonDoc["happiness"]   = state.pet.happiness;
  jsonDoc["health"]      = state.pet.health;
  jsonDoc["energy"]      = state.pet.energy;
  jsonDoc["cleanliness"] = state.pet.cleanliness;
  jsonDoc["age"]         = state.pet.age;
  jsonDoc["isAlive"]     = state.pet.isAlive;
  jsonDoc["state"]       = state.pet.state;

  // Phase 2: new fields
  const char* stageStr = "baby";
  switch (state.pet.stage) {
    case BABY:  stageStr = "baby";  break;
    case CHILD: stageStr = "child"; break;
    case ADULT: stageStr = "adult"; break;
    case ELDER: stageStr = "elder"; break;
  }
  jsonDoc["stage"]          = stageStr;
  jsonDoc["isNight"]        = state.pet.isNight;
  jsonDoc["virtualMinutes"] = state.pet.virtualMinutes;

  // Phase 3: name + sound + type
  jsonDoc["name"]         = state.pet.name;
  jsonDoc["soundEnabled"] = state.pet.soundEnabled;
  const char* typeStr = "blob";
  switch (state.pet.type) {
    case BLOB: typeStr = "blob"; break;
    case CAT:  typeStr = "cat";  break;
    case DOG:  typeStr = "dog";  break;
  }
  jsonDoc["type"] = typeStr;

  // Phase 3: achievements array
  String achJson = getAchievementsJson(state.pet);
  StaticJsonDocument<512> achDoc;
  deserializeJson(achDoc, achJson);
  jsonDoc["achievements"] = achDoc["achievements"];

  // Phase 4: new fields
  jsonDoc["isDying"]      = state.pet.isDying;
  jsonDoc["dyingStartTime"] = state.pet.dyingStartTime;
  jsonDoc["isEvolving"]   = state.pet.isEvolving;
  jsonDoc["weather"]      = getWeatherName(state.pet.weather);
  jsonDoc["musicEnabled"] = state.pet.musicEnabled;
  jsonDoc["difficulty"]   = getDifficultyName(state.pet.difficulty);
  jsonDoc["gameCooldown"] = state.pet.gameCooldown;
  jsonDoc["activeGame"]   = state.pet.activeGame;

  // Phase 7.5: mood & personality
  jsonDoc["mood"]               = state.pet.mood;
  jsonDoc["moodName"]           = getMoodName(state.pet.mood);
  jsonDoc["moodEmoji"]          = getMoodEmoji(state.pet.mood);
  jsonDoc["personalityCheerful"]  = state.pet.personalityCheerful;
  jsonDoc["personalityEnergetic"] = state.pet.personalityEnergetic;
  jsonDoc["personalityHungry"]    = state.pet.personalityHungry;

  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

void handleFeed() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  // Phase 6: Clamp stat before action to prevent overflow
  state.pet.hunger = constrain(state.pet.hunger, STAT_MIN, STAT_MAX);
  feedPet(state.pet);
  savePetData(state.pet);
  checkAchievements(state.pet);
  saveAchievements(state.pet);
  sendJsonResponse(true);
  // Phase 10.2: Broadcast state change via WebSocket
  webSocketBroadcastNotification("feed", "Fed " + state.pet.name);
}

void handlePlay() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  if (state.pet.energy < PLAY_ENERGY_MIN) { sendJsonResponse(false, "Pet is too tired to play"); return; }
  playPet(state.pet);
  savePetData(state.pet);
  checkAchievements(state.pet);
  saveAchievements(state.pet);
  sendJsonResponse(true);
  // Phase 10.2: Broadcast state change via WebSocket
  webSocketBroadcastNotification("play", "Played with " + state.pet.name);
}

void handleClean() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  cleanPet(state.pet);
  savePetData(state.pet);
  sendJsonResponse(true);
  // Phase 10.2: Broadcast state change via WebSocket
  webSocketBroadcastNotification("clean", "Cleaned " + state.pet.name);
}

void handleSleep() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  sleepPet(state.pet);
  savePetData(state.pet);
  sendJsonResponse(true);
  // Phase 10.2: Broadcast state change via WebSocket
  webSocketBroadcastNotification("sleep", state.pet.name + " is now sleeping");
}

void handleHeal() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive && !state.pet.isDying) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  healPet(state.pet);
  savePetData(state.pet);
  sendJsonResponse(true);
  // Phase 10.2: Broadcast state change via WebSocket
  webSocketBroadcastNotification("heal", "Healed " + state.pet.name);
}

void handleReset() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  initPet(state.pet);
  // Reset achievement tracking
  state.pet.feedCount     = 0;
  state.pet.playCount     = 0;
  state.pet.hasBeenNamed  = false;
  state.pet.elderAchieved = false;
  savePetData(state.pet);
  saveAchievements(state.pet);
  sendJsonResponse(true);
  // Phase 10.2: Broadcast state change via WebSocket
  webSocketBroadcastNotification("reset", "Pet has been reset");
}

void handleUpdate() {
  handleGetPet();
}

void handleGameStart() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  if (state.pet.gameCooldown > 0) { sendJsonResponse(false, "Game on cooldown: " + String(state.pet.gameCooldown) + "s"); return; }
  if (state.pet.energy < 10) { sendJsonResponse(false, "Pet is too tired to play"); return; }

  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, body);
  if (error) { sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON body"); return; }

  // Phase 6: Validate game type is an integer in valid range
  if (!jsonDoc["game"].is<int>()) { sendJsonResponse(false, "Game type must be an integer"); return; }
  int gameType = jsonDoc["game"].as<int>();
  if (gameType < 1 || gameType > 3) { sendJsonResponse(false, "Invalid game type — must be 1, 2, or 3"); return; }
  startGame(state.pet, gameType);
  savePetData(state.pet);

  String gameState = getGameStateJSON(state.pet);
  StaticJsonDocument<256> resp;
  resp["success"] = true;
  StaticJsonDocument<512> gs;
  deserializeJson(gs, gameState);
  resp["gameState"] = gs;
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

void handleGameAction() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  if (state.pet.activeGame == 0) { sendErrorResponse(ERR_PARAM_INVALID, "No active game"); return; }

  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, body);
  int input = jsonDoc["input"] | -1;

  if (state.pet.activeGame == 1) {
    checkMemoryInput(state.pet, input);
  } else if (state.pet.activeGame == 2) {
    // Catch game: input is X coordinate, check proximity to target
    int targetX = state.pet.catchTargetX;
    int targetY = state.pet.catchTargetY;
    int dist = abs(input - targetX);
    if (dist < 15) {
      state.pet.gameScore++;
    }
    updateCatchTarget(state.pet);
  } else if (state.pet.activeGame == 3) {
    // Quiz game
    int answer = jsonDoc["answer"] | -1;
    // Correct answer is always 0 for now (simplified)
    if (answer == 0) {
      state.pet.gameScore++;
    }
    state.pet.gameRound++;
    if (state.pet.gameRound >= 5) {
      endGame(state.pet, state.pet.gameScore >= 3);
    }
  }

  savePetData(state.pet);
  String gameState = getGameStateJSON(state.pet);
  StaticJsonDocument<256> resp;
  resp["success"] = true;
  StaticJsonDocument<512> gs;
  deserializeJson(gs, gameState);
  resp["gameState"] = gs;
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

void handleGameState() {
  AppState& state = APP_STATE;
  String gameState = getGameStateJSON(state.pet);
  g_server->send(200, "application/json", gameState);
}

void handleSetMusic() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }
  state.pet.musicEnabled = !state.pet.musicEnabled;
  savePetData(state.pet);
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["success"] = true;
  jsonDoc["musicEnabled"] = state.pet.musicEnabled;
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

void handleSetDifficulty() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, body);
  String diffStr = jsonDoc["difficulty"] | "normal";
  diffStr.toLowerCase();
  diffStr.trim();

  // Phase 6: Validate difficulty is one of the expected values
  if (diffStr == "easy") state.pet.difficulty = 0;
  else if (diffStr == "hard") state.pet.difficulty = 2;
  else if (diffStr == "normal") state.pet.difficulty = 1;
  else {
    sendErrorResponse(ERR_PARAM_INVALID, "Invalid difficulty: use easy, normal, or hard");
    return;
  }

  savePetData(state.pet);
  StaticJsonDocument<256> resp;
  resp["success"] = true;
  resp["difficulty"] = getDifficultyName(state.pet.difficulty);
  String response;
  serializeJson(resp, response);
  g_server->send(200, "application/json", response);
}

void handleRevive() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (state.pet.isAlive) { sendErrorResponse(ERR_PET_SAVE_FAIL, "Pet is already alive"); return; }
  if (state.pet.isDying) { sendJsonResponse(false, "Window has closed — pet is dead"); return; }
  if (!canRevive(state.pet)) {
    unsigned long remaining = (300000 - (millis() - state.pet.lastReviveTime)) / 1000;
    sendJsonResponse(false, "Revive on cooldown: " + String(remaining) + "s remaining");
    return;
  }
  revivePet(state.pet);
  savePetData(state.pet);
  sendJsonResponse(true);
}

void handleSetType() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }

  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, body);

  if (error) {
    sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON");
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
    sendErrorResponse(ERR_PET_INVALID_TYPE, "Invalid pet type — must be blob, cat, or dog");
    return;
  }

  // Reset pet with new type
  state.pet.type = newType;
  initPet(state.pet);
  state.pet.type = newType; // preserve type after reset
  savePetData(state.pet);
  sendJsonResponse(true);
}

void handleAchievements() {
  AppState& state = APP_STATE;
  String achJson = getAchievementsJson(state.pet);
  g_server->send(200, "application/json", achJson);
}

void handleMute() {
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }

  state.pet.soundEnabled = !state.pet.soundEnabled;
  savePetData(state.pet);

  StaticJsonDocument<256> jsonDoc;
  jsonDoc["success"] = true;
  jsonDoc["soundEnabled"] = state.pet.soundEnabled;
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

void handleSetName() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  if (!state.pet.isAlive) { sendErrorResponse(ERR_PET_DEAD, "Pet is not alive"); return; }

  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, body);

  if (error) {
    sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON");
    return;
  }

  String newName = jsonDoc["name"] | "";
  newName.trim();
  if (newName.length() == 0) {
    sendErrorResponse(ERR_PET_INVALID_NAME, "Name cannot be empty");
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
    sendErrorResponse(ERR_PET_INVALID_NAME, "Name contains only invalid characters");
    return;
  }

  state.pet.name = sanitized;
  state.pet.hasBeenNamed = true;
  savePetData(state.pet);
  saveAchievements(state.pet);
  checkAchievements(state.pet);
  sendJsonResponse(true);
}

// ============================================================
// Phase 6.3: Buzzer Melody Configuration Endpoints
// ============================================================

void handleGetMelodyConfig() {
  if (!g_server) return;
  String json = getMelodyConfigJson();
  g_server->send(200, "application/json", json);
}

void handleSetMelodyConfig() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  String body = g_server->arg("plain");
  setMelodyConfigFromJson(body);
  sendJsonResponse(true);
}

// ============================================================
// Phase 5: Multi-Pet Endpoints
// ============================================================

void handleGetPets() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String json = getMultiPetJson(state.multiPet);
  g_server->send(200, "application/json", json);
}

void handleCreatePet() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, body);
  if (err) { sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON"); return; }
  String name = jsonDoc["name"] | "";
  int type = jsonDoc["type"] | 0;
  PetType pt = (PetType)constrain(type, 0, 2);
  int slot = createPet(state.multiPet, name, pt);
  if (slot >= 0) {
    StaticJsonDocument<256> resp;
    resp["success"] = true;
    resp["slot"] = slot;
    String r; serializeJson(resp, r);
    g_server->send(200, "application/json", r);
  } else {
    sendErrorResponse(ERR_PET_NO_SLOTS, "No free pet slots available");
  }
}

void handleSwitchPet() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, body);
  if (err) { sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON"); return; }
  int slot = jsonDoc["slot"] | -1;
  if (switchPet(state.multiPet, slot)) {
    sendJsonResponse(true);
  } else {
    sendErrorResponse(ERR_PET_INVALID_SLOT, "Invalid pet slot");
  }
}

void handleDeletePet() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, body);
  if (err) { sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON"); return; }
  int slot = jsonDoc["slot"] | -1;
  if (deletePet(state.multiPet, slot)) {
    sendJsonResponse(true);
  } else {
    sendErrorResponse(ERR_PET_INVALID_SLOT, "Cannot delete pet");
  }
}

// ============================================================
// Phase 5: Stats & Notifications Endpoints
// ============================================================

void handleGetStats() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String json = getStatsJson(state.stats);
  g_server->send(200, "application/json", json);
}

void handleGetNotifications() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String json = getNotificationsJson(state.multiPet.activePetIndex);
  g_server->send(200, "application/json", json);
}

// ============================================================
// Phase 7.5: Mood & Scheduled Feeding Endpoints
// ============================================================

void handleGetMood() {
  if (!g_server) {
    return;
  }
  AppState& state = APP_STATE;
  String result = "{";
  result += "\"mood\":" + String(state.pet.mood) + ",";
  result += "\"moodName\":\"" + getMoodName(state.pet.mood) + "\",";
  result += "\"moodEmoji\":\"" + getMoodEmoji(state.pet.mood) + "\",";
  result += "\"personalityCheerful\":" + String(state.pet.personalityCheerful) + ",";
  result += "\"personalityEnergetic\":" + String(state.pet.personalityEnergetic) + ",";
  result += "\"personalityHungry\":" + String(state.pet.personalityHungry);
  result += "}";
  g_server->send(200, "application/json", result);
}

void handleGetScheduledFeed() {
  if (!g_server) {
    return;
  }
  AppState& state = APP_STATE;
  g_server->send(200, "application/json", getScheduledFeedJson(state.pet));
}

void handleSetScheduledFeed() {
  if (!g_server) {
    return;
  }
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, body);
  if (err) { sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON"); return; }

  if (jsonDoc["enabled"].is<bool>()) {
    state.pet.scheduledFeedEnabled = jsonDoc["enabled"].as<bool>();
  }
  if (jsonDoc["intervalHours"].is<int>()) {
    int hours = jsonDoc["intervalHours"].as<int>();
    state.pet.scheduledFeedInterval = constrain(hours, 1, 24) * 3600000UL;
  }
  if (jsonDoc["amount"].is<int>()) {
    state.pet.scheduledFeedAmount = constrain(jsonDoc["amount"].as<int>(), 5, 50);
  }

  state.pet.lastScheduledFeed = millis(); // Reset timer on config change
  savePetData(state.pet);
  sendJsonResponse(true);
}

// ============================================================
// Phase 7.5: IR Remote Status Endpoint
// ============================================================
#ifndef DISABLE_IR_REMOTE
void handleGetIRStatus() {
  if (!g_server) return;
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["irEnabled"] = isIRRemoteEnabled();
  jsonDoc["lastButton"] = (int)getLastIRButton();
  jsonDoc["lastRawCode"] = getLastIRRawCode();
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

void handleSetIRRemote() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, body);
  if (err) { sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON"); return; }

  if (jsonDoc["enabled"].is<bool>()) {
    enableIRRemote(jsonDoc["enabled"].as<bool>());
  }
  sendJsonResponse(true);
}
#endif // !DISABLE_IR_REMOTE

// ============================================================
// Phase 10.3: i18n Language Endpoints
// ============================================================

void handleGetLanguage() {
  if (!g_server) return;
  AppState &state = APP_STATE;
  Language lang = getCurrentLanguage();
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["success"] = true;
  jsonDoc["language"] = getLanguageCode(lang);
  String response;
  serializeJson(jsonDoc, response);
  g_server->send(200, "application/json", response);
}

void handleSetLanguage() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState &state = APP_STATE;
  String body = g_server->arg("plain");
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, body);
  if (err) { sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON"); return; }

  String langCode = jsonDoc["language"] | "en";
  Language lang = parseLanguage(langCode);
  setCurrentLanguage(lang);
  sendJsonResponse(true);
}

void handleGetLocale() {
  if (!g_server) return;
  AppState &state = APP_STATE;
  Language lang = getCurrentLanguage();
  String locale = loadLocale(lang);
  if (locale.length() == 0) {
    locale = "{}";
  }
  g_server->send(200, "application/json", locale);
}

// ============================================================
// Phase 10.4: Factory Reset Handler
// Software trigger: POST /api/settings/factory-reset
// Wipes SPIFFS, resets WiFi credentials, restarts device
// ============================================================
void handleFactoryReset() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }

  // Send response before reset (device will restart)
  g_server->send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset initiated — device restarting\"}");

  // Give time for HTTP response to be sent
  delay(500);

  // Visual feedback: flash red 3 times
#ifndef DISABLE_RGB_LED
  flashRGBRed(3, 300, 200);
#endif

  // Show factory reset on OLED
#ifdef ENABLE_OLED
  showFactoryResetOLED();
#endif

  delay(500);

  // Wipe SPIFFS
  SPIFFS.format();

  // Reset WiFi credentials
  WiFi.disconnect(true, true);  // eraseAP = true, true

  delay(1000);

  // Restart device
  ESP.restart();
}
