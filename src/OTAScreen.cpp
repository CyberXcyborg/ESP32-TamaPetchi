// ============================================================
// OTAScreen.cpp — OTA firmware update screen implementation
// ============================================================

#include "OTAScreen.h"
#include "config_v2.h"

OTAScreen::OTAScreen()
    : Screen("OTA"), _confirm_cb(nullptr) {
}

void OTAScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_clear_flag(_lv_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    _label_title = createLabel(_lv_screen, "Firmware Update", &lv_font_montserrat_16);
    lv_obj_align(_label_title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Status label
    _label_status = createLabel(_lv_screen, "Ready to update", &lv_font_montserrat_12);
    lv_obj_set_style_text_color(_label_status, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(_label_status, LV_ALIGN_TOP_MID, 0, 40);
    
    // Progress bar container
    lv_obj_t *bar_cont = lv_obj_create(_lv_screen);
    lv_obj_set_size(bar_cont, 200, 24);
    lv_obj_align(bar_cont, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(bar_cont, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(bar_cont, 6, 0);
    lv_obj_set_style_pad_all(bar_cont, 2, 0);
    
    // Progress bar
    _bar_progress = lv_bar_create(bar_cont);
    lv_obj_set_size(_bar_progress, 192, 16);
    lv_obj_center(_bar_progress);
    lv_bar_set_range(_bar_progress, 0, 100);
    lv_bar_set_value(_bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(_bar_progress, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_radius(_bar_progress, 4, 0);
    
    // Percent label
    _label_percent = createLabel(_lv_screen, "0%", &lv_font_montserrat_14);
    lv_obj_align(_label_percent, LV_ALIGN_CENTER, 0, 20);
    
    // Update button
    _btn_update = lv_btn_create(_lv_screen);
    lv_obj_set_size(_btn_update, 160, 44);
    lv_obj_align(_btn_update, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_bg_color(_btn_update, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_color(_btn_update, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_STATE_PRESSED);
    lv_obj_set_style_radius(_btn_update, 10, 0);
    lv_obj_add_event_cb(_btn_update, updateHandler, LV_EVENT_CLICKED, this);
    
    lv_obj_t *update_lbl = lv_label_create(_btn_update);
    lv_label_set_text(update_lbl, "Start Update");
    lv_obj_set_style_text_font(update_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(update_lbl);
    
    // Back button
    _btn_back = createButton(_lv_screen, "Back", 60, 30, backHandler);
    lv_obj_align(_btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(_btn_back, lv_palette_main(LV_PALETTE_GREY), 0);
}

void OTAScreen::onEnter() {
    setProgress(0);
    setStatus("Ready to update");
}

void OTAScreen::onExit() {
    // Nothing special
}

void OTAScreen::setProgress(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    lv_bar_set_value(_bar_progress, percent, LV_ANIM_ON);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(_label_percent, buf);
}

void OTAScreen::setStatus(const char *status) {
    lv_label_set_text(_label_status, status);
    lv_obj_set_style_text_color(_label_status, lv_color_hex(0xAAAAAA), 0);
}

void OTAScreen::setSuccess(bool success) {
    if (success) {
        lv_label_set_text(_label_status, "Update successful!");
        lv_obj_set_style_text_color(_label_status, lv_color_hex(0x2ECC71), 0);
        lv_obj_set_style_bg_color(_bar_progress, lv_color_hex(0x2ECC71), LV_PART_INDICATOR);
        
        // Show reboot countdown
        lv_label_set_text(_label_percent, "Rebooting...");
        
        // Schedule reboot after 3 seconds
        lv_timer_t *reboot_timer = lv_timer_create(rebootTimerCb, 3000, this);
        lv_timer_set_repeat_count(reboot_timer, 1);
    }
}

void OTAScreen::setError(const char *error) {
    lv_label_set_text(_label_status, error);
    lv_obj_set_style_text_color(_label_status, lv_color_hex(0xE74C3C), 0);
    lv_obj_set_style_bg_color(_bar_progress, lv_color_hex(0xE74C3C), LV_PART_INDICATOR);
}

void OTAScreen::updateHandler(lv_event_t *e) {
    OTAScreen *self = (OTAScreen *)lv_event_get_user_data(e);
    if (!self) return;
    
    self->setStatus("Uploading...");
    self->setProgress(0);
    
    if (self->_confirm_cb) {
        self->_confirm_cb();
    }
}

void OTAScreen::backHandler(lv_event_t *e) {
    ScreenManager::handleBack();
}

void OTAScreen::rebootTimerCb(lv_timer_t *timer) {
    // In real implementation: ESP.restart()
    DEBUG_PRINTF("[OTA] Would reboot now\\n");
    lv_timer_del(timer);
}
