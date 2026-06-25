// ============================================================
// ReactionGameScreen.cpp — Reaction time game implementation
// A bar fills and empties — tap when it's in the green zone
// ============================================================

#include "ReactionGameScreen.h"
#include "config_v2.h"

ReactionGameScreen::ReactionGameScreen()
    : Screen("ReactionGame"),
      _score(0), _currentRound(0),
      _game_active(false), _bar_filling(false),
      _bar_direction(1), _green_zone_low(35), _green_zone_high(65),
      _game_timer(nullptr) {
}

void ReactionGameScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_clear_flag(_lv_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    _label_title = createLabel(_lv_screen, "Reaction", &lv_font_montserrat_16);
    lv_obj_align(_label_title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Info label
    _label_info = createLabel(_lv_screen, "Tap when GREEN!", &lv_font_montserrat_12);
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0x2ECC71), 0);
    lv_obj_align(_label_info, LV_ALIGN_TOP_MID, 0, 30);
    
    // Score
    _label_score = createLabel(_lv_screen, "Score: 0/5", &lv_font_montserrat_12);
    lv_obj_align(_label_score, LV_ALIGN_TOP_LEFT, 10, 50);
    
    // Reaction bar container
    lv_obj_t *bar_cont = lv_obj_create(_lv_screen);
    lv_obj_set_size(bar_cont, 200, 30);
    lv_obj_align(bar_cont, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(bar_cont, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(bar_cont, 6, 0);
    lv_obj_set_style_border_width(bar_cont, 1, 0);
    lv_obj_set_style_border_color(bar_cont, lv_color_hex(0x555555), 0);
    lv_obj_set_style_pad_all(bar_cont, 2, 0);
    
    // Green zone indicator (background)
    _bar_green_zone = lv_obj_create(bar_cont);
    lv_obj_set_size(_bar_green_zone, 126, 22);  // ~60% of 196
    lv_obj_align(_bar_green_zone, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_bg_color(_bar_green_zone, lv_color_hex(0x2ECC71), 0);
    lv_obj_set_style_bg_opa(_bar_green_zone, LV_OPA_30, 0);
    lv_obj_set_style_border_width(_bar_green_zone, 0, 0);
    lv_obj_clear_flag(_bar_green_zone, LV_OBJ_FLAG_CLICKABLE);
    
    // Filling bar
    _bar_reaction = lv_bar_create(bar_cont);
    lv_obj_set_size(_bar_reaction, 192, 22);
    lv_obj_align(_bar_reaction, LV_ALIGN_LEFT_MID, 2, 0);
    lv_bar_set_range(_bar_reaction, 0, 100);
    lv_bar_set_value(_bar_reaction, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(_bar_reaction, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_bar_reaction, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_radius(_bar_reaction, 4, 0);
    
    // Tap button
    _btn_tap = lv_btn_create(_lv_screen);
    lv_obj_set_size(_btn_tap, 160, 50);
    lv_obj_align(_btn_tap, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(_btn_tap, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_color(_btn_tap, lv_palette_main(LV_PALETTE_DEEP_ORANGE), LV_STATE_PRESSED);
    lv_obj_set_style_radius(_btn_tap, 12, 0);
    lv_obj_add_event_cb(_btn_tap, tapHandler, LV_EVENT_CLICKED, this);
    
    lv_obj_t *tap_lbl = lv_label_create(_btn_tap);
    lv_label_set_text(tap_lbl, "TAP!");
    lv_obj_set_style_text_font(tap_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(tap_lbl);
    
    // Back button
    _btn_back = createButton(_lv_screen, "Back", 60, 30, backHandler);
    lv_obj_align(_btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(_btn_back, lv_palette_main(LV_PALETTE_GREY), 0);
}

void ReactionGameScreen::onEnter() {
    if (!_game_active) {
        _score = 0;
        _currentRound = 0;
        _game_active = true;
        startRound();
    }
}

void ReactionGameScreen::onExit() {
    if (_game_timer) {
        lv_timer_del(_game_timer);
        _game_timer = nullptr;
    }
    _bar_filling = false;
}

void ReactionGameScreen::startRound() {
    _currentRound++;
    if (_currentRound > 5) {
        endGame(true);
        return;
    }
    
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %d/5", _score);
    lv_label_set_text(_label_score, buf);
    
    // Randomize green zone position
    int zone_start = random(20, 50);
    _green_zone_low = zone_start;
    _green_zone_high = zone_start + random(20, 35);
    if (_green_zone_high > 95) _green_zone_high = 95;
    
    // Update green zone visual
    int zone_width = (_green_zone_high - _green_zone_low) * 192 / 100;
    int zone_x = _green_zone_low * 192 / 100;
    lv_obj_set_width(_bar_green_zone, LV_MAX(zone_width, 20));
    lv_obj_align(_bar_green_zone, LV_ALIGN_LEFT_MID, zone_x + 2, 0);
    
    // Reset bar
    lv_bar_set_value(_bar_reaction, 0, LV_ANIM_OFF);
    _bar_direction = 1;
    _bar_filling = true;
    
    lv_label_set_text(_label_info, "Tap when GREEN!");
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0x2ECC71), 0);
    
    // Start bar animation timer (20ms = 50 FPS)
    if (_game_timer) {
        lv_timer_del(_game_timer);
    }
    _game_timer = lv_timer_create(gameTimerCb, 20, this);
}

void ReactionGameScreen::onTap() {
    if (!_bar_filling) return;
    
    int val = lv_bar_get_value(_bar_reaction);
    
    if (val >= _green_zone_low && val <= _green_zone_high) {
        // Hit!
        _score++;
        lv_label_set_text(_label_info, "HIT! +1");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0x2ECC71), 0);
        
        // Flash bar green
        lv_obj_set_style_bg_color(_bar_reaction, lv_color_hex(0x2ECC71), LV_PART_INDICATOR);
    } else {
        // Miss
        lv_label_set_text(_label_info, "MISS!");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0xE74C3C), 0);
        
        // Flash bar red
        lv_obj_set_style_bg_color(_bar_reaction, lv_color_hex(0xE74C3C), LV_PART_INDICATOR);
    }
    
    _bar_filling = false;
    if (_game_timer) {
        lv_timer_del(_game_timer);
        _game_timer = nullptr;
    }
    
    // Next round after 1 second
    lv_timer_t *next_timer = lv_timer_create([](lv_timer_t *t) {
        ReactionGameScreen *self = (ReactionGameScreen *)lv_timer_get_user_data(t);
        if (self) {
            lv_obj_set_style_bg_color(self->_bar_reaction, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
            self->startRound();
        }
        lv_timer_del(t);
    }, 1000, this);
    lv_timer_set_repeat_count(next_timer, 1);
}

void ReactionGameScreen::endGame(bool won) {
    _game_active = false;
    _bar_filling = false;
    
    if (_game_timer) {
        lv_timer_del(_game_timer);
        _game_timer = nullptr;
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Final: %d/5", _score);
    lv_label_set_text(_label_score, buf);
    
    if (_score >= 4) {
        lv_label_set_text(_label_info, "Great reflexes!");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0x2ECC71), 0);
    } else if (_score >= 2) {
        lv_label_set_text(_label_info, "Not bad!");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0xF39C12), 0);
    } else {
        lv_label_set_text(_label_info, "Keep practicing!");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0xE74C3C), 0);
    }
    
    if (_result_cb) {
        _result_cb(_score >= 3, _score);
    }
    
    // Auto-return after 2.5 seconds
    lv_timer_t *return_timer = lv_timer_create([](lv_timer_t *t) {
        ScreenManager::handleBack();
        lv_timer_del(t);
    }, 2500, nullptr);
    lv_timer_set_repeat_count(return_timer, 1);
}

void ReactionGameScreen::tapHandler(lv_event_t *e) {
    ReactionGameScreen *self = (ReactionGameScreen *)lv_event_get_user_data(e);
    if (self) self->onTap();
}

void ReactionGameScreen::backHandler(lv_event_t *e) {
    ScreenManager::handleBack();
}

void ReactionGameScreen::gameTimerCb(lv_timer_t *timer) {
    ReactionGameScreen *self = (ReactionGameScreen *)lv_timer_get_user_data(timer);
    if (!self || !self->_bar_filling) return;
    
    int val = lv_bar_get_value(self->_bar_reaction);
    int speed = 2 + self->_currentRound;  // Gets faster each round
    
    val += self->_bar_direction * speed;
    
    if (val >= 100) {
        val = 100;
        self->_bar_direction = -1;
    } else if (val <= 0) {
        val = 0;
        self->_bar_direction = 1;
    }
    
    lv_bar_set_value(self->_bar_reaction, val, LV_ANIM_OFF);
}
