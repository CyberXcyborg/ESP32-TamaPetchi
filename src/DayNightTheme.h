#ifndef DAY_NIGHT_THEME_H
#define DAY_NIGHT_THEME_H

#include <Arduino.h>
#include "config_v2.h"

// ============================================================
// Phase 24.3: Day/Night Visual Enhancements
// Dynamic background rendering, weather effects, ambient
// light sensor integration, and smooth theme transitions.
// ============================================================

// Time-of-day themes
enum DayNightTheme {
    THEME_DAWN = 0,
    THEME_DAY,
    THEME_DUSK,
    THEME_NIGHT,
    THEME_COUNT
};

// Weather effect types
enum WeatherEffect {
    WEATHER_NONE = 0,
    WEATHER_RAIN,
    WEATHER_SNOW,
    WEATHER_SUNSHINE,
    WEATHER_CLOUDY,
    WEATHER_COUNT
};

// Theme color palette
struct ThemePalette {
    uint32_t bg_color;        // Background color
    uint32_t text_color;      // Text color
    uint32_t accent_color;    // Accent/highlight color
    uint32_t pet_color;       // Pet tint color
    uint8_t brightness;       // Display brightness (0-100)
};

// Day/night state
struct DayNightState {
    DayNightTheme currentTheme;
    DayNightTheme targetTheme;
    WeatherEffect currentWeather;
    uint8_t ambientLightLevel;  // 0-100 from sensor
    bool autoBrightness;
    bool autoTheme;
    unsigned long lastTransitionMs;
    float transitionProgress;  // 0.0 to 1.0
};

// Initialize day/night theme system
void initDayNightTheme();

// Update theme based on current time (minutes since midnight)
void updateThemeByTime(uint16_t minutesSinceMidnight);

// Force a specific theme
void setTheme(DayNightTheme theme);

// Get current theme
DayNightTheme getCurrentTheme();

// Get target theme (during transition)
DayNightTheme getTargetTheme();

// Get theme palette for current or transitioning state
ThemePalette getCurrentPalette();

// Get palette for a specific theme
ThemePalette getPaletteForTheme(DayNightTheme theme);

// Weather effects
void setWeatherEffect(WeatherEffect effect);
WeatherEffect getCurrentWeatherEffect();

// Ambient light sensor
void setAmbientLightLevel(uint8_t level);
uint8_t getAmbientLightLevel();

// Auto-brightness based on ambient light
uint8_t calculateAutoBrightness();

// Enable/disable auto-brightness
void setAutoBrightness(bool enabled);
bool isAutoBrightnessEnabled();

// Enable/disable auto-theme (time-based)
void setAutoTheme(bool enabled);
bool isAutoThemeEnabled();

// Transition control
bool isTransitioning();
float getTransitionProgress();

// Get state as JSON
String getDayNightStateJson();

// Task update (call in main loop)
void updateDayNightTheme();

// Render weather overlay (call in display render loop)
void renderWeatherOverlay();

#endif // DAY_NIGHT_THEME_H
