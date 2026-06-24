#include "DayNightTheme.h"
#include "AppState.h"
#include <ArduinoJson.h>

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
    
    Serial.println("[DayNightTheme] Initialized");
}

void updateThemeByTime(uint16_t minutesSinceMidnight) {
    DayNightTheme newTheme = timeToTheme(minutesSinceMidnight);
    if (newTheme != dnState.targetTheme) {
        dnState.targetTheme = newTheme;
        dnState.lastTransitionMs = millis();
        dnState.transitionProgress = 0.0f;
        Serial.printf("[DayNightTheme] Transitioning to theme %d\n", newTheme);
    }
}

void setTheme(DayNightTheme theme) {
    if (theme < THEME_COUNT && theme != dnState.targetTheme) {
        dnState.targetTheme = theme;
        dnState.lastTransitionMs = millis();
        dnState.transitionProgress = 0.0f;
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

void renderWeatherOverlay() {
    // Weather overlay rendering placeholder
    // In full implementation, this would render rain/snow particles
    // using LVGL animations on top of the current screen
    // For now, this is a no-op stub
    (void)0;
}
