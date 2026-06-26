// ============================================================
// ReactionGameScreen.h — Reaction time game
// ============================================================

#ifndef REACTION_GAME_SCREEN_H
#define REACTION_GAME_SCREEN_H

#include "ScreenManager.h"

class ReactionGameScreen : public Screen {
public:
    ReactionGameScreen();
    
    void create() override;
    void onEnter() override;
    void onExit() override;
    
    typedef std::function<void(bool won, int score)> GameResultCallback;
    void setGameResultCallback(GameResultCallback cb) { _result_cb = cb; }

private:
    lv_obj_t *_label_title;
    lv_obj_t *_label_info;
    lv_obj_t *_label_score;
    lv_obj_t *_bar_reaction;      // Filling bar
    lv_obj_t *_bar_green_zone;     // Target zone indicator
    lv_obj_t *_btn_tap;            // Main tap button
    lv_obj_t *_btn_back;
    
    // Game state
    int _score;
    int _currentRound;
    bool _game_active;
    bool _bar_filling;
    int _bar_direction;            // 1 = filling, -1 = emptying
    int _green_zone_low;
    int _green_zone_high;
    lv_timer_t *_game_timer;
    
    GameResultCallback _result_cb;
    
    static void tapHandler(lv_event_t *e);
    static void backHandler(lv_event_t *e);
    static void gameTimerCb(lv_timer_t *timer);
    
    void startRound();
    void onTap();
    void endGame(bool won);
};

#endif // REACTION_GAME_SCREEN_H
