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

// Getter/setter for web handlers to access server (set during registration)
WebServer* getServer();

// ============================================================
// SSE Support (Phase 6.2)
// ============================================================
void beginSSE();
void broadcastSSE(const String &data);
void handleSSEClients();
void handleSetMelodyConfig();

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

#endif // WEBHANDLERS_H
