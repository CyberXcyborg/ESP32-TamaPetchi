// ============================================================
// TiltGameScreen.h — Tilt game placeholder (accelerometer)
// Phase 20.4: Placeholder UI for Phase 21 accelerometer
// ============================================================

#ifndef TILT_GAME_SCREEN_H
#define TILT_GAME_SCREEN_H

#include "ScreenManager.h"

class TiltGameScreen : public Screen {
public:
    TiltGameScreen();
    
    void create() override;
    void onEnter() override;
    void onExit() override;
    
    typedef std::function<void(bool won, int score)> GameResultCallback;
    void setGameResultCallback(GameResultCallback cb) { _result_cb = cb; }

private:
    lv_obj_t *_label_title;
    lv_obj_t *_label_info;
    lv_obj_t *_label_score;
    lv_obj_t *_btn_back;
    
    int _score;
    bool _game_active;
    
    GameResultCallback _result_cb;
    
    static void backHandler(lv_event_t *e);
    static void demoTimerCb(lv_timer_t *timer);
    
    void startDemo();
    void endDemo();
};

#endif // TILT_GAME_SCREEN_H
