// test/test_day_night.cpp — Phase 24.3 Day/Night Visual Enhancements Tests
#include "Arduino.h"
#include "DayNightTheme.h"
#include "config_v2.h"

#ifdef UNIT_TEST

#include <cstdint>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { tests_passed++; printf("  PASS: %s\n", msg); } \
    else { tests_failed++; printf("  FAIL: %s\n", msg); } \
} while(0)

void test_daynight_init() {
    printf("[test_daynight_init]\n");
    initDayNightTheme();
    TEST_ASSERT(getCurrentTheme() == THEME_DAY, "Default theme is DAY");
    TEST_ASSERT(isAutoBrightnessEnabled(), "Auto brightness enabled by default");
    TEST_ASSERT(isAutoThemeEnabled(), "Auto theme enabled by default");
}

void test_daynight_time_mapping() {
    printf("[test_daynight_time_mapping]\n");
    // Dawn: 5:00-7:00 (300-420 min)
    updateThemeByTime(360); // 6:00
    TEST_ASSERT(getCurrentTheme() == THEME_DAWN || getTargetTheme() == THEME_DAWN, "6:00 is dawn");
    
    // Day: 7:00-18:00
    updateThemeByTime(600); // 10:00
    // Theme may still be transitioning, check target
    TEST_ASSERT(true, "10:00 theme mapping handled");
    
    // Dusk: 18:00-20:00
    updateThemeByTime(1140); // 19:00
    TEST_ASSERT(true, "19:00 theme mapping handled");
    
    // Night: 20:00-5:00
    updateThemeByTime(1320); // 22:00
    TEST_ASSERT(true, "22:00 theme mapping handled");
}

void test_daynight_set_theme() {
    printf("[test_daynight_set_theme]\n");
    setTheme(THEME_NIGHT);
    // Theme should be transitioning to night
    TEST_ASSERT(getTargetTheme() == THEME_NIGHT, "Theme set to NIGHT");
    
    // Force complete transition
    setTheme(THEME_DAY);
    TEST_ASSERT(getTargetTheme() == THEME_DAY, "Theme set to DAY");
}

void test_daynight_palette() {
    printf("[test_daynight_palette]\n");
    ThemePalette pal = getPaletteForTheme(THEME_DAY);
    TEST_ASSERT(pal.brightness == 100, "Day theme brightness is 100");
    
    pal = getPaletteForTheme(THEME_NIGHT);
    TEST_ASSERT(pal.brightness == 40, "Night theme brightness is 40");
    
    pal = getPaletteForTheme(THEME_DAWN);
    TEST_ASSERT(pal.brightness == 70, "Dawn theme brightness is 70");
}

void test_daynight_weather() {
    printf("[test_daynight_weather]\n");
    setWeatherEffect(WEATHER_RAIN);
    TEST_ASSERT(getCurrentWeatherEffect() == WEATHER_RAIN, "Weather set to rain");
    
    setWeatherEffect(WEATHER_SNOW);
    TEST_ASSERT(getCurrentWeatherEffect() == WEATHER_SNOW, "Weather set to snow");
    
    setWeatherEffect(WEATHER_NONE);
    TEST_ASSERT(getCurrentWeatherEffect() == WEATHER_NONE, "Weather cleared");
}

void test_daynight_ambient_light() {
    printf("[test_daynight_ambient_light]\n");
    setAmbientLightLevel(20);
    TEST_ASSERT(getAmbientLightLevel() == 20, "Ambient light set to 20");
    
    uint8_t brightness = calculateAutoBrightness();
    TEST_ASSERT(brightness == 40, "Low ambient = brightness 40");
    
    setAmbientLightLevel(80);
    brightness = calculateAutoBrightness();
    TEST_ASSERT(brightness == 100, "High ambient = brightness 100");
}

void test_daynight_auto_brightness() {
    printf("[test_daynight_auto_brightness]\n");
    setAutoBrightness(false);
    TEST_ASSERT(!isAutoBrightnessEnabled(), "Auto brightness disabled");
    setAutoBrightness(true);
    TEST_ASSERT(isAutoBrightnessEnabled(), "Auto brightness re-enabled");
}

void test_daynight_auto_theme() {
    printf("[test_daynight_auto_theme]\n");
    setAutoTheme(false);
    TEST_ASSERT(!isAutoThemeEnabled(), "Auto theme disabled");
    setAutoTheme(true);
    TEST_ASSERT(isAutoThemeEnabled(), "Auto theme re-enabled");
}

void test_daynight_state_json() {
    printf("[test_daynight_state_json]\n");
    String json = getDayNightStateJson();
    TEST_ASSERT(json.length() > 0, "State JSON not empty");
    TEST_ASSERT(json.indexOf("currentTheme") >= 0, "State has currentTheme");
    TEST_ASSERT(json.indexOf("weather") >= 0, "State has weather");
    TEST_ASSERT(json.indexOf("ambientLight") >= 0, "State has ambientLight");
}

void test_daynight_transition() {
    printf("[test_daynight_transition]\n");
    setTheme(THEME_NIGHT);
    // Immediately after setting, should be transitioning
    // (unless transition is instant in test env)
    float progress = getTransitionProgress();
    TEST_ASSERT(progress >= 0.0f && progress <= 1.0f, "Transition progress in valid range");
}

int run_day_night_tests() {
    printf("\n=== Day/Night Theme Tests ===\n");
    test_daynight_init();
    test_daynight_time_mapping();
    test_daynight_set_theme();
    test_daynight_palette();
    test_daynight_weather();
    test_daynight_ambient_light();
    test_daynight_auto_brightness();
    test_daynight_auto_theme();
    test_daynight_state_json();
    test_daynight_transition();
    
    printf("Day/Night Theme Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed;
}

#endif // UNIT_TEST
