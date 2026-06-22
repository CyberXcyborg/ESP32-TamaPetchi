// ============================================================
// AppState_v2.cpp — Singleton Global State Implementation
// ============================================================

#include "AppState_v2.h"
#include <esp_system.h>

AppStateV2::AppStateV2()
    : wifiConnected(false)
    , bleConnected(false)
    , apMode(false)
    , uptimeSeconds(0)
    , freeHeap(0)
    , batteryPercent(100)
    , displayReady(false)
    , touchReady(false)
    , brightness(255)
    , audioReady(false)
    , volume(80)
{
}

AppStateV2& AppStateV2::getInstance() {
    static AppStateV2 instance;
    return instance;
}

void AppStateV2::update() {
    uptimeSeconds = millis() / 1000;
    freeHeap = ESP.getFreeHeap();
}

void AppStateV2::reset() {
    wifiConnected = false;
    bleConnected = false;
    apMode = false;
    uptimeSeconds = 0;
    displayReady = false;
    touchReady = false;
    audioReady = false;
}
