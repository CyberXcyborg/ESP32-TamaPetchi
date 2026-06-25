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
#include "OTARollback.h"
#include "SoundPack.h"
#include "PetTrade.h"
#include "Community.h"   // Phase 13.3: Community features
#include "Provisioning.h" // Phase 13.4: Manufacturing & Provisioning
#include "Backup.h"      // Phase 15.3: Backup & Restore
#include "RGB_LED.h"    // Phase 10.4: for flashRGBRed()
#include "PetAI.h"      // Phase 16.1: Pet AI
#include "VoiceControl.h" // Phase 17.2: Voice Control
#include "Analytics.h"    // Phase 17.3: Advanced Analytics
#include "Plugin.h"       // Phase 17.4: Plugin System
#ifdef ENABLE_OLED
#include "OLED.h"       // Phase 10.4: for showFactoryResetOLED()
#endif
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

#include "ErrorCode.h"
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

static void sendJsonResponse(bool success, const String &message = "");
static void sendErrorResponse(const char* errorCode, const String &message, int httpCode = 400);
// Phase 12.1: Achievements 2.0
void handleGetAchievementsProgress();

// Phase 12.2: Pet Lineage
void handleGetLineage();

// Phase 12.3: Analytics & Export
void handleGetStatsTrends();
void handleExportCsv();
void handleExportJson();

// Phase 12.4: Accessibility
void handleGetAccessibility();
void handleSetAccessibility();

// Phase 12.5: Backup & Restore
void handleGetBackup();
void handlePostRestore();

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

  // Phase 11.1: OTA rollback routes
  registerRollbackRoutes();

  // Phase 11.3: Sound pack routes
  registerSoundPackRoutes();

  // Phase 11.4: Pet trade routes
  registerTradeRoutes();

  // Phase 12.1: Achievements 2.0 progress
  server.on("/api/achievements/progress", HTTP_GET, handleGetAchievementsProgress);

  // Phase 12.2: Pet lineage
  server.on("/api/pets/lineage", HTTP_GET, handleGetLineage);

  // Phase 12.3: Analytics & export
  server.on("/api/stats/trends", HTTP_GET, handleGetStatsTrends);
  server.on("/api/export/csv", HTTP_GET, handleExportCsv);
  server.on("/api/export/json", HTTP_GET, handleExportJson);

  // Phase 12.4: Accessibility
  server.on("/api/settings/accessibility", HTTP_GET, handleGetAccessibility);
  server.on("/api/settings/accessibility", HTTP_POST, handleSetAccessibility);

  // Phase 12.5 / 15.3: Backup & restore
  server.on("/api/backup", HTTP_GET, handleGetBackup);
  server.on("/api/restore", HTTP_POST, handlePostRestore);
  server.on("/api/backup/verify", HTTP_POST, handleVerifyBackup);

  // Phase 13.3: Community features
  server.on("/api/community/gallery", HTTP_GET, handleGetGallery);
  server.on("/api/community/leaderboard", HTTP_GET, handleGetLeaderboard);
  server.on("/api/community/share", HTTP_POST, handleShareProfile);
  server.on("/api/community/import", HTTP_POST, handleImportProfile);

  // Phase 15.4: Advanced achievement endpoints
  server.on("/api/achievements/categories", HTTP_GET, handleGetCategoryProgress);
  server.on("/api/achievements/rewards", HTTP_GET, handleGetAchievementRewards);

  // Phase 15.6: Pet snapshot and comparison
  server.on("/api/pets/snapshot", HTTP_GET, handleGetPetSnapshot);
  server.on("/api/pets/compare", HTTP_GET, handleComparePets);

  // Phase 16.1: Pet AI routes
  server.on("/api/pets/ai/status", HTTP_GET, handleGetPetAIStatus);
  server.on("/api/pets/ai/memory", HTTP_GET, handleGetPetAIMemory);

  // Phase 16.2: HA config endpoint
  registerHARoutes(server);

  // Phase 16.5: Data export & import
  server.on("/api/export/full", HTTP_GET, handleExportFull);
  server.on("/api/import/settings", HTTP_POST, handleImportSettings);

  // Phase 24.2: Data Export API routes
  server.on("/api/export/create", HTTP_GET, handleExportCreate);
  server.on("/api/export/list", HTTP_GET, handleExportList);
  server.on("/api/export/download", HTTP_GET, handleExportDownload);
  server.on("/api/export/delete", HTTP_POST, handleExportDelete);

  // Phase 13.4: Provisioning routes
  registerProvisioningRoutes();

  // Phase 17.2: Voice Control routes
  server.on("/api/voice/status", HTTP_GET, handleGetVoiceStatus);
  server.on("/api/voice/command", HTTP_POST, handlePostVoiceCommand);

  // Phase 17.3: Advanced Analytics routes
  server.on("/api/analytics/care-patterns", HTTP_GET, handleGetCarePatterns);
  server.on("/api/analytics/predictions", HTTP_GET, handleGetHealthPredictions);
  server.on("/api/analytics/reports/weekly", HTTP_GET, handleGetCareReport);
  server.on("/api/analytics/reports/monthly", HTTP_GET, handleGetCareReport);

  // Phase 17.4: Plugin System routes
  server.on("/api/plugins", HTTP_GET, handleGetPlugins);
  server.on("/api/plugins/upload", HTTP_POST, handleUploadPlugin);
  server.on("/api/plugins/enable", HTTP_POST, handleEnablePlugin);
  server.on("/api/plugins/disable", HTTP_POST, handleDisablePlugin);
  server.on("/api/plugins/delete", HTTP_POST, handleDeletePlugin);

  // Phase 20.4: LVGL-compatible API endpoints
  server.on("/api/sprites", HTTP_GET, handleGetSprites);
  server.on("/api/screen", HTTP_GET, handleGetScreenState);
}

