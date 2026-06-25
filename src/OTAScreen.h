// ============================================================
// OTAScreen.h — OTA firmware update screen
// ============================================================

#ifndef OTA_SCREEN_H
#define OTA_SCREEN_H

#include "ScreenManager.h"

class OTAScreen : public Screen {
public:
    OTAScreen();
    
    void create() override;
    void onEnter() override;
    void onExit() override;
    
    // Set progress (0-100)
    void setProgress(int percent);
    void setStatus(const char *status);
    void setSuccess(bool success);
    void setError(const char *error);
    
    typedef std::function<void()> ConfirmCallback;
    void setConfirmCallback(ConfirmCallback cb) { _confirm_cb = cb; }

private:
    lv_obj_t *_label_title;
    lv_obj_t *_label_status;
    lv_obj_t *_bar_progress;
    lv_obj_t *_label_percent;
    lv_obj_t *_btn_update;
    lv_obj_t *_btn_back;
    
    ConfirmCallback _confirm_cb;
    
    static void updateHandler(lv_event_t *e);
    static void backHandler(lv_event_t *e);
    static void rebootTimerCb(lv_timer_t *timer);
};

#endif // OTA_SCREEN_H
