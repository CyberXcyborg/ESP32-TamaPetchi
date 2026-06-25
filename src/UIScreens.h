// ============================================================
// UIScreens.h — v2.0 LVGL UI Screens
// MainPetScreen, MenuScreen, StatsScreen implementations
// ============================================================

#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include "ScreenManager.h"
#include "Pet_v2.h"
#include "animations.h"
#include "config_v2.h"

// ============================================================
// Main Pet Screen — Primary screen showing the pet
// ============================================================
class MainPetScreen : public Screen {
public:
    MainPetScreen();
    
    void create() override;
    void onEnter() override;
    void onExit() override;
    void onUpdate() override;
    
    // Pet data binding
    void setPetData(const PetData *data);
    void setAnimationPlayerID(uint8_t anim_id) { _anim_player_id = anim_id; }
    
    // Touch interactions
    void onTouch(lv_point_t *point) override;
    void onSwipe(uint8_t direction) override;
    
    // Update methods
    void updateStats();
    void updatePetColor();

private:
    const PetData *_pet_data;
    uint8_t _anim_player_id;
    
    // UI elements
    lv_obj_t *_pet_sprite;       // Pet image/sprite
    lv_obj_t *_label_name;       // Pet name
    lv_obj_t *_label_stage;      // Evolution stage
    lv_obj_t *_bar_hunger;       // Stats bars
    lv_obj_t *_bar_happiness;
    lv_obj_t *_bar_energy;
    lv_obj_t *_bar_health;
    lv_obj_t *_label_mood;       // Mood emoji
    
    // Background
    lv_obj_t *_bg_gradient;
    
    void createBackground();
    void createPetSprite();
    void createStatsOverlay();
    void createStatusBar();
    
    // Animation frame callback
    static void animFrameHandler(uint8_t frame_index);
    static MainPetScreen *_instance;  // For callback access
};

// ============================================================
// Menu Screen — Action grid (Feed, Play, Clean, Sleep, etc.)
// ============================================================
class MenuScreen : public Screen {
public:
    MenuScreen();
    
    void create() override;
    void onEnter() override;
    void onExit() override;
    
    // Menu action callback
    typedef std::function<void(const char*)> MenuActionCallback;
    void setActionCallback(MenuActionCallback cb) { _action_cb = cb; }

private:
    MenuActionCallback _action_cb;
    lv_obj_t *_buttons[8];
    lv_obj_t *_toast;  // Toast notification
    uint32_t _toast_tick;
    
    static void btnEventHandler(lv_event_t *e);
    static void toastTimerCb(lv_timer_t *timer);
    void showToast(const char *message);
    void executeAction(const char *action);
};

// ============================================================
// Stats Screen — Detailed pet statistics
// ============================================================
class StatsScreen : public Screen {
public:
    StatsScreen();
    
    void create() override;
    void onEnter() override;
    void onUpdate() override;
    
    void setPetData(const PetData *data);
    void setAchievements(const char **names, const bool *unlocked, uint8_t count);

private:
    const PetData *_pet_data;
    
    // UI elements
    lv_obj_t *_bar_stats[4];     // hunger, happiness, energy, health
    lv_obj_t *_label_info[6];    // name, age, weight, generation, stage, mood
    lv_obj_t *_achievement_list; // Scrollable achievement badges
    lv_obj_t *_lineage_tree;     // Mini lineage display
    
    void createStatsPanel();
    void createInfoPanel();
    void createAchievementsPanel();
    void createLineagePanel();
    void updateStatsBars();
};

// ============================================================
// Games Screen — Game selection menu (v2.0)
// ============================================================
class GamesScreen : public Screen {
public:
    GamesScreen();
    
    void create() override;
    void onEnter() override;
    
    typedef std::function<void(int)> GameSelectCallback;
    void setGameSelectCallback(GameSelectCallback cb) { _game_cb = cb; }

private:
    GameSelectCallback _game_cb;
    lv_obj_t *_list;
    
    static void listHandler(lv_event_t *e);
};

// ============================================================
// Settings Screen — Configuration options
// ============================================================
class SettingsScreen : public Screen {
public:
    SettingsScreen();
    
    void create() override;
    void onEnter() override;
    
    typedef std::function<void(const char*, bool)> SettingChangeCallback;
    void setSettingChangeCallback(SettingChangeCallback cb) { _setting_cb = cb; }

private:
    SettingChangeCallback _setting_cb;
    lv_obj_t *_sw_sound;
    lv_obj_t *_sw_night;
    lv_obj_t *_slider_brightness;
    lv_obj_t *_slider_volume;
    
    static void switchHandler(lv_event_t *e);
    static void sliderHandler(lv_event_t *e);
    static void languageHandler(lv_event_t *e);
    static void resetBtnHandler(lv_event_t *e);
};

#endif // UI_SCREENS_H
