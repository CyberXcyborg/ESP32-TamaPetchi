// ============================================================
// ScreenManager.cpp — LVGL Screen Manager Implementation
// ============================================================

#include "ScreenManager.h"

// Static members
ScreenManager::ScreenStackEntry ScreenManager::_stack[SCREEN_MANAGER_MAX_SCREENS];
uint8_t ScreenManager::_stack_depth = 0;
uint8_t ScreenManager::_cached_count = 0;
bool ScreenManager::_transitioning = false;

// ============================================================
// Screen Base Class
// ============================================================

Screen::Screen(const char *name)
    : _name(name), _lv_screen(nullptr) {
}

Screen::~Screen() {
    if (_lv_screen) {
        lv_obj_del(_lv_screen);
        _lv_screen = nullptr;
    }
}

void Screen::create() {
    if (_lv_screen) return;
    _lv_screen = lv_obj_create(nullptr);  // Create as screen (no parent)
    lv_obj_set_size(_lv_screen, 240, 240);
    lv_obj_clear_flag(_lv_screen, LV_OBJ_FLAG_SCROLLABLE);
}

void Screen::destroy() {
    if (_lv_screen) {
        lv_obj_del(_lv_screen);
        _lv_screen = nullptr;
    }
}

lv_obj_t* Screen::createContainer(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    return cont;
}

lv_obj_t* Screen::createLabel(lv_obj_t *parent, const char *text, const lv_font_t *font) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    return lbl;
}

lv_obj_t* Screen::createButton(lv_obj_t *parent, const char *text,
                                lv_coord_t w, lv_coord_t h,
                                lv_event_cb_t event_cb, void *user_data) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, user_data);
    
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    
    // Style
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE_GREY), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 8, 0);
    
    return btn;
}

lv_obj_t* Screen::createBar(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                             lv_color_t color) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, w, h);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_set_style_bg_color(bar, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 4, 0);
    return bar;
}

// ============================================================
// Screen Manager
// ============================================================

bool ScreenManager::begin() {
    memset(_stack, 0, sizeof(_stack));
    _stack_depth = 0;
    _cached_count = 0;
    _transitioning = false;
    return true;
}

void ScreenManager::pushScreen(Screen *screen, ScreenTransition trans) {
    if (!screen || _stack_depth >= SCREEN_MANAGER_MAX_SCREENS) return;
    
    // Pause current screen
    if (_stack_depth > 0 && _stack[_stack_depth - 1].active) {
        Screen *current = _stack[_stack_depth - 1].screen;
        current->onExit();
        _stack[_stack_depth - 1].active = false;
    }
    
    // Create new screen if needed
    if (!screen->isCreated()) {
        screen->create();
    }
    
    // Push to stack
    _stack[_stack_depth].screen = screen;
    _stack[_stack_depth].active = true;
    _stack_depth++;
    
    // Execute transition
    executeTransition(screen->getLvObj(), trans);
    
    // Enter new screen
    screen->onEnter();
    
    DEBUG_PRINTF("[Screen] Pushed '%s' (depth=%d)\n", screen->getName(), _stack_depth);
}

void ScreenManager::popScreen(ScreenTransition trans) {
    if (_stack_depth <= 1) return;  // Don't pop the root screen
    
    _transitioning = true;
    
    // Exit current screen
    Screen *current = _stack[_stack_depth - 1].screen;
    current->onExit();
    
    // Cache or destroy
    cacheScreen(current);
    
    _stack_depth--;
    
    // Resume previous screen
    Screen *prev = _stack[_stack_depth - 1].screen;
    _stack[_stack_depth - 1].active = true;
    
    executeTransition(prev->getLvObj(), trans);
    prev->onEnter();
    
    DEBUG_PRINTF("[Screen] Popped to '%s' (depth=%d)\n", prev->getName(), _stack_depth);
}

void ScreenManager::switchScreen(Screen *screen, ScreenTransition trans) {
    if (!screen || _stack_depth == 0) return;
    
    // Exit and cache current
    Screen *current = _stack[_stack_depth - 1].screen;
    current->onExit();
    cacheScreen(current);
    
    // Replace with new
    if (!screen->isCreated()) {
        screen->create();
    }
    
    _stack[_stack_depth - 1].screen = screen;
    _stack[_stack_depth - 1].active = true;
    
    executeTransition(screen->getLvObj(), trans);
    screen->onEnter();
    
    DEBUG_PRINTF("[Screen] Switched to '%s'\n", screen->getName());
}

void ScreenManager::popToRoot() {
    while (_stack_depth > 1) {
        popScreen(SCREEN_TRANS_NONE);
    }
}

Screen* ScreenManager::getCurrentScreen() {
    if (_stack_depth == 0) return nullptr;
    return _stack[_stack_depth - 1].screen;
}

const char* ScreenManager::getCurrentScreenName() {
    Screen *s = getCurrentScreen();
    return s ? s->getName() : "";
}

void ScreenManager::update() {
    if (_stack_depth > 0 && !_transitioning) {
        Screen *current = _stack[_stack_depth - 1].screen;
        if (current) current->onUpdate();
    }
}

void ScreenManager::handleBack() {
    if (_stack_depth > 1) {
        popScreen(SCREEN_TRANS_SLIDE_RIGHT);
    } else {
        // On root screen, could show exit confirmation
        DEBUG_PRINTF("[Screen] Back on root screen\n");
    }
}

// --- Private methods ---

void ScreenManager::executeTransition(lv_obj_t *new_screen, ScreenTransition trans) {
    if (trans == SCREEN_TRANS_NONE || _stack_depth <= 1) {
        lv_scr_load(new_screen);
        _transitioning = false;
        return;
    }
    
    _transitioning = true;
    
    lv_scr_load_anim_t anim;
    uint32_t duration = 300;
    
    switch (trans) {
        case SCREEN_TRANS_SLIDE_LEFT:
            anim = LV_SCR_LOAD_ANIM_MOVE_LEFT;
            break;
        case SCREEN_TRANS_SLIDE_RIGHT:
            anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
            break;
        case SCREEN_TRANS_FADE:
            anim = LV_SCR_LOAD_ANIM_FADE_ON;
            duration = 200;
            break;
        default:
            anim = LV_SCR_LOAD_ANIM_NONE;
            break;
    }
    
    lv_scr_load_anim(new_screen, anim, duration, 0, false);
    
    // Note: In a real implementation, we'd use lv_anim_t to detect completion
    // For simplicity, we use a delayed approach
    _transitioning = false;  // Simplified: assume instant completion
}

void ScreenManager::cacheScreen(Screen *screen) {
    if (!screen) return;
    
    // Keep the screen object alive for potential reuse
    // (In a full implementation, we'd manage a cache pool)
    // For now, just keep it created
}

Screen* ScreenManager::findCached(const char *name) {
    // Simplified: no caching for now
    (void)name;
    return nullptr;
}
