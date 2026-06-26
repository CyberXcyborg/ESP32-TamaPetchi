// ============================================================
// ScreenManager.h — LVGL Screen Manager for v2.0
// Stack-based screen navigation with transitions
// ============================================================

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>
#include <functional>

#define SCREEN_MANAGER_MAX_SCREENS 8
#define SCREEN_MANAGER_CACHE_SIZE  3

// Screen transition types
enum ScreenTransition {
    SCREEN_TRANS_NONE = 0,
    SCREEN_TRANS_SLIDE_LEFT,
    SCREEN_TRANS_SLIDE_RIGHT,
    SCREEN_TRANS_FADE
};

// Forward declaration
class Screen;

// Screen lifecycle callbacks
typedef std::function<void()> ScreenEnterCallback;
typedef std::function<void()> ScreenExitCallback;
typedef std::function<void()> ScreenUpdateCallback;

// Base screen class
class Screen {
public:
    Screen(const char *name);
    virtual ~Screen();
    
    // Lifecycle
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onUpdate() {}
    
    // Create the LVGL screen object (called on first push)
    virtual void create();
    
    // Destroy the LVGL screen object (called on pop or cache eviction)
    virtual void destroy();
    
    // Getters
    const char* getName() const { return _name; }
    lv_obj_t* getLvObj() const { return _lv_screen; }
    bool isCreated() const { return _lv_screen != nullptr; }
    
    // Input handling
    virtual void onButtonPress(uint8_t button) { (void)button; }
    virtual void onTouch(lv_point_t *point) { (void)point; }
    virtual void onSwipe(uint8_t direction) { (void)direction; }

protected:
    const char *_name;
    lv_obj_t *_lv_screen;
    
    // Helper to create a styled container
    lv_obj_t* createContainer(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
    
    // Helper to create a styled label
    lv_obj_t* createLabel(lv_obj_t *parent, const char *text, 
                          const lv_font_t *font = &lv_font_montserrat_12);
    
    // Helper to create a styled button
    lv_obj_t* createButton(lv_obj_t *parent, const char *text, 
                           lv_coord_t w, lv_coord_t h,
                           lv_event_cb_t event_cb, void *user_data = nullptr);
    
    // Helper to create a styled bar
    lv_obj_t* createBar(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, 
                        lv_color_t color);
};

// Screen Manager
class ScreenManager {
public:
    static bool begin();
    
    // Navigation
    static void pushScreen(Screen *screen, ScreenTransition trans = SCREEN_TRANS_SLIDE_LEFT);
    static void popScreen(ScreenTransition trans = SCREEN_TRANS_SLIDE_RIGHT);
    static void switchScreen(Screen *screen, ScreenTransition trans = SCREEN_TRANS_FADE);
    static void popToRoot();
    
    // Get current screen
    static Screen* getCurrentScreen();
    static const char* getCurrentScreenName();
    
    // Stack depth
    static uint8_t getStackDepth() { return _stack_depth; }
    
    // Update current screen (call from main loop)
    static void update();
    
    // Handle back button/gesture
    static void handleBack();
    
    // Memory stats
    static uint8_t getCachedCount() { return _cached_count; }

private:
    struct ScreenStackEntry {
        Screen *screen;
        bool active;
    };
    
    static ScreenStackEntry _stack[SCREEN_MANAGER_MAX_SCREENS];
    static uint8_t _stack_depth;
    static uint8_t _cached_count;
    static bool _transitioning;
    
    static void executeTransition(lv_obj_t *new_screen, ScreenTransition trans);
    static void transitionComplete(lv_anim_t *anim);
    static void cacheScreen(Screen *screen);
    static Screen* findCached(const char *name);
};

#endif // SCREEN_MANAGER_H