// ============================================================
// Phase 12.1: Achievements 2.0 Handlers
// ============================================================

void handleGetAchievementsProgress() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  g_server->send(200, "application/json", getAchievementsProgressJson(state.pet));
}

// ============================================================
// Phase 12.2: Pet Lineage Handler
// ============================================================

void handleGetLineage() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  g_server->send(200, "application/json", getLineageJson(state.pet));
}

// ============================================================
// Phase 12.3: Analytics & Export Handlers
// ============================================================

void handleGetStatsTrends() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String range = g_server->arg("range");
  if (range.length() == 0) range = "7d";
  g_server->send(200, "application/json", getAnalyticsTrendsJson(state.pet, range));
}

void handleExportCsv() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String range = g_server->arg("range");
  if (range.length() == 0) range = "7d";
  String csv = getAnalyticsCsv(state.pet, range);
  g_server->sendHeader("Content-Disposition", "attachment; filename=\"tamapetchi_export.csv\"");
  g_server->send(200, "text/csv", csv);
}

void handleExportJson() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String range = g_server->arg("range");
  if (range.length() == 0) range = "7d";
  String json = getAnalyticsJson(state.pet, range);
  g_server->sendHeader("Content-Disposition", "attachment; filename=\"tamapetchi_export.json\"");
  g_server->send(200, "application/json", json);
}

// ============================================================
// Phase 12.4: Accessibility Handlers
// ============================================================

void handleGetAccessibility() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  g_server->send(200, "application/json", getAccessibilityJson(state.pet));
}

void handleSetAccessibility() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");
  setAccessibilityFromJson(state.pet, body);
  savePetData(state.pet);
  sendJsonResponse(true);
}

// ============================================================
// Phase 12.5 / 15.3: Backup & Restore Handlers
// ============================================================

void handleGetBackup() {
  if (!g_server) return;
  AppState& state = APP_STATE;

  String backup = createBackupJson(state.pet);

  g_server->sendHeader("Content-Disposition", "attachment; filename=\"tamapetchi_backup.json\"");
  g_server->sendHeader("Cache-Control", "no-cache");
  g_server->send(200, "application/json", backup);
}

