// ============================================================
// MemoryGameScreen.cpp — Memory game implementation
// Player must repeat a sequence of button presses
// ============================================================

#include "MemoryGameScreen.h"
#include "config_v2.h"

// Button colors for each of the 16 grid positions
static const uint32_t BUTTON_COLORS[] = {
    0xE74C3C, 0x3498DB, 0x2ECC71, 0xF39C12,
    0x9B59B6, 0x1ABC9C, 0xE67E22, 0x2980B9,
    0x27AE60, 0xC0392B, 0x8E44AD, 0x16A085,
    0xD35400, 0x7F8C8D, 0x2C3E50, 0xF1C40F
};

MemoryGameScreen::MemoryGameScreen()
    : Screen("MemoryGame"),
      _current_round(0), _current_step(0), _score(0),
      _showing_sequence(false), _awaiting_input(false),
      _game_active(false), _input_index(0),
      _flash_index(0), _flash_timer(nullptr) {
    memset(_grid, 0, sizeof(_grid));
    memset(_sequence, 0, sizeof(_sequence));
}

void MemoryGameScreen::create() {
    Screen::create();
    
    lv_obj_set_style_bg_color(_lv_screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_clear_flag(_lv_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    _label_title = createLabel(_lv_screen, "Memory", &lv_font_montserrat_16);
    lv_obj_align(_label_title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Info label (shows round/status)
    _label_info = createLabel(_lv_screen, "Tap to start", &lv_font_montserrat_10);
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(_label_info, LV_ALIGN_TOP_MID, 0, 28);
    
    // Score label
    _label_score = createLabel(_lv_screen, "Score: 0", &lv_font_montserrat_12);
    lv_obj_align(_label_score, LV_ALIGN_TOP_LEFT, 10, 45);
    
    // Create 4x4 grid of buttons
    // Grid: 200x200 centered, each button 44x44 with 4px gap
    lv_obj_t *grid_cont = lv_obj_create(_lv_screen);
    lv_obj_set_size(grid_cont, 200, 200);
    lv_obj_align(grid_cont, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_bg_opa(grid_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_cont, 0, 0);
    lv_obj_set_style_pad_all(grid_cont, 0, 0);
    lv_obj_set_flex_flow(grid_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    for (int i = 0; i < 16; i++) {
        _grid[i] = lv_btn_create(grid_cont);
        lv_obj_set_size(_grid[i], 44, 44);
        lv_obj_set_style_bg_color(_grid[i], lv_color_hex(BUTTON_COLORS[i]), 0);
        lv_obj_set_style_bg_color(_grid[i], lv_color_hex(0xFFFFFF), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(_grid[i], LV_OPA_40, 0);  // Dimmed by default
        lv_obj_set_style_radius(_grid[i], 6, 0);
        lv_obj_set_style_border_width(_grid[i], 1, 0);
        lv_obj_set_style_border_color(_grid[i], lv_color_hex(0x555555), 0);
        lv_obj_add_event_cb(_grid[i], btnEventHandler, LV_EVENT_CLICKED, this);
        lv_obj_add_event_cb(_grid[i], btnEventHandler, LV_EVENT_PRESSED, this);
    }
    
    // Back button
    _btn_back = createButton(_lv_screen, "Back", 60, 30, backHandler);
    lv_obj_align(_btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(_btn_back, lv_palette_main(LV_PALETTE_GREY), 0);
}

void MemoryGameScreen::onEnter() {
    if (!_game_active) {
        startGame();
    }
}

void MemoryGameScreen::onExit() {
    if (_flash_timer) {
        lv_timer_del(_flash_timer);
        _flash_timer = nullptr;
    }
    _showing_sequence = false;
    _awaiting_input = false;
}

void MemoryGameScreen::startGame() {
    _current_round = 0;
    _current_step = 0;
    _score = 0;
    _game_active = true;
    _input_index = 0;
    
    generateSequence();
    updateInfoLabel();
    
    // Show sequence after a short delay
    _showing_sequence = true;
    lv_label_set_text(_label_info, "Watch...");
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0x3498DB), 0);
    
    // Start showing sequence after 800ms
    _flash_timer = lv_timer_create(flashTimerCb, 800, this);
    lv_timer_set_repeat_count(_flash_timer, 1);
}

void MemoryGameScreen::resetGame() {
    _game_active = false;
    _showing_sequence = false;
    _awaiting_input = false;
    if (_flash_timer) {
        lv_timer_del(_flash_timer);
        _flash_timer = nullptr;
    }
    lv_label_set_text(_label_info, "Tap to start");
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0xAAAAAA), 0);
}

void MemoryGameScreen::generateSequence() {
    // Use random seed from analog noise
    for (int i = 0; i < 10; i++) {
        _sequence[i] = random(0, 16);
    }
}

void MemoryGameScreen::showNextInSequence() {
    if (_current_step > _current_round) {
        // Done showing this round's sequence, now await input
        _showing_sequence = false;
        _awaiting_input = true;
        _input_index = 0;
        lv_label_set_text(_label_info, "Your turn!");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0x2ECC71), 0);
        
        // Undim all buttons
        for (int i = 0; i < 16; i++) {
            lv_obj_set_style_bg_opa(_grid[i], LV_OPA_100, 0);
        }
        return;
    }
    
    // Flash the next button in sequence
    _flash_index = _sequence[_current_step];
    flashButton(_flash_index);
    
    _current_step++;
    
    // Schedule next flash (600ms on, 300ms off = ~900ms per step)
    if (_flash_timer) {
        lv_timer_del(_flash_timer);
    }
    _flash_timer = lv_timer_create(flashTimerCb, 900, this);
    lv_timer_set_repeat_count(_flash_timer, 1);
}

void MemoryGameScreen::flashButton(uint8_t index) {
    if (index >= 16) return;
    
    // Bright flash
    lv_obj_set_style_bg_opa(_grid[index], LV_OPA_100, 0);
    lv_obj_set_style_border_width(_grid[index], 2, 0);
    lv_obj_set_style_border_color(_grid[index], lv_color_hex(0xFFFFFF), 0);
    
    // Schedule dim after 400ms using a one-shot timer
    lv_timer_t *dim = lv_timer_create([](lv_timer_t *t) {
        MemoryGameScreen *self = (MemoryGameScreen *)lv_timer_get_user_data(t);
        if (self && self->_flash_index < 16) {
            lv_obj_set_style_bg_opa(self->_grid[self->_flash_index], LV_OPA_40, 0);
            lv_obj_set_style_border_width(self->_grid[self->_flash_index], 1, 0);
            lv_obj_set_style_border_color(self->_grid[self->_flash_index], lv_color_hex(0x555555), 0);
        }
        lv_timer_del(t);
    }, 400, this);
    lv_timer_set_repeat_count(dim, 1);
}

void MemoryGameScreen::onButtonPressed(uint8_t index) {
    if (!_awaiting_input || _showing_sequence) return;
    
    // Brief visual feedback
    lv_obj_set_style_bg_opa(_grid[index], LV_OPA_100, 0);
    lv_obj_set_style_border_width(_grid[index], 2, 0);
    lv_obj_set_style_border_color(_grid[index], lv_color_hex(0xFFFFFF), 0);
    
    // Dim after 150ms - use a simple struct to pass both self and index
    typedef struct { MemoryGameScreen *self; uint8_t idx; } DimData;
    DimData *dd = new DimData{this, index};
    lv_timer_t *dim = lv_timer_create([](lv_timer_t *t) {
        DimData *dd = (DimData *)lv_timer_get_user_data(t);
        if (dd && dd->self && dd->idx < 16) {
            lv_obj_set_style_bg_opa(dd->self->_grid[dd->idx], LV_OPA_40, 0);
            lv_obj_set_style_border_width(dd->self->_grid[dd->idx], 1, 0);
            lv_obj_set_style_border_color(dd->self->_grid[dd->idx], lv_color_hex(0x555555), 0);
        }
        delete dd;
        lv_timer_del(t);
    }, 150, dd);
    lv_timer_set_repeat_count(dim, 1);
    
    // Check if correct
    if (index == _sequence[_input_index]) {
        _input_index++;
        _score++;
        
        // Update score display
        char buf[32];
        snprintf(buf, sizeof(buf), "Score: %d", _score);
        lv_label_set_text(_label_score, buf);
        
        // Check if round complete
        if (_input_index > _current_round) {
            advanceRound();
        }
    } else {
        // Wrong! Game over
        endGame(false);
    }
}

void MemoryGameScreen::advanceRound() {
    _current_round++;
    _current_step = 0;
    _input_index = 0;
    
    if (_current_round >= 10) {
        endGame(true);
        return;
    }
    
    // Show next round's sequence
    _showing_sequence = true;
    _awaiting_input = false;
    lv_label_set_text(_label_info, "Watch...");
    lv_obj_set_style_text_color(_label_info, lv_color_hex(0x3498DB), 0);
    
    // Dim all buttons
    for (int i = 0; i < 16; i++) {
        lv_obj_set_style_bg_opa(_grid[i], LV_OPA_40, 0);
    }
    
    // Start showing after 600ms
    if (_flash_timer) {
        lv_timer_del(_flash_timer);
    }
    _flash_timer = lv_timer_create(flashTimerCb, 600, this);
    lv_timer_set_repeat_count(_flash_timer, 1);
}

void MemoryGameScreen::endGame(bool won) {
    _game_active = false;
    _showing_sequence = false;
    _awaiting_input = false;
    
    if (_flash_timer) {
        lv_timer_del(_flash_timer);
        _flash_timer = nullptr;
    }
    
    if (won) {
        lv_label_set_text(_label_info, "You win!");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0x2ECC71), 0);
    } else {
        lv_label_set_text(_label_info, "Wrong! Game over");
        lv_obj_set_style_text_color(_label_info, lv_color_hex(0xE74C3C), 0);
    }
    
    // Flash all buttons to indicate end
    for (int i = 0; i < 16; i++) {
        lv_obj_set_style_bg_opa(_grid[i], won ? LV_OPA_100 : LV_OPA_20, 0);
    }
    
    // Report result
    if (_result_cb) {
        _result_cb(won, _score);
    }
    
    // Auto-return to game menu after 2 seconds
    lv_timer_t *return_timer = lv_timer_create([](lv_timer_t *t) {
        ScreenManager::handleBack();
        lv_timer_del(t);
    }, 2000, nullptr);
    lv_timer_set_repeat_count(return_timer, 1);
}

