#ifndef WEBHANDLERS_H
#define WEBHANDLERS_H

#include <WebServer.h>
#include "Pet.h"

// ============================================================
// HTTP handler registration & implementation
// These functions need a reference to the pet instance
// ============================================================

// Register all routes on the server
void registerHandlers(WebServer &server, Pet &pet);

// Global state for wake-up messages (owned by WebHandlers)
extern bool showWakeMessage;
extern unsigned long wakeMessageStartTime;
extern String previousState;

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

#endif // WEBHANDLERS_H
