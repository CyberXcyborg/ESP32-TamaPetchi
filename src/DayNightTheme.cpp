#include "DayNightTheme.h"
#include "AppState.h"
#include "drivers/DisplayDriver.h"
#include <ArduinoJson.h>

#ifdef ESP32
#include <sntp.h>
#include <esp_wifi.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#endif

// Forward declaration (defined below)
static DayNightTheme timeToTheme(uint16_t minutes);

// ============================================================
// Theme color palettes
// ============================================================
static const ThemePalette themePalettes[THEME_COUNT] = {
    // THEME_DAWN — warm orange/pink
    { 0xFFE4C4, 0x4A3728, 0xFF8C42, 0xFFB347, 70 },
    // THEME_DAY — bright blue/white
    { 0x87CEEB, 0x1A1A2E, 0x4FC3F7, 0xFFFFFF, 100 },
    // THEME_DUSK — purple/orange sunset
    { 0x4A2C6E, 0xE8D5B7, 0xFF6B6B, 0xC9A8FF, 60 },
    // THEME_NIGHT — dark blue/silver
    { 0x0D1B2A, 0x8899AA, 0x4488CC, 0x667788, 40 },
};

// ============================================================
// Module-level state
// ============================================================
static DayNightState dnState = {
    .currentTheme = THEME_DAY,
    .targetTheme = THEME_DAY,
    .currentWeather = WEATHER_NONE,
    .ambientLightLevel = 50,
    .autoBrightness = true,
    .autoTheme = true,
    .lastTransitionMs = 0,
    .transitionProgress = 1.0f
};

static const unsigned long TRANSITION_DURATION_MS = 5000; // 5 seconds

// ============================================================
// Theme change observer pattern
// ============================================================
#define MAX_THEME_CALLBACKS 8
static ThemeChangeCallback themeCallbacks[MAX_THEME_CALLBACKS];
static uint8_t themeCallbackCount = 0;

void registerThemeChangeCallback(ThemeChangeCallback cb) {
    if (themeCallbackCount < MAX_THEME_CALLBACKS) {
        themeCallbacks[themeCallbackCount++] = cb;
    }
}

static void notifyThemeChange(DayNightTheme oldTheme, DayNightTheme newTheme) {
    for (uint8_t i = 0; i < themeCallbackCount; i++) {
        if (themeCallbacks[i]) {
            themeCallbacks[i](oldTheme, newTheme);
        }
    }
}

// ============================================================
// Rain particle overlay state
// ============================================================
#ifdef ESP32
static bool rainActive = false;
static uint32_t rainLastFrame = 0;
#define RAIN_PARTICLE_COUNT 20
struct RainParticle {
    int16_t x;
    int16_t y;
    uint8_t speed; // 1-5
};
static RainParticle rainParticles[RAIN_PARTICLE_COUNT];
#endif

void startRainOverlay() {
#ifdef ESP32
    rainActive = true;
    rainLastFrame = millis();
    // Initialize particles at random positions across the top of screen
    for (uint8_t i = 0; i < RAIN_PARTICLE_COUNT; i++) {
        rainParticles[i].x = rand() % TFT_WIDTH;
        rainParticles[i].y = rand() % TFT_HEIGHT;
        rainParticles[i].speed = 1 + (rand() % 5);
    }
#endif
}

void stopRainOverlay() {
#ifdef ESP32
    rainActive = false;
#endif
}

// ============================================================
// LVGL Theme Application
// ============================================================
#ifdef ESP32
#include <lvgl.h>
#endif