void MemoryGameScreen::updateInfoLabel() {
    char buf[32];
    snprintf(buf, sizeof(buf), "Round: %d/10", _current_round + 1);
    lv_label_set_text(_label_info, buf);
}

void MemoryGameScreen::btnEventHandler(lv_event_t *e) {
    MemoryGameScreen *self = (MemoryGameScreen *)lv_event_get_user_data(e);
    if (!self) return;
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    
    // Find which button was pressed
    int index = -1;
    for (int i = 0; i < 16; i++) {
        if (self->_grid[i] == btn) {
            index = i;
            break;
        }
    }
    
    if (index < 0) return;
    
    if (code == LV_EVENT_PRESSED && !self->_awaiting_input) {
        // Tap to start when not playing
        if (!self->_game_active) {
            self->startGame();
        }
    } else if (code == LV_EVENT_CLICKED && self->_awaiting_input) {
        self->onButtonPressed(index);
    }
}

void MemoryGameScreen::backHandler(lv_event_t *e) {
    ScreenManager::handleBack();
}

void MemoryGameScreen::flashTimerCb(lv_timer_t *timer) {
    MemoryGameScreen *self = (MemoryGameScreen *)lv_timer_get_user_data(timer);
    if (!self) {
        lv_timer_del(timer);
        return;
    }
    
    if (self->_showing_sequence) {
        self->showNextInSequence();
    }
}
