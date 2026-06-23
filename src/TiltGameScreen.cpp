// ============================================================
// TiltGameScreen.cpp — Tilt game placeholder implementation
// Phase 20.4: Placeholder with demo mode for Phase 21 accel
// ============================================================

#include "TiltGameScreen.h"
#include "config_v2.h"

TiltGameScreen::TiltGameScreen()
    : Screen("TiltGame"),
      _score(0), _game_active(false) {
}

void TiltGameScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_clear_flag(_lv_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    _label_title = createLabel(_lv_screen, "Tilt Game", &lv_font_montserrat_16);
    lv_obj_align(_label_title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Info label
    _label_info = createLabel(_lv_screen, "Coming in Phase 21!", &lv_font_montserrat_12);
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(_label_info, LV_ALIGN_TOP_MID, 0, 35);
    
    // Score
    _label_score = createLabel(_lv_screen, "Demo Mode", &lv_font_montserrat_14);
    lv_obj_set_style_text_color(_label_score, lv_color_hex(0x3498DB), 0);
    lv_obj_align(_label_score, LV_ALIGN_CENTER, 0, -10);
    
    // Instructions
    lv_obj_t *lbl_inst = createLabel(_lv_screen,
        "Accelerometer required.\nTilt to move the pet!", &lv_font_montserrat_10);
    lv_obj_set_style_text_color(lbl_inst, lv_color_hex(0x888888), 0);
    lv_obj_align(lbl_inst, LV_ALIGN_CENTER, 0, 20);
    
    // Demo button
    lv_obj_t *btn_demo = lv_btn_create(_lv_screen);
    lv_obj_set_size(btn_demo, 140, 40);
    lv_obj_align(btn_demo, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(btn_demo, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_radius(btn_demo, 10, 0);
    
    lv_obj_t *demo_lbl = lv_label_create(btn_demo);
    lv_label_set_text(demo_lbl, "Try Demo");
    lv_obj_set_style_text_font(demo_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(demo_lbl);
    
    lv_obj_add_event_cb(btn_demo, [](lv_event_t *e) {
        TiltGameScreen *self = (TiltGameScreen*)ScreenManager::getCurrentScreen();
        if (self) self->startDemo();
    }, LV_EVENT_CLICKED, this);
    
    // Back button
    _btn_back = createButton(_lv_screen, "Back", 60, 30, backHandler);
    lv_obj_align(_btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(_btn_back, lv_palette_main(LV_PALETTE_GREY), 0);
}

void TiltGameScreen::onEnter() {
    _score = 0;
    _game_active = false;
}

void TiltGameScreen::onExit() {
    _game_active = false;
}

void TiltGameScreen::startDemo() {
    _game_active = true;
    _score = 0;
    lv_label_set_text(_label_score, "Playing demo...");
    lv_obj_set_style_text_color(_label_score, lv_color_hex(0x2ECC71), 0);
    lv_label_set_text(_label_info, "Tilt your device!");
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0xF39C12), 0);
    
    // Demo timer - auto-end after 5 seconds
    lv_timer_t *timer = lv_timer_create(demoTimerCb, 5000, this);
    lv_timer_set_repeat_count(timer, 1);
}

void TiltGameScreen::endDemo() {
    _game_active = false;
    char buf[32];
    snprintf(buf, sizeof(buf), "Demo score: %d", _score);
    lv_label_set_text(_label_score, buf);
    lv_obj_set_style_text_color(_label_score, lv_color_hex(0x3498DB), 0);
    lv_label_set_text(_label_info, "Coming in Phase 21!");
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0xAAAAAA), 0);
    
    if (_result_cb) {
        _result_cb(_score >= 3, _score);
    }
}

void TiltGameScreen::backHandler(lv_event_t *e) {
    ScreenManager::handleBack();
}

void TiltGameScreen::demoTimerCb(lv_timer_t *timer) {
    TiltGameScreen *self = (TiltGameScreen *)lv_timer_get_user_data(timer);
    if (self) {
        self->_score = random(1, 6);  // Simulated score
        self->endDemo();
    }
    lv_timer_del(timer);
}