void handlePostRestore() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests — slow down", 429);
    return;
  }
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");

  // Use Backup module for validation and restore
  int result = restoreBackupJson(body, state.pet);
  if (result != ERR_OK) {
    const char *errStr = "backup_restore_fail";
    if (result == ERR_BACKUP_VERSION_MISSING) errStr = ERR_STR_BACKUP_VERSION_MISSING;
    else if (result == ERR_BACKUP_NO_PET) errStr = ERR_STR_BACKUP_NO_PET;
    else if (result == ERR_BACKUP_NO_CHECKSUM) errStr = ERR_STR_BACKUP_NO_CHECKSUM;
    else if (result == ERR_BACKUP_CHECKSUM_MISMATCH) errStr = ERR_STR_BACKUP_CHECKSUM_MISMATCH;
    sendErrorResponse(errStr, "Backup restore failed — invalid or corrupt data");
    return;
  }

  // Persist restored data
  savePetData(state.pet);
  saveAchievements(state.pet);

  g_server->send(200, "application/json", "{\"success\":true,\"message\":\"Backup restored successfully\"}");
}

void handleVerifyBackup() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");

  int result = verifyBackupJson(body);
  if (result == ERR_OK) {
    String ver = getBackupVersion(body);
    g_server->send(200, "application/json", "{\"success\":true,\"valid\":true,\"version\":\"" + ver + "\"}");
  } else {
    const char *errStr = "backup_verify_fail";
    if (result == ERR_INT_JSON_PARSE_FAIL) errStr = ERR_JSON_PARSE_FAIL;
    else if (result == ERR_BACKUP_VERSION_MISSING) errStr = ERR_STR_BACKUP_VERSION_MISSING;
    else if (result == ERR_BACKUP_NO_PET) errStr = ERR_STR_BACKUP_NO_PET;
    else if (result == ERR_BACKUP_NO_CHECKSUM) errStr = ERR_STR_BACKUP_NO_CHECKSUM;
    else if (result == ERR_BACKUP_CHECKSUM_MISMATCH) errStr = ERR_STR_BACKUP_CHECKSUM_MISMATCH;
    sendErrorResponse(errStr, "Backup verification failed");
  }
}

// ============================================================
// Utility — Phase 10.6: Structured error responses
// ============================================================

// Include ErrorCode.h for error code macros
#include "ErrorCode.h"

// Basic success/error response
static void sendJsonResponse(bool success, const String &message) {
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
static void sendErrorResponse(const char* errorCode, const String &message, int httpCode) {
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

  // Phase 11.5: ETag support for caching
  String etag = "";
  File etagFile = SPIFFS.open("/index.html.gz", "r");
  if (!etagFile) etagFile = SPIFFS.open("/index.html", "r");
  if (etagFile) {
    etag = String("\"") + String(etagFile.size()) + "\"";
    etagFile.close();
    if (g_server->header("If-None-Match") == etag) {
      g_server->send(304, "text/plain", "");
      return;
    }
  }

  // Phase 7.3: Try gzip-compressed version first
  File file = SPIFFS.open("/index.html.gz", "r");
  if (file) {
    g_server->sendHeader("Content-Encoding", "gzip");
    g_server->sendHeader("Cache-Control", "max-age=86400");
    if (etag.length() > 0) g_server->sendHeader("ETag", etag);
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
  if (etag.length() > 0) g_server->sendHeader("ETag", etag);
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

// ============================================================
// Phase 13.3: Community Features & Pet Sharing
// ============================================================

void handleGetGallery() {
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    g_server->send(429, "application/json", "{\"error\":\"rate_limit\"}");
    return;
  }
  g_server->send(200, "application/json", getGalleryJson());
}

void handleGetLeaderboard() {
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    g_server->send(429, "application/json", "{\"error\":\"rate_limit\"}");
    return;
  }
  LeaderboardSort sort = SORT_ACHIEVEMENTS;
  String sortParam = g_server->arg("sort");
  if (sortParam == "age") sort = SORT_AGE;
  else if (sortParam == "generation") sort = SORT_GENERATION;
  int limit = 10;
  String limitParam = g_server->arg("limit");
  if (limitParam.length() > 0) limit = limitParam.toInt();
  if (limit < 1) limit = 1;
  if (limit > 50) limit = 50;
  g_server->send(200, "application/json", getLeaderboardJson(sort, limit));
}

void handleShareProfile() {
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    g_server->send(429, "application/json", "{\"error\":\"rate_limit\"}");
    return;
  }
  sharePetProfile(APP_STATE.pet);
  StaticJsonDocument<256> resp;
  resp["success"] = true;
  resp["message"] = "Pet profile shared to community gallery";
  resp["profile"] = getShareableProfileJson(APP_STATE.pet);
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

void handleImportProfile() {
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    g_server->send(429, "application/json", "{\"error\":\"rate_limit\"}");
    return;
  }
  String body = g_server->arg("plain");
  if (body.length() == 0) {
    body = g_server->arg("body");
  }
  if (body.length() == 0) {
    g_server->send(400, "application/json", "{\"success\":false,\"error\":\"missing_body\"}");
    return;
  }
  // Import creates a view-only card for the gallery
  PetCard card;
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, body)) {
    g_server->send(400, "application/json", "{\"success\":false,\"error\":\"invalid_json\"}");
    return;
  }
  card.deviceId = doc["deviceId"] | "unknown";
  card.petName = doc["petName"] | "Unknown";
  card.petType = doc["petType"] | "blob";
  card.generation = doc["generation"] | 0;
  card.age = doc["age"] | 0;
  card.feedCount = doc["feedCount"] | 0;
  card.playCount = doc["playCount"] | 0;
  card.highScore = doc["highScore"] | 0;
  card.achievementCount = doc["achievementCount"] | 0;
  card.maxTier = doc["maxTier"] | 0;
  card.createdAt = doc["createdAt"] | 0;
  addToGallery(card);
  StaticJsonDocument<256> resp;
  resp["success"] = true;
  resp["message"] = "Pet profile imported to gallery";
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

