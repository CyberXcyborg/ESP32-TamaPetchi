// ============================================================
// UIScreens.cpp — v2.0 LVGL UI Screens Implementation
// ============================================================

#include "UIScreens.h"

// ============================================================
// Main Pet Screen
// ============================================================

MainPetScreen* MainPetScreen::_instance = nullptr;

MainPetScreen::MainPetScreen()
    : Screen("Main"), _pet_data(nullptr), _anim_player_id(0) {
    _instance = this;
}

void MainPetScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_bg_grad_color(_lv_screen, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_bg_grad_dir(_lv_screen, LV_GRAD_DIR_VER, 0);
    
    createBackground();
    createPetSprite();
    createStatsOverlay();
    createStatusBar();
}

void MainPetScreen::createBackground() {
    // Decorative background elements
    lv_obj_t *stars = lv_obj_create(_lv_screen);
    lv_obj_set_size(stars, 240, 120);
    lv_obj_align(stars, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(stars, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stars, 0, 0);
    lv_obj_clear_flag(stars, LV_OBJ_FLAG_CLICKABLE);
}

void MainPetScreen::createPetSprite() {
    // Pet sprite area (centered, 80x80)
    _pet_sprite = lv_obj_create(_lv_screen);
    lv_obj_set_size(_pet_sprite, 80, 80);
    lv_obj_align(_pet_sprite, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(_pet_sprite, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_radius(_pet_sprite, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_pet_sprite, 3, 0);
    lv_obj_set_style_border_color(_pet_sprite, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(_pet_sprite, LV_OPA_60, 0);
    lv_obj_clear_flag(_pet_sprite, LV_OBJ_FLAG_CLICKABLE);
    
    // Pet emoji/icon inside sprite
    lv_obj_t *emoji = lv_label_create(_pet_sprite);
    lv_label_set_text(emoji, LV_SYMBOL_HOME);  // Placeholder
    lv_obj_set_style_text_font(emoji, &lv_font_montserrat_12, 0);
    lv_obj_center(emoji);
}

void MainPetScreen::createStatsOverlay() {
    // Top-left: Hunger
    lv_obj_t *lbl_h = createLabel(_lv_screen, "Hunger", &lv_font_montserrat_10);
    lv_obj_align(lbl_h, LV_ALIGN_TOP_LEFT, 8, 35);
    _bar_hunger = createBar(_lv_screen, 80, 8, lv_palette_main(LV_PALETTE_RED));
    lv_obj_align(_bar_hunger, LV_ALIGN_TOP_LEFT, 8, 50);
    
    // Top-right: Happiness
    lv_obj_t *lbl_hp = createLabel(_lv_screen, "Happy", &lv_font_montserrat_10);
    lv_obj_align(lbl_hp, LV_ALIGN_TOP_RIGHT, -8, 35);
    _bar_happiness = createBar(_lv_screen, 80, 8, lv_palette_main(LV_PALETTE_YELLOW));
    lv_obj_align(_bar_happiness, LV_ALIGN_TOP_RIGHT, -8, 50);
    
    // Bottom-left: Energy
    lv_obj_t *lbl_e = createLabel(_lv_screen, "Energy", &lv_font_montserrat_10);
    lv_obj_align(lbl_e, LV_ALIGN_BOTTOM_LEFT, 8, -50);
    _bar_energy = createBar(_lv_screen, 80, 8, lv_palette_main(LV_PALETTE_BLUE));
    lv_obj_align(_bar_energy, LV_ALIGN_BOTTOM_LEFT, 8, -35);
    
    // Bottom-right: Health
    lv_obj_t *lbl_hl = createLabel(_lv_screen, "Health", &lv_font_montserrat_10);
    lv_obj_align(lbl_hl, LV_ALIGN_BOTTOM_RIGHT, -8, -50);
    _bar_health = createBar(_lv_screen, 80, 8, lv_palette_main(LV_PALETTE_GREEN));
    lv_obj_align(_bar_health, LV_ALIGN_BOTTOM_RIGHT, -8, -35);
}

void MainPetScreen::createStatusBar() {
    // Pet name (top center)
    _label_name = createLabel(_lv_screen, "TamaPetchi", &lv_font_montserrat_14);
    lv_obj_align(_label_name, LV_ALIGN_TOP_MID, 0, 8);
    
    // Stage label
    _label_stage = createLabel(_lv_screen, "Baby", &lv_font_montserrat_10);
    lv_obj_set_style_text_color(_label_stage, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(_label_stage, LV_ALIGN_TOP_MID, 0, 26);
    
    // Mood emoji (near pet)
    _label_mood = createLabel(_lv_screen, "", &lv_font_montserrat_16);
    lv_obj_align(_label_mood, LV_ALIGN_CENTER, 50, -50);
}

void MainPetScreen::onEnter() {
    updateStats();
    updatePetColor();
}

void MainPetScreen::onExit() {
    // Pause animations when leaving
}

void MainPetScreen::onUpdate() {
    // Update stats periodically
    static uint32_t last_update = 0;
    uint32_t now = millis();
    if (now - last_update > 1000) {
        updateStats();
        updatePetColor();
        last_update = now;
    }
}

void MainPetScreen::setPetData(const PetData *data) {
    _pet_data = data;
    updateStats();
    updatePetColor();
}

void MainPetScreen::updateStats() {
    if (!_pet_data) return;
    
    if (_label_name) {
        lv_label_set_text(_label_name, _pet_data->name.c_str());
    }
    
    if (_label_stage) {
        const char *stages[] = {"Baby", "Child", "Adult", "Elder"};
        lv_label_set_text(_label_stage, stages[_pet_data->stage % 4]);
    }
    
    if (_bar_hunger) lv_bar_set_value(_bar_hunger, _pet_data->hunger, LV_ANIM_ON);
    if (_bar_happiness) lv_bar_set_value(_bar_happiness, _pet_data->happiness, LV_ANIM_ON);
    if (_bar_energy) lv_bar_set_value(_bar_energy, _pet_data->energy, LV_ANIM_ON);
    if (_bar_health) lv_bar_set_value(_bar_health, _pet_data->health, LV_ANIM_ON);
}

void MainPetScreen::updatePetColor() {
    if (!_pet_data || !_pet_sprite) return;
    
    lv_color_t color;
    if (_pet_data->health > 70) color = lv_palette_main(LV_PALETTE_GREEN);
    else if (_pet_data->health > 40) color = lv_palette_main(LV_PALETTE_YELLOW);
    else if (_pet_data->health > 10) color = lv_palette_main(LV_PALETTE_ORANGE);
    else color = lv_palette_main(LV_PALETTE_RED);
    
    lv_obj_set_style_bg_color(_pet_sprite, color, 0);
}

void MainPetScreen::onTouch(lv_point_t *point) {
    (void)point;
    // Tap pet -> trigger happy animation
    DEBUG_PRINTF("[UI] Pet tapped!\n");
}

void MainPetScreen::onSwipe(uint8_t direction) {
    switch (direction) {
        case 0: // Up -> Menu
            DEBUG_PRINTF("[UI] Swipe up -> Menu\n");
            break;
        case 1: // Down -> Stats
            DEBUG_PRINTF("[UI] Swipe down -> Stats\n");
            break;
        case 2: // Left -> Previous pet
        case 3: // Right -> Next pet
            DEBUG_PRINTF("[UI] Swipe left/right -> Cycle pets\n");
            break;
    }
}

void MainPetScreen::animFrameHandler(uint8_t frame_index) {
    if (_instance && _instance->_pet_sprite) {
        // Update pet sprite based on frame
        (void)frame_index;
    }
}

// ============================================================
// Menu Screen
// ============================================================

MenuScreen::MenuScreen()
    : Screen("Menu"), _action_cb(nullptr), _toast(nullptr), _toast_tick(0) {
}

void MenuScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x0f3460), 0);
    
    // Title
    lv_obj_t *title = createLabel(_lv_screen, "Menu", &lv_font_montserrat_16);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 4x2 grid of buttons
    const char *labels[] = {"Feed", "Play", "Clean", "Sleep", "Games", "Shop", "Settings", "Back"};
    const lv_palette_t colors[] = {
        LV_PALETTE_RED, LV_PALETTE_ORANGE, LV_PALETTE_BLUE, LV_PALETTE_PURPLE,
        LV_PALETTE_GREEN, LV_PALETTE_AMBER, LV_PALETTE_GREY, LV_PALETTE_DEEP_ORANGE
    };
    
    lv_coord_t btn_w = 90, btn_h = 40;
    lv_coord_t gap = 12;
    lv_coord_t start_x = (240 - (btn_w * 2 + gap)) / 2;
    lv_coord_t start_y = 45;
    
    for (int i = 0; i < 8; i++) {
        int row = i / 2;
        int col = i % 2;
        
        _buttons[i] = lv_btn_create(_lv_screen);
        lv_obj_set_size(_buttons[i], btn_w, btn_h);
        lv_obj_set_pos(_buttons[i], start_x + col * (btn_w + gap), start_y + row * (btn_h + gap));
        lv_obj_set_style_bg_color(_buttons[i], lv_palette_main(colors[i]), 0);
        lv_obj_set_style_radius(_buttons[i], 10, 0);
        lv_obj_add_event_cb(_buttons[i], btnEventHandler, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        
        // Highlight animation on press
        lv_obj_set_style_transform_width(_buttons[i], -4, LV_STATE_PRESSED);
        lv_obj_set_style_transform_height(_buttons[i], -4, LV_STATE_PRESSED);
        
        lv_obj_t *lbl = lv_label_create(_buttons[i]);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
    }
}

void MenuScreen::onEnter() {
    _toast = nullptr;
}

void MenuScreen::onExit() {
    if (_toast) {
        lv_obj_del(_toast);
        _toast = nullptr;
    }
}

void MenuScreen::btnEventHandler(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    MenuScreen *self = (MenuScreen*)ScreenManager::getCurrentScreen();
    if (!self) return;
    
    const char *actions[] = {"feed", "play", "clean", "sleep", "games", "shop", "settings", "back"};
    
    // Button press animation
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_set_style_transform_zoom(btn, 240, LV_STATE_PRESSED);  // 96% scale
    
    if (idx == 7) {
        // Back
        ScreenManager::handleBack();
    } else {
        self->executeAction(actions[idx]);
    }
}

void MenuScreen::executeAction(const char *action) {
    char toast_msg[64];
    snprintf(toast_msg, sizeof(toast_msg), "%s!", action);
    showToast(toast_msg);
    
    if (_action_cb) {
        _action_cb(action);
    }
    
    DEBUG_PRINTF("[UI] Menu action: %s\n", action);
}

void MenuScreen::showToast(const char *message) {
    if (_toast) {
        lv_obj_del(_toast);
    }
    
    _toast = lv_obj_create(_lv_screen);
    lv_obj_set_size(_toast, 160, 36);
    lv_obj_align(_toast, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(_toast, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(_toast, LV_OPA_90, 0);
    lv_obj_set_style_radius(_toast, 18, 0);
    lv_obj_set_style_border_width(_toast, 0, 0);
    lv_obj_clear_flag(_toast, LV_OBJ_FLAG_CLICKABLE);
    
    lv_obj_t *lbl = lv_label_create(_toast);
    lv_label_set_text(lbl, message);
    lv_obj_center(lbl);
    
    // Auto-dismiss after 2 seconds
    lv_timer_t *timer = lv_timer_create(toastTimerCb, 2000, _toast);
    lv_timer_set_repeat_count(timer, 1);
}

void MenuScreen::toastTimerCb(lv_timer_t *timer) {
    lv_obj_t *toast = (lv_obj_t*)lv_timer_get_user_data(timer);
    if (toast) {
        lv_obj_del(toast);
    }
}

// ============================================================
// Stats Screen
// ============================================================

StatsScreen::StatsScreen()
    : Screen("Stats"), _pet_data(nullptr) {
    memset(_bar_stats, 0, sizeof(_bar_stats));
    memset(_label_info, 0, sizeof(_label_info));
    _achievement_list = nullptr;
    _lineage_tree = nullptr;
}

void StatsScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    
    // Title
    lv_obj_t *title = createLabel(_lv_screen, "Stats", &lv_font_montserrat_16);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    
    createStatsPanel();
    createInfoPanel();
    createAchievementsPanel();
}

void StatsScreen::createStatsPanel() {
    const char *labels[] = {"Hunger", "Happy", "Energy", "Health"};
    const lv_palette_t colors[] = {
        LV_PALETTE_RED, LV_PALETTE_YELLOW, LV_PALETTE_BLUE, LV_PALETTE_GREEN
    };
    
    for (int i = 0; i < 4; i++) {
        lv_coord_t y = 35 + i * 28;
        
        lv_obj_t *lbl = createLabel(_lv_screen, labels[i], &lv_font_montserrat_10);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, y);
        
        _bar_stats[i] = createBar(_lv_screen, 140, 10, lv_palette_main(colors[i]));
        lv_obj_align(_bar_stats[i], LV_ALIGN_TOP_RIGHT, -8, y);
    }
}

void StatsScreen::createInfoPanel() {
    const char *info_labels[] = {"Name:", "Age:", "Stage:", "Gen:", "Weight:", "Mood:"};
    
    for (int i = 0; i < 6; i++) {
        lv_coord_t y = 155 + i * 14;
        
        _label_info[i] = createLabel(_lv_screen, info_labels[i], &lv_font_montserrat_10);
        lv_obj_set_style_text_color(_label_info[i], lv_color_hex(0x888888), 0);
        lv_obj_align(_label_info[i], LV_ALIGN_TOP_LEFT, 8, y);
    }
}

void StatsScreen::createAchievementsPanel() {
    // Achievement badges label
    lv_obj_t *lbl = createLabel(_lv_screen, "Achievements", &lv_font_montserrat_10);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, 240);
    
    // Placeholder for achievement badges
    _achievement_list = lv_obj_create(_lv_screen);
    lv_obj_set_size(_achievement_list, 220, 40);
    lv_obj_align(_achievement_list, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_opa(_achievement_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_achievement_list, 0, 0);
    lv_obj_set_flex_flow(_achievement_list, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_achievement_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

void StatsScreen::onEnter() {
    updateStatsBars();
}

void StatsScreen::onUpdate() {
    static uint32_t last_update = 0;
    uint32_t now = millis();
    if (now - last_update > 2000) {
        updateStatsBars();
        last_update = now;
    }
}

void StatsScreen::setPetData(const PetData *data) {
    _pet_data = data;
    updateStatsBars();
}

void StatsScreen::setAchievements(const char **names, const bool *unlocked, uint8_t count) {
    if (!_achievement_list) return;
    
    // Clear existing
    lv_obj_clean(_achievement_list);
    
    for (uint8_t i = 0; i < count && i < 10; i++) {
        lv_obj_t *badge = lv_obj_create(_achievement_list);
        lv_obj_set_size(badge, 24, 24);
        lv_obj_set_style_radius(badge, 4, 0);
        
        if (unlocked[i]) {
            lv_obj_set_style_bg_color(badge, lv_palette_main(LV_PALETTE_AMBER), 0);
        } else {
            lv_obj_set_style_bg_color(badge, lv_color_hex(0x444444), 0);
        }
        
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE);
    }
}

void StatsScreen::updateStatsBars() {
    if (!_pet_data) return;
    
    if (_bar_stats[0]) lv_bar_set_value(_bar_stats[0], _pet_data->hunger, LV_ANIM_ON);
    if (_bar_stats[1]) lv_bar_set_value(_bar_stats[1], _pet_data->happiness, LV_ANIM_ON);
    if (_bar_stats[2]) lv_bar_set_value(_bar_stats[2], _pet_data->energy, LV_ANIM_ON);
    if (_bar_stats[3]) lv_bar_set_value(_bar_stats[3], _pet_data->health, LV_ANIM_ON);
    
    // Update info labels
    if (_label_info[0]) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Name: %s", _pet_data->name.c_str());
        lv_label_set_text(_label_info[0], buf);
    }
    if (_label_info[1]) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Age: %lum", _pet_data->age_minutes);
        lv_label_set_text(_label_info[1], buf);
    }
    if (_label_info[2]) {
        const char *stages[] = {"Baby", "Child", "Adult", "Elder"};
        char buf[32];
        snprintf(buf, sizeof(buf), "Stage: %s", stages[_pet_data->stage % 4]);
        lv_label_set_text(_label_info[2], buf);
    }
    if (_label_info[3]) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Gen: %d", _pet_data->generation);
        lv_label_set_text(_label_info[3], buf);
    }
}

// ============================================================
// Games Screen — Game selection menu (v2.0)
// ============================================================

GamesScreen::GamesScreen()
    : Screen("Games"), _game_cb(nullptr), _list(nullptr) {
}

void GamesScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *title = createLabel(_lv_screen, "Games", &lv_font_montserrat_16);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Game list with icons
    _list = lv_list_create(_lv_screen);
    lv_obj_set_size(_list, 220, 160);
    lv_obj_align(_list, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(_list, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_border_width(_list, 1, 0);
    lv_obj_set_style_border_color(_list, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(_list, 8, 0);
    lv_obj_set_style_pad_all(_list, 4, 0);
    
    // Add games to list
    const char *game_names[] = {"Memory Game", "Reaction Time", "Tilt Game", "Back"};
    const char *game_icons[] = {LV_SYMBOL_EYE_OPEN, LV_SYMBOL_PLAY, LV_SYMBOL_REFRESH, LV_SYMBOL_LEFT};
    const char *game_scores[] = {"Best: --", "Best: --", "Phase 21", ""};
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_list_add_btn(_list, game_icons[i], game_names[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a2e), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x3498DB), LV_STATE_PRESSED);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 8, 0);
        lv_obj_add_event_cb(btn, listHandler, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        
        // Add high score label
        if (i < 3) {
            lv_obj_t *score_lbl = lv_label_create(btn);
            lv_label_set_text(score_lbl, game_scores[i]);
            lv_obj_set_style_text_font(score_lbl, &lv_font_montserrat_10, 0);
            lv_obj_set_style_text_color(score_lbl, lv_color_hex(0x888888), 0);
            lv_obj_align(score_lbl, LV_ALIGN_RIGHT_MID, -8, 0);
        }
    }
}

void GamesScreen::onEnter() {
}

void GamesScreen::listHandler(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    
    if (idx == 3) {
        ScreenManager::handleBack();
    } else {
        GamesScreen *self = (GamesScreen*)ScreenManager::getCurrentScreen();
        if (self && self->_game_cb) {
            self->_game_cb(idx);
        }
    }
}

// ============================================================
// Settings Screen (v2.0) — lv_list with switches, sliders, dialogs
// ============================================================

SettingsScreen::SettingsScreen()
    : Screen("Settings"), _setting_cb(nullptr),
      _sw_sound(nullptr), _sw_night(nullptr),
      _slider_brightness(nullptr), _slider_volume(nullptr) {
}

void SettingsScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    
    lv_obj_t *title = createLabel(_lv_screen, "Settings", &lv_font_montserrat_16);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Settings list
    lv_obj_t *list = lv_list_create(_lv_screen);
    lv_obj_set_size(list, 220, 175);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_border_width(list, 1, 0);
    lv_obj_set_style_border_color(list, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(list, 8, 0);
    lv_obj_set_style_pad_all(list, 4, 0);
    
    // Sound toggle
    lv_obj_t *item_sound = lv_list_add_btn(list, LV_SYMBOL_AUDIO, "Sound");
    _sw_sound = lv_switch_create(item_sound);
    lv_obj_align(_sw_sound, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_add_state(_sw_sound, LV_STATE_CHECKED);
    lv_obj_add_event_cb(_sw_sound, switchHandler, LV_EVENT_VALUE_CHANGED, (void*)"sound");
    lv_obj_set_style_bg_color(item_sound, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(item_sound, 0, 0);
    lv_obj_set_style_pad_all(item_sound, 6, 0);
    
    // Night mode toggle
    lv_obj_t *item_night = lv_list_add_btn(list, LV_SYMBOL_EYE_CLOSE, "Night Mode");
    _sw_night = lv_switch_create(item_night);
    lv_obj_align(_sw_night, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_add_event_cb(_sw_night, switchHandler, LV_EVENT_VALUE_CHANGED, (void*)"night_mode");
    lv_obj_set_style_bg_color(item_night, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(item_night, 0, 0);
    lv_obj_set_style_pad_all(item_night, 6, 0);
    
    // Brightness slider
    lv_obj_t *item_bright = lv_list_add_btn(list, LV_SYMBOL_CHARGE, "Brightness");
    _slider_brightness = lv_slider_create(item_bright);
    lv_obj_set_size(_slider_brightness, 100, 10);
    lv_obj_align(_slider_brightness, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_slider_set_range(_slider_brightness, 10, 100);
    lv_slider_set_value(_slider_brightness, 80, LV_ANIM_OFF);
    lv_obj_add_event_cb(_slider_brightness, sliderHandler, LV_EVENT_VALUE_CHANGED, (void*)"brightness");
    lv_obj_set_style_bg_color(item_bright, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(item_bright, 0, 0);
    lv_obj_set_style_pad_all(item_bright, 6, 0);
    
    // Volume slider
    lv_obj_t *item_vol = lv_list_add_btn(list, LV_SYMBOL_VOLUME_MAX, "Volume");
    _slider_volume = lv_slider_create(item_vol);
    lv_obj_set_size(_slider_volume, 100, 10);
    lv_obj_align(_slider_volume, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_slider_set_range(_slider_volume, 0, 100);
    lv_slider_set_value(_slider_volume, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(_slider_volume, sliderHandler, LV_EVENT_VALUE_CHANGED, (void*)"volume");
    lv_obj_set_style_bg_color(item_vol, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(item_vol, 0, 0);
    lv_obj_set_style_pad_all(item_vol, 6, 0);
    
    // Language selector
    lv_obj_t *item_lang = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Language");
    lv_obj_t *lbl_lang = lv_label_create(item_lang);
    lv_label_set_text(lbl_lang, "EN");
    lv_obj_align(lbl_lang, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_bg_color(item_lang, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(item_lang, 0, 0);
    lv_obj_set_style_pad_all(item_lang, 6, 0);
    lv_obj_add_event_cb(item_lang, languageHandler, LV_EVENT_CLICKED, this);
    
    // Factory reset
    lv_obj_t *item_reset = lv_list_add_btn(list, LV_SYMBOL_TRASH, "Factory Reset");
    lv_obj_set_style_text_color(item_reset, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_color(item_reset, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(item_reset, 0, 0);
    lv_obj_set_style_pad_all(item_reset, 6, 0);
    lv_obj_add_event_cb(item_reset, resetBtnHandler, LV_EVENT_CLICKED, (void*)0);
    
    // Back button (standalone)
    lv_obj_t *btn_back = createButton(_lv_screen, "Back", 80, 36, [](lv_event_t *e) {
        ScreenManager::handleBack();
    }, nullptr);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void SettingsScreen::onEnter() {
}

void SettingsScreen::switchHandler(lv_event_t *e) {
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    const char *setting = (const char*)lv_event_get_user_data(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    
    SettingsScreen *self = (SettingsScreen*)ScreenManager::getCurrentScreen();
    if (self && self->_setting_cb) {
        self->_setting_cb(setting, state);
    }
}

void SettingsScreen::sliderHandler(lv_event_t *e) {
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    const char *setting = (const char*)lv_event_get_user_data(e);
    int value = lv_slider_get_value(slider);
    
    DEBUG_PRINTF("[UI] Setting '%s' = %d\n", setting, value);
    
    SettingsScreen *self = (SettingsScreen*)ScreenManager::getCurrentScreen();
    if (self && self->_setting_cb) {
        // Pass value as bool (true if > 50) for backward compatibility
        // In a full implementation, use a variant callback
        self->_setting_cb(setting, value > 50);
    }
}

void SettingsScreen::languageHandler(lv_event_t *e) {
    // Cycle through languages: EN -> ES -> FR -> DE -> JA -> EN
    static const char *lang_codes[] = {"EN", "ES", "FR", "DE", "JA"};
    static const char *lang_names[] = {"English", "Espanol", "Francais", "Deutsch", "Japanese"};
    static int lang_idx = 0;
    
    lang_idx = (lang_idx + 1) % 5;
    DEBUG_PRINTF("[UI] Language changed to %s (%s)\n", lang_codes[lang_idx], lang_names[lang_idx]);
    
    // In a real implementation, update i18n and persist to config
}

void SettingsScreen::resetBtnHandler(lv_event_t *e) {
    // Factory reset - show lv_msgbox confirmation (LVGL 9.x API)
    DEBUG_PRINTF("[UI] Factory reset requested\n");

    lv_obj_t *mbox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox, "Factory Reset");
    lv_msgbox_add_text(mbox, "This will erase all pet data.\nContinue?");
    lv_msgbox_add_footer_button(mbox, "Yes");
    lv_msgbox_add_footer_button(mbox, "No");
    lv_obj_set_size(mbox, 200, 120);
    lv_obj_center(mbox);

    // Add event callbacks for buttons
    lv_obj_t *footer = lv_msgbox_get_footer(mbox);
    uint32_t btn_count = lv_obj_get_child_count(footer);
    for (uint32_t i = 0; i < btn_count && i < 2; i++) {
        lv_obj_t *btn = lv_obj_get_child(footer, i);
        if (btn) {
            lv_obj_add_event_cb(btn, [](lv_event_t *ev) {
                lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(ev);
                lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(ev);
                lv_obj_t *label = lv_obj_get_child(btn, 0);
                const char *txt = label ? lv_label_get_text(label) : "";
                if (strcmp(txt, "Yes") == 0) {
                    DEBUG_PRINTF("[UI] Factory reset confirmed\n");
                    // In real implementation: trigger factory reset
                }
                lv_msgbox_close(mbox);
            }, LV_EVENT_CLICKED, mbox);
        }
    }
    (void)e;
}
