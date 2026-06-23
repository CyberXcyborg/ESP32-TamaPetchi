// ============================================================
// ScreenFactory.h — v2.0 Screen Registration & Wiring
// Central registry for all LVGL screens
// Phase 20.4: Migrate all v1.x screens to LVGL
// ============================================================

#ifndef SCREEN_FACTORY_H
#define SCREEN_FACTORY_H

#include "ScreenManager.h"
#include "UIScreens.h"
#include "MemoryGameScreen.h"
#include "ReactionGameScreen.h"
#include "TiltGameScreen.h"
#include "OTAScreen.h"

// ============================================================
// Screen IDs for navigation
// ============================================================
enum ScreenID {
    SCREEN_MAIN = 0,
    SCREEN_MENU,
    SCREEN_STATS,
    SCREEN_GAMES,
    SCREEN_SETTINGS,
    SCREEN_MEMORY_GAME,
    SCREEN_REACTION_GAME,
    SCREEN_TILT_GAME,
    SCREEN_OTA,
    SCREEN_COUNT
};

// ============================================================
// Screen Factory — creates and wires all screens
// ============================================================
class ScreenFactory {
public:
    static void init();
    static Screen* createScreen(ScreenID id);
    
    // Wire all screen callbacks to game logic
    static void wireMainScreen(MainPetScreen *screen);
    static void wireMenuScreen(MenuScreen *screen);
    static void wireGamesScreen(GamesScreen *screen);
    static void wireSettingsScreen(SettingsScreen *screen);
    static void wireOTAScreen(OTAScreen *screen);
    
private:
    static bool _initialized;
};

#endif // SCREEN_FACTORY_H