// ============================================================
// Phase 15.4: Advanced Achievement Handlers
// ============================================================

void handleGetCategoryProgress() {
  if (!g_server) return;
  g_server->send(200, "application/json", getCategoryProgressJson());
}

void handleGetAchievementRewards() {
  if (!g_server) return;
  g_server->send(200, "application/json", getAchievementRewardsJson());
}

// ============================================================
// Phase 15.6: Pet Snapshot & Comparison Handlers
// ============================================================

void handleGetPetSnapshot() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  // Create a snapshot JSON of the current pet state for sharing/comparison
  StaticJsonDocument<1024> jsonDoc;
  jsonDoc["name"] = state.pet.name.c_str();
  jsonDoc["type"] = state.pet.type == BLOB ? "blob" : (state.pet.type == CAT ? "cat" : "dog");
  jsonDoc["stage"] = state.pet.stage == BABY ? "baby" : (state.pet.stage == CHILD ? "child" : (state.pet.stage == ADULT ? "adult" : "elder"));
  jsonDoc["age"] = state.pet.age;
  jsonDoc["generation"] = state.pet.generation;
  jsonDoc["feedCount"] = state.pet.feedCount;
  jsonDoc["playCount"] = state.pet.playCount;
  jsonDoc["highScore"] = state.pet.highScore;
  jsonDoc["hunger"] = state.pet.hunger;
  jsonDoc["happiness"] = state.pet.happiness;
  jsonDoc["health"] = state.pet.health;
  jsonDoc["energy"] = state.pet.energy;
  jsonDoc["cleanliness"] = state.pet.cleanliness;
  jsonDoc["achievementScore"] = getAchievementScore();
  jsonDoc["achievements"] = getAchievementScore(); // compatibility
  jsonDoc["score"] = calculatePetScore(state.pet);
  jsonDoc["timestamp"] = millis() / 1000;
  String result;
  serializeJson(jsonDoc, result);
  g_server->send(200, "application/json", result);
}

