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

#endif // WEBHANDLERS_H
