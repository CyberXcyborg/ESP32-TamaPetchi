// ============================================================
// AppState_v2.h — Singleton Global State for v2.0
// Ported from v1.x AppState.h, adapted for ESP32-S3
// ============================================================

#ifndef APPSTATE_V2_H
#define APPSTATE_V2_H

#include <Arduino.h>
#include "Pet_v2.h"

// Global application state singleton
class AppStateV2 {
public:
    static AppStateV2& getInstance();

    // Core pet engine (replaces v1 AppState::pet)
    PetEngine pet;

    // System state
    bool wifiConnected;
    bool bleConnected;
    bool apMode;
    uint32_t uptimeSeconds;
    uint32_t freeHeap;
    uint8_t batteryPercent;

    // Display state
    bool displayReady;
    bool touchReady;
    uint8_t brightness;

    // Audio state
    bool audioReady;
    uint8_t volume;

    // System
    void update();
    void reset();

private:
    AppStateV2();
    AppStateV2(const AppStateV2&) = delete;
    AppStateV2& operator=(const AppStateV2&) = delete;
};

// Global accessor
#define g_state AppStateV2::getInstance()

#endif // APPSTATE_V2_H
