// ============================================================
// AnimationPlayer.cpp — Animation Engine Implementation
// ============================================================

#include "AnimationPlayer.h"
#include "animations.h"

// Static members
AnimationPlayer::PlayerSlot AnimationPlayer::_players[ANIM_MAX_PLAYERS];
AnimationPlayer::AnimStateCallback AnimationPlayer::_state_cb;

bool AnimationPlayer::begin() {
    memset(_players, 0, sizeof(_players));
    _state_cb = nullptr;
    return true;
}

uint8_t AnimationPlayer::play(const AnimDefinition *anim,
                                AnimCompleteCallback on_complete,
                                AnimFrameCallback on_frame) {
    if (!anim || anim->frame_count == 0) return 0;
    
    PlayerSlot *slot = findFreeSlot();
    if (!slot) return 0;  // No free slots
    
    uint8_t id = (slot - _players) + 1;
    
    slot->in_use = true;
    slot->state = ANIM_PLAYING;
    slot->anim = anim;
    slot->current_frame = 0;
    slot->frame_start_tick = millis();
    slot->accumulated_ms = 0;
    slot->on_complete = on_complete;
    slot->on_frame = on_frame;
    
    // Create LVGL timer for this animation
    uint16_t period = getFrameDuration(anim, 0);
    slot->timer = lv_timer_create(onTimerTick, period, slot);
    lv_timer_set_repeat_count(slot->timer, 1);  // One-shot, we manage repeats
    
    // Fire initial frame callback
    if (slot->on_frame) {
        slot->on_frame(0);
    }
    
    DEBUG_PRINTF("[Anim] Playing '%s' (%d frames, %dms period, id=%d)\n",
                 anim->name, anim->frame_count, period, id);
    
    return id;
}

uint8_t AnimationPlayer::playForAction(uint8_t pet_stage, const char *action,
                                        AnimCompleteCallback on_complete,
                                        AnimFrameCallback on_frame) {
    const AnimSet *set = getAnimSet(pet_stage);
    if (!set) return 0;
    
    AnimActionType action_type = ACTION_NONE;
    if (strcmp(action, "idle") == 0) action_type = ACTION_NONE;
    else if (strcmp(action, "eat") == 0) action_type = ACTION_FEED;
    else if (strcmp(action, "play") == 0) action_type = ACTION_PLAY;
    else if (strcmp(action, "sleep") == 0) action_type = ACTION_SLEEP;
    else if (strcmp(action, "sick") == 0) action_type = ACTION_HEAL;
    else if (strcmp(action, "happy") == 0) action_type = ACTION_HAPPY;
    else if (strcmp(action, "evolve") == 0) action_type = ACTION_EVOLVE;
    else if (strcmp(action, "clean") == 0) action_type = ACTION_CLEAN;
    else if (strcmp(action, "wake") == 0) action_type = ACTION_WAKE;
    
    const AnimDefinition *anim = getAnimForAction(set, action_type);
    if (!anim) {
        DEBUG_PRINTF("[Anim] No animation for action '%s' stage %d\n", action, pet_stage);
        return 0;
    }
    
    return play(anim, on_complete, on_frame);
}

void AnimationPlayer::pause(uint8_t player_id) {
    PlayerSlot *slot = findSlot(player_id);
    if (!slot || slot->state != ANIM_PLAYING) return;
    
    slot->state = ANIM_PAUSED;
    if (slot->timer) {
        lv_timer_pause(slot->timer);
    }
    DEBUG_PRINTF("[Anim] Paused '%s' (id=%d)\n", slot->anim->name, player_id);
}

void AnimationPlayer::resume(uint8_t player_id) {
    PlayerSlot *slot = findSlot(player_id);
    if (!slot || slot->state != ANIM_PAUSED) return;
    
    slot->state = ANIM_PLAYING;
    slot->frame_start_tick = millis();
    if (slot->timer) {
        lv_timer_resume(slot->timer);
    }
    DEBUG_PRINTF("[Anim] Resumed '%s' (id=%d)\n", slot->anim->name, player_id);
}