void handleComparePets() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  // Compare current pet with gallery entries
  // Returns comparison data: current pet vs top gallery entry
  String gallery = getGalleryJson();
  String current = getShareableProfileJson(state.pet);

  DynamicJsonDocument jsonDoc(4096);
  jsonDoc["current"] = serialized(current);
  // Find the top-scoring gallery entry for comparison
  int bestIdx = -1;
  int bestScore = -1;
  DynamicJsonDocument galDoc(8192);
  deserializeJson(galDoc, gallery.c_str());
  JsonArray arr = galDoc["gallery"].as<JsonArray>();
  int idx = 0;
  for (JsonObject entry : arr) {
    int score = entry["score"] | 0;
    if (score > bestScore) {
      bestScore = score;
      bestIdx = idx;
    }
    idx++;
  }
  if (bestIdx >= 0) {
    JsonObject topEntry = arr[bestIdx];
    JsonObject compare = jsonDoc.createNestedObject("topGallery");
    compare["petName"] = topEntry["petName"].as<String>();
    compare["score"] = bestScore;
    compare["achievementCount"] = topEntry["achievementCount"] | 0;
  } else {
    jsonDoc["topGallery"] = nullptr;
  }
  String result;
  serializeJson(jsonDoc, result);
  g_server->send(200, "application/json", result);
}

// ============================================================
// Phase 16.1: Pet AI Handlers
// ============================================================

void handleGetPetAIStatus() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String json = getPetAIStatusJson(state.pet);
  g_server->send(200, "application/json", json);
}

void handleGetPetAIMemory() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  String json = getPetAIMemoryJson(state.pet);
  g_server->send(200, "application/json", json);
}

// ============================================================
// Phase 16.5: Data Export & Import
// ============================================================

void handleExportFull() {
  if (!g_server) return;
  AppState& state = APP_STATE;

  // Build complete device state JSON (up to 8KB for full export)
  DynamicJsonDocument doc(8192);

  // Pet data (core stats)
  JsonObject petObj = doc.createNestedObject("pet");
  petObj["name"] = state.pet.name;
  petObj["type"] = state.pet.type;
  petObj["stage"] = state.pet.stage;
  petObj["age"] = state.pet.age;
  petObj["hunger"] = state.pet.hunger;
  petObj["happiness"] = state.pet.happiness;
  petObj["health"] = state.pet.health;
  petObj["energy"] = state.pet.energy;
  petObj["cleanliness"] = state.pet.cleanliness;
  petObj["isAlive"] = state.pet.isAlive;
  petObj["state"] = state.pet.state;
  petObj["mood"] = state.pet.mood;
  petObj["isDying"] = state.pet.isDying;
  petObj["isEvolving"] = state.pet.isEvolving;
  petObj["soundEnabled"] = state.pet.soundEnabled;
  petObj["musicEnabled"] = state.pet.musicEnabled;
  petObj["difficulty"] = state.pet.difficulty;
  petObj["batteryLevel"] = state.pet.batteryLevel;

  // Phase 16.1: AI data
  JsonObject aiObj = doc.createNestedObject("ai");
  aiObj["activityLevel"] = state.pet.aiActivityLevel;
  JsonObject mods = aiObj.createNestedObject("modifiers");
  mods["hungerRate"] = state.pet.aiMods.hungerRateMod;
  mods["happinessRate"] = state.pet.aiMods.happinessRateMod;
  mods["energyRate"] = state.pet.aiMods.energyRateMod;
  mods["healthRate"] = state.pet.aiMods.healthRateMod;
  JsonObject personality = aiObj.createNestedObject("personality");
  personality["cheerful"] = state.pet.personalityCheerful;
  personality["energetic"] = state.pet.personalityEnergetic;
  personality["hungry"] = state.pet.personalityHungry;

  // Lineage
  JsonObject lineage = doc.createNestedObject("lineage");
  lineage["generation"] = state.pet.generation;
  lineage["birthTimestamp"] = state.pet.birthTimestamp;
  JsonArray parents = lineage.createNestedArray("parents");
  if (state.pet.parentIds[0].length() > 0) parents.add(state.pet.parentIds[0]);
  if (state.pet.parentIds[1].length() > 0) parents.add(state.pet.parentIds[1]);

  // Analytics
  JsonObject analytics = doc.createNestedObject("analytics");
  analytics["dailyFeedCount"] = state.pet.dailyFeedCount;
  analytics["dailyPlayCount"] = state.pet.dailyPlayCount;
  analytics["dailySleepCount"] = state.pet.dailySleepCount;
  analytics["totalFeeds"] = state.pet.timesFed;
  analytics["totalPlays"] = state.pet.timesPlayed;
  analytics["totalSleeps"] = state.pet.timesSlept;
  analytics["totalCleans"] = state.pet.timesCleaned;
  analytics["totalHeals"] = state.pet.timesHealed;

  // Meta
  doc["version"] = "1.6.0";
  doc["exportTime"] = millis();

  String result;
  serializeJson(doc, result);
  g_server->send(200, "application/json", result);
}