void applyThemeToLVGL() {
#ifdef ESP32
    ThemePalette pal = getCurrentPalette();
    lv_color_t bgColor = lv_color_make((pal.bg_color >> 16) & 0xFF,
                                        (pal.bg_color >> 8) & 0xFF,
                                        pal.bg_color & 0xFF);

    // Apply to current screen background
    lv_obj_t *screen = lv_scr_act();
    if (screen) {
        lv_obj_set_style_bg_color(screen, bgColor, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    }
#endif
    // No-op in non-ESP32 builds (native tests)
}

// ============================================================
// SNTP Time Sync
// ============================================================
#ifdef ESP32
static bool sntpInitialized = false;
static void sntp_time_sync_cb(struct timeval *tv) {
    if (tv) {
        struct tm timeinfo;
        localtime_r(&tv->tv_sec, &timeinfo);
        uint16_t minutesSinceMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        updateThemeByTime(minutesSinceMidnight);
        Serial.printf("[DayNightTheme] Time sync: %02d:%02d (theme=%d)\n",
                       timeinfo.tm_hour, timeinfo.tm_min, timeToTheme(minutesSinceMidnight));
    }
}
#endif

void syncTimeFromNTP() {
#ifdef ESP32
    if (!sntpInitialized) {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_setservername(1, "time.google.com");
        sntp_set_time_sync_notification_cb(sntp_time_sync_cb);
        sntp_init();
        sntpInitialized = true;
        Serial.println("[DayNightTheme] SNTP initialized");
    }

    // Check if we have valid time
    time_t now;
    time(&now);
    if (now < 1000000000) {
        // Time not synced yet — default to day theme
        updateThemeByTime(0);
        Serial.println("[DayNightTheme] Waiting for NTP sync, defaulting to day theme");
    } else {
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        uint16_t minutesSinceMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        updateThemeByTime(minutesSinceMidnight);
    }
#else
    // Native/test stub: no NTP available, call with 0
    updateThemeByTime(0);
#endif
}

// ============================================================
// Ambient Light Sensor ADC
// ============================================================
#ifdef ESP32
#define AMBIENT_LIGHT_ADC_PIN 3  // GPIO3 (ADC1_CH2) — dedicated light sensor pin
                                        // (BATTERY_ADC_PIN=1 is used for battery)
#define AMBIENT_LIGHT_SAMPLES 4   // Oversampling for noise reduction
#endif

void pollAmbientLightSensor() {
#ifdef ESP32
    // Configure ADC for light sensor reading using ESP-IDF ADC driver
    uint32_t adcAvg = 0;
    for (int i = 0; i < AMBIENT_LIGHT_SAMPLES; i++) {
        // Simple digital readout stub for v2 — actual ADC driver would use esp_adc_cal
        adcAvg += 2048; // midpoint placeholder
    }
    adcAvg /= AMBIENT_LIGHT_SAMPLES;

    // Map ADC 0-4095 to 0-100 percentage
    uint8_t lightLevel = constrain(adcAvg * 100 / 4095, 0, 100);
    setAmbientLightLevel(lightLevel);

    // Auto-brightness: adjust display
    if (dnState.autoBrightness) {
        uint8_t brightness = calculateAutoBrightness();
        setDisplayBrightness(brightness);
        DisplayDriver::getInstance().setBrightness(brightness);
    }
#else
    // Native/test stub: no-op
    (void)0;
#endif
}

// ============================================================
// Display Brightness (connects to DisplayDriver)
// ============================================================
void setDisplayBrightness(uint8_t level) {
    DisplayDriver::getInstance().setBrightness(level);
    Serial.printf("[DayNightTheme] Brightness set to %d%%\n", level);
}

// ============================================================
// Time-to-theme mapping
// ============================================================
static DayNightTheme timeToTheme(uint16_t minutes) {
    // Dawn: 5:00 - 7:00 (300-420)
    // Day: 7:00 - 18:00 (420-1080)
    // Dusk: 18:00 - 20:00 (1080-1200)
    // Night: 20:00 - 5:00 (1200-300)
    if (minutes >= 300 && minutes < 420) return THEME_DAWN;
    if (minutes >= 420 && minutes < 1080) return THEME_DAY;
    if (minutes >= 1080 && minutes < 1200) return THEME_DUSK;
    return THEME_NIGHT;
}

// ============================================================
// Public API
// ============================================================

void initDayNightTheme() {
    dnState.currentTheme = THEME_DAY;
    dnState.targetTheme = THEME_DAY;
    dnState.currentWeather = WEATHER_NONE;
    dnState.ambientLightLevel = 50;
    dnState.autoBrightness = true;
    dnState.autoTheme = true;
    dnState.transitionProgress = 1.0f;

    // Initialize DisplayDriver backlight
    DisplayDriver::getInstance().begin();

    // Initialize theme callbacks
    themeCallbackCount = 0;

    Serial.println("[DayNightTheme] Initialized");
}

void updateThemeByTime(uint16_t minutesSinceMidnight) {
    DayNightTheme newTheme = timeToTheme(minutesSinceMidnight);
    if (newTheme != dnState.targetTheme) {
        DayNightTheme oldTheme = dnState.targetTheme;
        dnState.targetTheme = newTheme;
        dnState.lastTransitionMs = millis();
        dnState.transitionProgress = 0.0f;
        Serial.printf("[DayNightTheme] Transitioning to theme %d\n", newTheme);

        // Notify observers
        notifyThemeChange(oldTheme, newTheme);

        // Apply to LVGL
        applyThemeToLVGL();
    }
}

void setTheme(DayNightTheme theme) {
    if (theme < THEME_COUNT && theme != dnState.targetTheme) {
        DayNightTheme oldTheme = dnState.targetTheme;
        dnState.targetTheme = theme;
        dnState.lastTransitionMs = millis();
        dnState.transitionProgress = 0.0f;
        notifyThemeChange(oldTheme, theme);
        applyThemeToLVGL();
    }
}

DayNightTheme getCurrentTheme() {
    return dnState.currentTheme;
}

DayNightTheme getTargetTheme() {
    return dnState.targetTheme;
}

ThemePalette getCurrentPalette() {
    // If transitioning, interpolate between current and target
    if (dnState.transitionProgress < 1.0f && dnState.currentTheme != dnState.targetTheme) {
        const ThemePalette &from = themePalettes[dnState.currentTheme];
        const ThemePalette &to = themePalettes[dnState.targetTheme];
        float t = dnState.transitionProgress;

        ThemePalette result;
        // Simple linear interpolation for colors
        uint8_t r1 = (from.bg_color >> 16) & 0xFF, g1 = (from.bg_color >> 8) & 0xFF, b1 = from.bg_color & 0xFF;
        uint8_t r2 = (to.bg_color >> 16) & 0xFF, g2 = (to.bg_color >> 8) & 0xFF, b2 = to.bg_color & 0xFF;
        result.bg_color = ((uint8_t)(r1 + (r2 - r1) * t) << 16) |
                          ((uint8_t)(g1 + (g2 - g1) * t) << 8) |
                          (uint8_t)(b1 + (b2 - b1) * t);

        result.text_color = from.text_color; // Keep text readable
        result.accent_color = from.accent_color;
        result.pet_color = from.pet_color;
        result.brightness = from.brightness + (int8_t)((int8_t)to.brightness - (int8_t)from.brightness) * t;
        return result;
    }
    return themePalettes[dnState.currentTheme];
}

ThemePalette getPaletteForTheme(DayNightTheme theme) {
    if (theme < THEME_COUNT) return themePalettes[theme];
    return themePalettes[THEME_DAY];
}

void setWeatherEffect(WeatherEffect effect) {
    dnState.currentWeather = effect;
    Serial.printf("[DayNightTheme] Weather set to %d\n", effect);

    // Start/stop rain overlay
    if (effect == WEATHER_RAIN) {
        startRainOverlay();
    } else {
        stopRainOverlay();
    }
}

WeatherEffect getCurrentWeatherEffect() {
    return dnState.currentWeather;
}

void setAmbientLightLevel(uint8_t level) {
    dnState.ambientLightLevel = constrain(level, 0, 100);
}

uint8_t getAmbientLightLevel() {
    return dnState.ambientLightLevel;
}

uint8_t calculateAutoBrightness() {
    // Map ambient light to display brightness
    // Low ambient = low brightness, high ambient = high brightness
    if (dnState.ambientLightLevel < 10) return 20;
    if (dnState.ambientLightLevel < 30) return 40;
    if (dnState.ambientLightLevel < 50) return 60;
    if (dnState.ambientLightLevel < 70) return 80;
    return 100;
}

void setAutoBrightness(bool enabled) {
    dnState.autoBrightness = enabled;
}

bool isAutoBrightnessEnabled() {
    return dnState.autoBrightness;
}

void setAutoTheme(bool enabled) {
    dnState.autoTheme = enabled;
}

bool isAutoThemeEnabled() {
    return dnState.autoTheme;
}

bool isTransitioning() {
    return dnState.transitionProgress < 1.0f;
}

float getTransitionProgress() {
    return dnState.transitionProgress;
}

String getDayNightStateJson() {
    DynamicJsonDocument doc(256);
    doc["currentTheme"] = (int)dnState.currentTheme;
    doc["targetTheme"] = (int)dnState.targetTheme;
    doc["weather"] = (int)dnState.currentWeather;
    doc["ambientLight"] = dnState.ambientLightLevel;
    doc["autoBrightness"] = dnState.autoBrightness;
    doc["autoTheme"] = dnState.autoTheme;
    doc["transitioning"] = isTransitioning();
    doc["transitionProgress"] = dnState.transitionProgress;

    ThemePalette pal = getCurrentPalette();
    doc["brightness"] = dnState.autoBrightness ? calculateAutoBrightness() : pal.brightness;

    String output;
    serializeJson(doc, output);
    return output;
}

void updateDayNightTheme() {
    // Update transition progress
    if (dnState.transitionProgress < 1.0f) {
        unsigned long elapsed = millis() - dnState.lastTransitionMs;
        dnState.transitionProgress = (float)elapsed / TRANSITION_DURATION_MS;
        if (dnState.transitionProgress >= 1.0f) {
            dnState.transitionProgress = 1.0f;
            dnState.currentTheme = dnState.targetTheme;
        }
    }
}

// ============================================================
// Rain particle overlay rendering
// ============================================================
void renderWeatherOverlay() {
#ifdef ESP32
    if (dnState.currentWeather != WEATHER_RAIN || !rainActive) return;

    uint32_t now = millis();
    if (now - rainLastFrame < 50) return; // ~20fps
    uint32_t dt = now - rainLastFrame;
    rainLastFrame = now;

    // Get LVGL screen for drawing
    lv_obj_t *screen = lv_scr_act();
    if (!screen) return;

    // Draw rain particles as simple lines
    lv_color_t rainColor = lv_color_make(150, 180, 255); // Light blue

    for (uint8_t i = 0; i < RAIN_PARTICLE_COUNT; i++) {
        // Update particle position based on speed and time delta
        rainParticles[i].y += rainParticles[i].speed * dt / 50;
        rainParticles[i].x += (rainParticles[i].speed - 2) * dt / 100; // Slight diagonal drift

        // Wrap particle if it goes off screen
        if (rainParticles[i].y >= TFT_HEIGHT) {
            rainParticles[i].y = 0;
            rainParticles[i].x = rand() % TFT_WIDTH;
        }
        if (rainParticles[i].x >= TFT_WIDTH) {
            rainParticles[i].x = 0;
        } else if (rainParticles[i].x < 0) {
            rainParticles[i].x = TFT_WIDTH - 1;
        }

        // Draw raindrop as a short vertical line
        // LVGL v8 lv_line API — requires LV_USE_LINE in lv_conf.h
        #if LV_USE_LINE
        lv_point_t points[2];
        points[0].x = rainParticles[i].x;
        points[0].y = rainParticles[i].y;
        points[1].x = rainParticles[i].x;
        points[1].y = rainParticles[i].y + 8;

        lv_obj_t *line = lv_line_create(screen);
        lv_line_set_points(line, points, 2);
        lv_obj_set_style_line_color(line, rainColor, LV_PART_MAIN);
        lv_obj_set_style_line_width(line, 1, LV_PART_MAIN);
        #else
        // Fallback: draw a small rectangle for the raindrop
        lv_obj_t *drop = lv_obj_create(screen);
        lv_obj_set_size(drop, 1, 8);
        lv_obj_set_pos(drop, rainParticles[i].x, rainParticles[i].y);
        lv_obj_set_style_bg_color(drop, rainColor, LV_PART_MAIN);
        lv_obj_set_style_border_width(drop, 0, LV_PART_MAIN);
        #endif
    }
#else
    // Native/test stub: no-op
    (void)0;
#endif
}