void AnimationPlayer::stop(uint8_t player_id) {
    PlayerSlot *slot = findSlot(player_id);
    if (!slot) return;
    
    if (slot->timer) {
        lv_timer_del(slot->timer);
    }
    
    DEBUG_PRINTF("[Anim] Stopped '%s' (id=%d)\n",
                 slot->anim ? slot->anim->name : "none", player_id);
    
    slot->in_use = false;
    slot->state = ANIM_STOPPED;
    slot->anim = nullptr;
    slot->timer = nullptr;
    slot->on_complete = nullptr;
    slot->on_frame = nullptr;
}

void AnimationPlayer::stopAll() {
    for (int i = 0; i < ANIM_MAX_PLAYERS; i++) {
        if (_players[i].in_use) {
            stop(i + 1);
        }
    }
}

AnimState AnimationPlayer::getState(uint8_t player_id) {
    PlayerSlot *slot = findSlot(player_id);
    return slot ? slot->state : ANIM_STOPPED;
}

uint8_t AnimationPlayer::getCurrentFrame(uint8_t player_id) {
    PlayerSlot *slot = findSlot(player_id);
    return slot ? slot->current_frame : 0;
}

const char* AnimationPlayer::getCurrentAnimName(uint8_t player_id) {
    PlayerSlot *slot = findSlot(player_id);
    return (slot && slot->anim) ? slot->anim->name : "";
}

void AnimationPlayer::update() {
    // Called from main loop as backup (LVGL timers handle most work)
    for (int i = 0; i < ANIM_MAX_PLAYERS; i++) {
        PlayerSlot *slot = &_players[i];
        if (!slot->in_use || slot->state != ANIM_PLAYING) continue;
        
        uint32_t now = millis();
        uint16_t frame_dur = getFrameDuration(slot->anim, slot->current_frame);
        
        if (now - slot->frame_start_tick >= frame_dur) {
            advanceFrame(slot);
            slot->frame_start_tick = now;
        }
    }
}

// --- Private methods ---

AnimationPlayer::PlayerSlot* AnimationPlayer::findSlot(uint8_t player_id) {
    uint8_t idx = player_id - 1;
    if (idx >= ANIM_MAX_PLAYERS || !_players[idx].in_use) return nullptr;
    return &_players[idx];
}

AnimationPlayer::PlayerSlot* AnimationPlayer::findFreeSlot() {
    for (int i = 0; i < ANIM_MAX_PLAYERS; i++) {
        if (!_players[i].in_use) return &_players[i];
    }
    return nullptr;
}

void AnimationPlayer::onTimerTick(lv_timer_t *timer) {
    PlayerSlot *slot = (PlayerSlot*)lv_timer_get_user_data(timer);
    if (!slot || !slot->in_use) return;
    
    advanceFrame(slot);
    
    // Reset timer for next frame
    if (slot->state == ANIM_PLAYING) {
        uint16_t next_dur = getFrameDuration(slot->anim, slot->current_frame);
        lv_timer_set_period(timer, next_dur);
    }
}

void AnimationPlayer::advanceFrame(PlayerSlot *slot) {
    if (!slot || !slot->anim) return;
    
    slot->current_frame++;
    
    if (slot->current_frame >= slot->anim->frame_count) {
        if (slot->anim->loop) {
            // Loop back to start
            slot->current_frame = 0;
            if (slot->on_frame) {
                slot->on_frame(0);
            }
        } else {
            // Animation complete
            completeAnimation(slot);
        }
    } else {
        // Normal frame advance
        if (slot->on_frame) {
            slot->on_frame(slot->current_frame);
        }
    }
}

void AnimationPlayer::completeAnimation(PlayerSlot *slot) {
    // Stay on last frame
    slot->current_frame = slot->anim->frame_count - 1;
    
    DEBUG_PRINTF("[Anim] Completed '%s'\n", slot->anim->name);
    
    // Fire completion callback
    if (slot->on_complete) {
        slot->on_complete(slot->anim->name);
    }
    
    // For non-looping animations, stop after completion
    if (!slot->anim->loop) {
        slot->state = ANIM_STOPPED;
        if (slot->timer) {
            lv_timer_pause(slot->timer);
        }
    }
}

uint16_t AnimationPlayer::getFrameDuration(const AnimDefinition *anim, uint8_t frame_index) {
    if (!anim) return 100;
    
    if (anim->frame_durations && frame_index < anim->frame_count) {
        return anim->frame_durations[frame_index];
    }
    
    // Fallback: compute from FPS
    if (anim->fps > 0) {
        return 1000 / anim->fps;
    }
    
    return 100;  // Default 100ms
}

