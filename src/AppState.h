#ifndef APPSTATE_H
#define APPSTATE_H

#include <Arduino.h>
#include <WebServer.h>
#include "Pet.h"
#include "MultiPet.h"
#include "Stats.h"

// ============================================================
// AppState — Singleton Global State Manager (Phase 10.1)
//
// Centralizes all global state into a single singleton class,
// eliminating extern global pointer spaghetti and providing
// a clean access point for all modules.
//
// Usage: AppState::getInstance().pet, AppState::getInstance().server, etc.
// ============================================================

class AppState {
public:
  // Core instances
  Pet pet;
  WebServer server;
  MultiPetState multiPet;
  GameStats stats;

  // Wake-up message tracking (moved from WebHandlers)
  bool showWakeMessage = false;
  unsigned long wakeMessageStartTime = 0;
  String previousState = "";

  // Singleton access
  static AppState& getInstance() {
    static AppState instance;
    return instance;
  }

  // Delete copy/move constructors and assignment operators
  AppState(const AppState&) = delete;
  AppState& operator=(const AppState&) = delete;
  AppState(AppState&&) = delete;
  AppState& operator=(AppState&&) = delete;

private:
  // Private constructor — use getInstance()
  AppState() : server(WEB_SERVER_PORT) {}
};

// Convenience macro for shorter access
#define APP_STATE AppState::getInstance()

#endif // APPSTATE_H
