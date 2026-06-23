// ============================================================
// ScreenFactory.cpp — v2.0 Screen Registration & Wiring
// ============================================================

#include "ScreenFactory.h"
#include "config_v2.h"

bool ScreenFactory::_initialized = false;

void ScreenFactory::init() {
    if (_initialized) return;
    _initialized = true;
    
    ScreenManager::begin();
    DEBUG_PRINTF("[ScreenFactory] Initialized\n");
}

Screen* ScreenFactory::createScreen(ScreenID id) {
    switch (id) {
        case SCREEN_MAIN:
            return new MainPetScreen();
        case SCREEN_MENU:
            return new MenuScreen();
        case SCREEN_STATS:
            return new StatsScreen();
        case SCREEN_GAMES:
            return new GamesScreen();
        case SCREEN_SETTINGS:
            return new SettingsScreen();
        case SCREEN_MEMORY_GAME:
            return new MemoryGameScreen();
        case SCREEN_REACTION_GAME:
            return new ReactionGameScreen();
        case SCREEN_TILT_GAME:
            return new TiltGameScreen();
        case SCREEN_OTA:
            return new OTAScreen();
        default:
            return nullptr;
    }
}

void ScreenFactory::wireMainScreen(MainPetScreen *screen) {
    if (!screen) return;
    // Wire pet data binding
    // screen->setPetData(&APP_STATE.pet);
    DEBUG_PRINTF("[ScreenFactory] MainPetScreen wired\n");
}

void ScreenFactory::wireMenuScreen(MenuScreen *screen) {
    if (!screen) return;
    screen->setActionCallback([](const char *action) {
        DEBUG_PRINTF("[ScreenFactory] Menu action: %s\n", action);
        // Dispatch actions to pet engine
        // if (strcmp(action, "feed") == 0) feedPet(APP_STATE.pet);
        // if (strcmp(action, "play") == 0) playPet(APP_STATE.pet);
        // etc.
    });
    DEBUG_PRINTF("[ScreenFactory] MenuScreen wired\n");
}

void ScreenFactory::wireGamesScreen(GamesScreen *screen) {
    if (!screen) return;
    screen->setGameSelectCallback([](int game_id) {
        DEBUG_PRINTF("[ScreenFactory] Game selected: %d\n", game_id);
        // Navigate to game screen based on ID
        // 0 = Memory, 1 = Reaction, 2 = Tilt
    });
    DEBUG_PRINTF("[ScreenFactory] GamesScreen wired\n");
}

void ScreenFactory::wireSettingsScreen(SettingsScreen *screen) {
    if (!screen) return;
    screen->setSettingChangeCallback([](const char *setting, bool value) {
        DEBUG_PRINTF("[ScreenFactory] Setting '%s' = %d\n", setting, value);
        // Apply setting changes to config
    });
    DEBUG_PRINTF("[ScreenFactory] SettingsScreen wired\n");
}

void ScreenFactory::wireOTAScreen(OTAScreen *screen) {
    if (!screen) return;
    screen->setConfirmCallback([]() {
        DEBUG_PRINTF("[ScreenFactory] OTA update confirmed\n");
        // Trigger OTA update process
    });
    DEBUG_PRINTF("[ScreenFactory] OTAScreen wired\n");
}