// ============================================================
// Animation State Machine Implementation
// ============================================================

AnimStateMachine::AnimStateMachine()
    : _current_action(ACTION_NONE),
      _previous_action(ACTION_NONE),
      _action_start_tick(0),
      _action_duration_ms(0),
      _pending_idle(false),
      _idle_return_tick(0) {
}

void AnimStateMachine::init() {
    _current_action = ACTION_NONE;
    _previous_action = ACTION_NONE;
    _action_start_tick = millis();
    _action_duration_ms = 0;
    _pending_idle = false;
    _idle_return_tick = 0;
}

bool AnimStateMachine::requestAction(AnimActionType action) {
    if (!canTransition(action)) {
        DEBUG_PRINTF("[AnimSM] Rejected: %s -> %s\n",
                     getCurrentActionName(), getActionName(action));
        return false;
    }
    
    DEBUG_PRINTF("[AnimSM] Transition: %s -> %s\n",
                 getCurrentActionName(), getActionName(action));
    
    transitionTo(action);
    return true;
}

const char* AnimStateMachine::getCurrentActionName() const {
    return getActionName(_current_action);
}

const char* AnimStateMachine::getActionName(AnimActionType action) const {
    switch (action) {
        case ACTION_NONE:  return "idle";
        case ACTION_FEED:  return "eat";
        case ACTION_PLAY:  return "play";
        case ACTION_SLEEP: return "sleep";
        case ACTION_WAKE:  return "wake";
        case ACTION_HEAL:  return "heal";
        case ACTION_HAPPY: return "happy";
        case ACTION_EVOLVE:return "evolve";
        case ACTION_CLEAN: return "clean";
        default:           return "unknown";
    }
}

const AnimDefinition* AnimStateMachine::getCurrentAnim(uint8_t pet_stage) const {
    const AnimSet *set = getAnimSet(pet_stage);
    return getAnimForAction(set, _current_action);
}

void AnimStateMachine::update() {
    uint32_t now = millis();
    
    // Check for pending idle return
    if (_pending_idle && now >= _idle_return_tick) {
        _pending_idle = false;
        if (isOnceAction(_current_action)) {
            transitionTo(ACTION_NONE);
        }
    }
    
    // Auto-wake from sleep after duration
    if (_current_action == ACTION_SLEEP) {
        // Sleep continues until explicitly woken or pet stat changes
        // (handled by pet engine, not animation SM)
    }
}

void AnimStateMachine::forceState(AnimActionType state) {
    _current_action = state;
    _action_start_tick = millis();
    _pending_idle = false;
}

bool AnimStateMachine::canTransition(AnimActionType action) const {
    // SICK overrides everything
    if (_current_action == ACTION_HEAL && action != ACTION_HEAL) {
        return false;  // Can't do anything while sick except heal
    }
    
    // Can't eat while sleeping
    if (_current_action == ACTION_SLEEP && action == ACTION_FEED) {
        return false;
    }
    
    // Can't play while sleeping
    if (_current_action == ACTION_SLEEP && action == ACTION_PLAY) {
        return false;
    }
    
    // Can't sleep while already sleeping
    if (_current_action == ACTION_SLEEP && action == ACTION_SLEEP) {
        return false;
    }
    
    // Can't evolve unless adult
    if (action == ACTION_EVOLVE && _current_action != ACTION_NONE) {
        return false;  // Must be idle to evolve
    }
    
    return true;
}

void AnimStateMachine::transitionTo(AnimActionType action) {
    _previous_action = _current_action;
    _current_action = action;
    _action_start_tick = millis();
    
    if (isOnceAction(action)) {
        _pending_idle = true;
        _idle_return_tick = millis() + 2000;  // Return to idle after 2s
    } else {
        _pending_idle = false;
    }
}

bool AnimStateMachine::isOnceAction(AnimActionType action) const {
    return action == ACTION_FEED || action == ACTION_HAPPY ||
           action == ACTION_EVOLVE || action == ACTION_CLEAN ||
           action == ACTION_HEAL;
}

bool AnimStateMachine::isLoopAction(AnimActionType action) const {
    return action == ACTION_NONE || action == ACTION_PLAY ||
           action == ACTION_SLEEP || action == ACTION_HEAL;
}