void handleImportSettings() {
  if (!g_server) return;
  AppState& state = APP_STATE;

  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    g_server->send(429, "application/json", "{\"success\":false,\"error\":\"rate_limit\"}");
    return;
  }

  String body = g_server->arg("plain");
  if (body.length() == 0) {
    g_server->send(400, "application/json", "{\"success\":false,\"error\":\"empty_body\"}");
    return;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    g_server->send(400, "application/json", "{\"success\":false,\"error\":\"invalid_json\"}");
    return;
  }

  // Apply settings from import
  if (doc["difficulty"].is<int>()) {
    state.pet.difficulty = constrain(doc["difficulty"].as<int>(), 0, 2);
  }
  if (doc["music"].is<bool>()) {
    state.pet.musicEnabled = doc["music"].as<bool>();
  }
  if (doc["soundFx"].is<bool>()) {
    state.pet.soundEnabled = doc["soundFx"].as<bool>();
  }
  if (doc["language"].is<const char*>()) {
    Language lang = parseLanguage(String(doc["language"].as<const char*>()));
    setCurrentLanguage(lang);
  }
  if (doc["petName"].is<const char*>()) {
    String name = doc["petName"].as<String>();
    if (name.length() > 0 && name.length() <= 16) {
      state.pet.name = name;
      state.pet.hasBeenNamed = true;
    }
  }
  if (doc["petType"].is<const char*>()) {
    String t = doc["petType"].as<String>();
    if (t == "blob") state.pet.type = BLOB;
    else if (t == "cat") state.pet.type = CAT;
    else if (t == "dog") state.pet.type = DOG;
  }
  if (doc["soundPack"].is<const char*>()) {
    setActiveSoundPack(doc["soundPack"].as<String>());
  }
  if (doc["volume"].is<int>()) {
    state.pet.soundVolume = constrain(doc["volume"].as<int>(), 0, 100);
  }
  if (doc["highContrast"].is<bool>()) {
    state.pet.highContrastMode = doc["highContrast"].as<bool>();
  }
  if (doc["reducedMotion"].is<bool>()) {
    state.pet.reducedMotion = doc["reducedMotion"].as<bool>();
  }

  savePetData(state.pet);

  StaticJsonDocument<256> resp;
  resp["success"] = true;
  resp["message"] = "Settings imported successfully";
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

// ============================================================
// Phase 24.2: Data Export API Handlers
// ============================================================

void handleExportCreate() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }

  // Create a new export file
  String json = handleExportRequest();
  if (json.length() == 0) {
    sendErrorResponse("export_failed", "Failed to create export");
    return;
  }

  // Return the export as a downloadable file
  g_server->sendHeader("Content-Disposition", "attachment; filename=\"tamapetchi_export.json\"");
  g_server->sendHeader("Cache-Control", "no-cache");
  g_server->send(200, "application/json", json);
}

void handleExportList() {
  if (!g_server) return;

  // Use DataExport module to get file list
  String json = getExportFileListJson();
  g_server->send(200, "application/json", json);
}

void handleExportDownload() {
  if (!g_server) return;

  String filename = g_server->arg("file");
  if (filename.length() == 0) {
    sendErrorResponse("missing_param", "Filename required (?file=export_N.json)");
    return;
  }

  // Sanitize filename
  if (filename.indexOf("..") >= 0 || filename.indexOf("/") >= 0) {
    sendErrorResponse("invalid_filename", "Invalid filename");
    return;
  }

  String path = "/exports/" + filename;
  File file = StorageV2::open(path, "r");
  if (!file) {
    sendErrorResponse("file_not_found", "Export file not found");
    return;
  }

  g_server->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  g_server->send(200, "application/json", file.readString());
  file.close();
}

