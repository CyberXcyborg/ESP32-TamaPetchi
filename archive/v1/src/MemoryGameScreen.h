// ============================================================
// MemoryGameScreen.h — Memory game with 4x4 button grid
// ============================================================

#ifndef MEMORY_GAME_SCREEN_H
#define MEMORY_GAME_SCREEN_H

#include "ScreenManager.h"
#include "Pet_v2.h"
using namespace PetV2;


class MemoryGameScreen : public Screen {
public:
    MemoryGameScreen();
    
    void create() override;
    void onEnter() override;
    void onExit() override;
    
    // Game state
    void startGame();
    void resetGame();
    
    // Callback for game result
    typedef std::function<void(bool won, int score)> GameResultCallback;
    void setGameResultCallback(GameResultCallback cb) { _result_cb = cb; }

private:
    // UI elements
    lv_obj_t *_grid[16];         // 4x4 button grid
    lv_obj_t *_label_title;
    lv_obj_t *_label_info;       // Shows current step / status
    lv_obj_t *_label_score;
    lv_obj_t *_btn_back;
    
    // Game state
    uint8_t _sequence[10];       // Memory sequence (values 0-15 for grid positions)
    uint8_t _current_round;       // Current position in sequence
    uint8_t _current_step;        // Step within current round (showing vs input)
    uint8_t _score;
    bool _showing_sequence;       // Currently showing sequence to player
    bool _awaiting_input;         // Waiting for player input
    bool _game_active;
    uint8_t _input_index;         // Which input we're waiting for
    
    // Visual feedback
    uint8_t _flash_index;         // Which button is currently flashing
    lv_timer_t *_flash_timer;     // Timer for sequence display
    
    // Callback
    GameResultCallback _result_cb;
    
    // Event handlers
    static void btnEventHandler(lv_event_t *e);
    static void backHandler(lv_event_t *e);
    static void flashTimerCb(lv_timer_t *timer);
    
    // Game logic
    void generateSequence();
    void showNextInSequence();
    void flashButton(uint8_t index);
    void onButtonPressed(uint8_t index);
    void advanceRound();
    void endGame(bool won);
    void updateInfoLabel();
    uint8_t getButtonColor(uint8_t index);
};

#endif // MEMORY_GAME_SCREEN_H
