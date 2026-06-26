// ============================================================
// AnimationPlayer.h — Sprite Animation Engine for v2.0
// Non-blocking animation player using LVGL timers
// ============================================================

#ifndef ANIMATION_PLAYER_H
#define ANIMATION_PLAYER_H

#include <Arduino.h>
#include "config_v2.h"
#include "AnimTypes.h"
#include <functional>
#include <lvgl.h>

// Maximum concurrent animation players
#define ANIM_MAX_PLAYERS 4

// Animation completion callback
typedef std::function<void(const char* anim_name)> AnimCompleteCallback;
// Animation frame callback (for rendering)
typedef std::function<void(uint8_t frame_index)> AnimFrameCallback;

// AnimDefinition, AnimSet, AnimActionType are in AnimTypes.h

class AnimationPlayer {
public:
    static bool begin();
    
    // Play an animation by name
    // Returns player_id (1-based), or 0 on failure
    static uint8_t play(const AnimDefinition *anim, 
                        AnimCompleteCallback on_complete = nullptr,
                        AnimFrameCallback on_frame = nullptr);
    
    // Play animation for a specific pet action
    static uint8_t playForAction(uint8_t pet_stage, const char *action,
                                 AnimCompleteCallback on_complete = nullptr,
                                 AnimFrameCallback on_frame = nullptr);
    
    // Control
    static void pause(uint8_t player_id);
    static void resume(uint8_t player_id);
    static void stop(uint8_t player_id);
    static void stopAll();
    
    // State
    static AnimState getState(uint8_t player_id);
    static uint8_t getCurrentFrame(uint8_t player_id);
    static const char* getCurrentAnimName(uint8_t player_id);
    
    // Update (called from LVGL timer or main loop)
    static void update();
    
    // Set animation state machine callback
    typedef std::function<void(const char* from, const char* to)> AnimStateCallback;
    static void setStateCallback(AnimStateCallback cb) { _state_cb = cb; }

private:
    struct PlayerSlot {
        bool in_use;
        AnimState state;
        const AnimDefinition *anim;
        uint8_t current_frame;
        uint32_t frame_start_tick;
        uint32_t accumulated_ms;
        AnimCompleteCallback on_complete;
        AnimFrameCallback on_frame;
        struct _lv_timer_t *timer;
    };
    
    static PlayerSlot _players[ANIM_MAX_PLAYERS];
    static AnimStateCallback _state_cb;
    
    static PlayerSlot* findSlot(uint8_t player_id);
    static PlayerSlot* findFreeSlot();
    static void onTimerTick(lv_timer_t *timer);
    static void advanceFrame(PlayerSlot *slot);
    static void completeAnimation(PlayerSlot *slot);
    static uint16_t getFrameDuration(const AnimDefinition *anim, uint8_t frame_index);
};

// ============================================================
// Animation State Machine
// ============================================================

// AnimActionType is defined in AnimTypes.h

class AnimStateMachine {
public:
    AnimStateMachine();
    
    void init();
    
    // Request an action (may be rejected if invalid transition)
    bool requestAction(AnimActionType action);
    
    // Get current action
    AnimActionType getCurrentAction() const { return _current_action; }
    const char* getCurrentActionName() const;
    const char* getActionName(AnimActionType action) const;
    
    // Get the animation definition for current action
    const AnimDefinition* getCurrentAnim(uint8_t pet_stage) const;
    
    // Update (check for auto-transitions)
    void update();
    
    // Force state (for loading saved state)
    void forceState(AnimActionType state);
    
    // Check if action is valid from current state
    bool canTransition(AnimActionType action) const;

private:
    AnimActionType _current_action;
    AnimActionType _previous_action;
    uint32_t _action_start_tick;
    uint32_t _action_duration_ms;
    
    // Auto-return to idle after "once" animations
    bool _pending_idle;
    uint32_t _idle_return_tick;
    
    void transitionTo(AnimActionType action);
    bool isOnceAction(AnimActionType action) const;
    bool isLoopAction(AnimActionType action) const;
};

#endif // ANIMATION_PLAYER_H