void handleExportDelete() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }

  String body = g_server->arg("plain");
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON body");
    return;
  }

  String filename = doc["filename"] | "";
  if (filename.length() == 0) {
    sendErrorResponse("missing_param", "Filename required in body");
    return;
  }

  // Sanitize filename
  if (filename.indexOf("..") >= 0 || filename.indexOf("/") >= 0) {
    sendErrorResponse("invalid_filename", "Invalid filename");
    return;
  }

  bool ok = deleteExportFile(filename);
  if (ok) {
    sendJsonResponse(true, "Export file deleted");
  } else {
    sendErrorResponse("delete_failed", "Failed to delete export file");
  }
}

// ============================================================
// Phase 17.2: Voice Control Handlers
// ============================================================

void handleGetVoiceStatus() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  g_server->send(200, "application/json", formatVoiceStatus(state.pet));
}

void handlePostVoiceCommand() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  AppState& state = APP_STATE;
  String body = g_server->arg("plain");

  // Parse command from JSON body
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    // Try plain text command
    VoiceCommand cmd = parseVoiceCommand(body.c_str());
    if (cmd == VC_NONE || cmd == VC_UNKNOWN) {
      sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid command");
      return;
    }
    bool ok = executeVoiceCommand(cmd, state.pet);
    if (ok) savePetData(state.pet);
    g_server->send(200, "application/json",
      String("{\"success\":") + (ok ? "true" : "false") +
      ",\"command\":\"" + voiceCommandToString(cmd) + "\"}");
    return;
  }

  // JSON command parsing
  VoiceCommand cmd = VC_UNKNOWN;

  // Check for "command" field (direct text)
  if (doc["command"].is<const char*>()) {
    cmd = parseVoiceCommand(doc["command"].as<const char*>());
  }
  // Check for "alexa" field (Alexa directive)
  else if (doc["alexa"].is<const char*>()) {
    cmd = parseAlexaDirective(doc["alexa"].as<const char*>());
  }
  // Check for "google" field (Google Home trait)
  else if (doc["google"].is<const char*>()) {
    const char* value = doc["value"] | "";
    cmd = parseGoogleTrait(doc["google"].as<const char*>(), value);
  }

  if (cmd == VC_NONE || cmd == VC_UNKNOWN) {
    sendErrorResponse(ERR_PARAM_INVALID, "Unknown voice command");
    return;
  }

  // Status is read-only
  if (cmd == VC_STATUS) {
    g_server->send(200, "application/json", formatVoiceStatus(state.pet));
    return;
  }

  bool ok = executeVoiceCommand(cmd, state.pet);
  if (ok) savePetData(state.pet);

  StaticJsonDocument<256> resp;
  resp["success"] = ok;
  resp["command"] = voiceCommandToString(cmd);
  if (!ok) {
    resp["message"] = "Command could not be executed (check pet state)";
  }
  String result;
  serializeJson(resp, result);
  g_server->send(200, "application/json", result);
}

// ============================================================
// Phase 17.3: Advanced Analytics Handlers
// ============================================================

void handleGetCarePatterns() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  unsigned long uptimeMin = millis() / 60000UL;
  g_server->send(200, "application/json", getCarePatternsJson(state.stats, uptimeMin));
}

void handleGetHealthPredictions() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  g_server->send(200, "application/json", getHealthPredictionsJson(state.pet, state.stats));
}

void handleGetCareReport() {
  if (!g_server) return;
  AppState& state = APP_STATE;
  // Determine period from URL path
  String path = g_server->uri();
  int periodDays = 7;  // default weekly
  if (path.indexOf("monthly") >= 0) periodDays = 30;
  g_server->send(200, "application/json", getCareReportJson(state.pet, state.stats, periodDays));
}

// ============================================================
// Phase 17.4: Plugin System Handlers
// ============================================================

