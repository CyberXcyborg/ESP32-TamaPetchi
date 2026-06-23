// lvgl.h — Minimal mock of LVGL for native unit tests
// Provides only the types needed by AnimationPlayer.h
#ifndef LVGL_H_MOCK
#define LVGL_H_MOCK

#include <stdint.h>
#include <stddef.h>

// Forward declarations
typedef struct _lv_timer_t lv_timer_t;
typedef struct _lv_obj_t lv_obj_t;

// Timer callback
typedef void (*lv_timer_cb_t)(lv_timer_t *timer);

// Timer functions (stubs for native)
inline lv_timer_t* lv_timer_create(lv_timer_cb_t cb, uint32_t period, void *user_data) {
    (void)cb; (void)period; (void)user_data;
    return nullptr;
}
inline void lv_timer_del(lv_timer_t *timer) { (void)timer; }
inline void lv_timer_pause(lv_timer_t *timer) { (void)timer; }
inline void lv_timer_resume(lv_timer_t *timer) { (void)timer; }
inline void lv_timer_set_period(lv_timer_t *timer, uint32_t period) { (void)timer; (void)period; }
inline void lv_timer_set_repeat_count(lv_timer_t *timer, uint16_t count) { (void)timer; (void)count; }

// Tick functions
inline void lv_tick_inc(uint32_t ms) { (void)ms; }
inline uint32_t lv_tick_get(void) { return 0; }

#endif // LVGL_H_MOCK
