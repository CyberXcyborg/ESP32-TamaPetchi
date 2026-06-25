#ifndef WEBHANDLERS_H
#define WEBHANDLERS_H

#include <WebServer.h>
#include "Pet.h"
#include "AppState.h"

// ============================================================
// HTTP handler registration & implementation
// These functions need a reference to the pet instance
// ============================================================

// Register all routes on the server
void registerHandlers(WebServer &server, Pet &pet);

// ============================================================
// Rate Limiting (Phase 7.2)
// ============================================================
bool checkRateLimit(const String &clientIP);
void cleanupRateBuckets();

// Server access: use APP_STATE.server directly (g_server is a convenience macro in WebHandlers.cpp)

// ============================================================
// WebSocket Support (Phase 10.2) — replaces SSE
// Note: handleWebSocketBroadcast() and webSocketBroadcastNotification()
// are declared in WebSocket.h — include that header instead of duplicating.
// ===========================================================

// Phase 5: Forward declarations
void handleGetPets();
void handleCreatePet();
void handleSwitchPet();
void handleDeletePet();
void handleGetStats();
void handleGetNotifications();

// Phase 7.5: Mood & Scheduled Feeding
void handleGetMood();
void handleGetScheduledFeed();
void handleSetScheduledFeed();

// Phase 7.5: IR Remote
#ifndef DISABLE_IR_REMOTE
void handleGetIRStatus();
void handleSetIRRemote();
#endif

// Phase 7.5: MQTT
#ifndef DISABLE_MQTT
void registerMQTTRoutes(WebServer &server);
#endif

// Phase 7.5: OTA Delta
#ifndef DISABLE_OTA_DELTA
void registerDeltaRoutes(WebServer &server);
#endif

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

// Phase 12.5 / 15.3: Backup & Restore
void handleGetBackup();
void handlePostRestore();
void handleVerifyBackup();

// Phase 13.3: Community Features
void handleGetGallery();
void handleGetLeaderboard();
void handleShareProfile();
void handleImportProfile();

// Phase 15.4: Advanced Achievement endpoints
void handleGetCategoryProgress();
void handleGetAchievementRewards();

// Phase 15.6: Community revision — pet snapshots, comparison
void handleGetPetSnapshot();
void handleComparePets();

// Phase 16.1: Pet AI
void handleGetPetAIStatus();
void handleGetPetAIMemory();

// Phase 16.5: Data Export & Import
void handleExportFull();
void handleImportSettings();

// Phase 17.2: Voice Control
void handleGetVoiceStatus();
void handlePostVoiceCommand();

// Phase 17.3: Advanced Analytics
void handleGetCarePatterns();
void handleGetHealthPredictions();
void handleGetCareReport();

// Phase 17.4: Plugin System
void handleGetPlugins();
void handleEnablePlugin();
void handleDisablePlugin();
void handleUploadPlugin();
void handleDeletePlugin();

// Phase 20.4: LVGL-compatible API endpoints
void handleGetSprites();
void handleGetScreenState();

#endif // WEBHANDLERS_H