void handleGetPlugins() {
  if (!g_server) return;
  g_server->send(200, "application/json", getPluginsJson());
}

void handleUploadPlugin() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  String body = g_server->arg("plain");
  if (body.length() == 0 || body.length() > 8192) {
    sendErrorResponse(ERR_PARAM_INVALID, "Invalid plugin data (max 8KB)");
    return;
  }
  bool ok = uploadPlugin(body.c_str(), body.length());
  if (ok) {
    g_server->send(200, "application/json", "{\"success\":true,\"message\":\"Plugin uploaded\"}");
  } else {
    sendErrorResponse(ERR_PARAM_INVALID, "Plugin upload failed (duplicate name or max plugins reached)");
  }
}

void handleEnablePlugin() {
  if (!g_server) return;
  String body = g_server->arg("plain");
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON");
    return;
  }
  const char* name = doc["name"] | "";
  if (enablePlugin(name)) {
    g_server->send(200, "application/json", "{\"success\":true}");
  } else {
    sendErrorResponse(ERR_PARAM_INVALID, "Plugin not found");
  }
}

void handleDisablePlugin() {
  if (!g_server) return;
  String body = g_server->arg("plain");
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON");
    return;
  }
  const char* name = doc["name"] | "";
  if (disablePlugin(name)) {
    g_server->send(200, "application/json", "{\"success\":true}");
  } else {
    sendErrorResponse(ERR_PARAM_INVALID, "Plugin not found");
  }
}

void handleDeletePlugin() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  String body = g_server->arg("plain");
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    sendErrorResponse(ERR_JSON_PARSE_FAIL, "Invalid JSON");
    return;
  }
  const char* name = doc["name"] | "";
  if (deletePlugin(name)) {
    g_server->send(200, "application/json", "{\"success\":true}");
  } else {
    sendErrorResponse(ERR_PARAM_INVALID, "Plugin not found");
  }
}

// ============================================================
// Phase 20.4: LVGL-compatible API Endpoints
// ============================================================

void handleGetSprites() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  
  StaticJsonDocument<1024> doc;
  JsonArray sprites = doc.createNestedArray("sprites");
  
  // List available sprite sets
  const char* sprite_sets[] = {"baby_idle", "baby_eat", "baby_sleep", "baby_happy",
                                 "child_idle", "child_eat", "child_play", "child_sleep", "child_sick",
                                 "adult_idle", "adult_eat", "adult_play", "adult_sleep", "adult_sick", "adult_evolve"};
  
  for (int i = 0; i < 15; i++) {
    JsonObject sprite = sprites.createNestedObject();
    sprite["name"] = sprite_sets[i];
    sprite["path"] = String("/sprites/") + sprite_sets[i] + ".spr";
    sprite["frames"] = 8;  // Default, varies per animation
    sprite["size"] = 64;   // 64x64
    sprite["format"] = "spr_v1";
  }
  
  doc["count"] = 15;
  doc["format"] = "spr_v1";
  doc["color_depth"] = 4;  // 4-bit palette
  
  String result;
  serializeJson(doc, result);
  g_server->send(200, "application/json", result);
}

void handleGetScreenState() {
  if (!g_server) return;
  if (!checkRateLimit(g_server->client().remoteIP().toString())) {
    sendErrorResponse(ERR_RATE_LIMIT, "Too many requests", 429);
    return;
  }
  
  StaticJsonDocument<512> doc;
  doc["screen"] = "MainPet";  // Current screen name
  doc["pet_state"] = APP_STATE.pet.isAlive ? "alive" : "dead";
  doc["pet_stage"] = APP_STATE.pet.stage;
  doc["pet_name"] = APP_STATE.pet.name;
  
  JsonObject stats = doc.createNestedObject("stats");
  stats["hunger"] = APP_STATE.pet.hunger;
  stats["happiness"] = APP_STATE.pet.happiness;
  stats["energy"] = APP_STATE.pet.energy;
  stats["health"] = APP_STATE.pet.health;
  
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  
  String result;
  serializeJson(doc, result);
  g_server->send(200, "application/json", result);
}
