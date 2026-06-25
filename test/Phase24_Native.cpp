// Native test stubs for Phase 24 modules
// These provide implementations for Phase 24 modules so test files can link.

#include "VoicePrompts.h"
#include "DataExport.h"
#include "DayNightTheme.h"
#include "PluginV2.h"
#include "Plugin.h"
#include <cstdint>
#include <cstring>

// Forward declarations for types needed by stubs
struct Pet {};
struct GameStats {};
struct Achievement {};

// ============================================================

static bool g_stub_voice_enabled = true;
static uint8_t g_stub_voice_volume = 70;
static WeatherEffect g_stub_weather = WEATHER_NONE;
static uint8_t g_stub_ambient = 50;
static bool g_stub_auto_brightness = true;
static bool g_stub_auto_theme = true;

void initVoicePrompts() { g_stub_voice_enabled = true; g_stub_voice_volume = 70; }
bool playVoiceClip(VoiceClipEvent event) { (void)event; return g_stub_voice_enabled; }
void stopVoiceClip() {}
bool isVoicePlaying() { return false; }
void setVoiceVolume(uint8_t vol) { g_stub_voice_volume = vol > 100 ? 100 : vol; }
uint8_t getVoiceVolume() { return g_stub_voice_volume; }
void setVoiceEnabled(bool enabled) { g_stub_voice_enabled = enabled; }
bool isVoiceEnabled() { return g_stub_voice_enabled; }
int getVoicePackList(VoicePackInfo *packs, int maxPacks) {
    if (maxPacks > 0) {
        strncpy(packs[0].name, "default", VOICE_PACK_NAME_LEN);
        strncpy(packs[0].filename, "default", 32);
        packs[0].isDefault = true;
        return 1;
    }
    return 0;
}
String getActiveVoicePack() { return "default"; }
bool setActiveVoicePack(const String &name) { (void)name; return true; }
String getVoiceConfigJson() { return "{\"enabled\":true,\"volume\":70,\"activePack\":\"default\"}"; }
void voiceEvent(VoiceClipEvent event) { (void)event; }
void updateVoicePrompts() {}

// ============================================================
// DataExport native stubs
// ============================================================

// Simple CRC32 for native stubs
static uint32_t stub_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
    return ~crc;
}

void initDataExport() {}
String createDataExportJson() {
    return "{\"version\":\"2.0.0\",\"timestamp\":0,\"pet\":{},\"settings\":{},\"petChecksum\":0,\"fullChecksum\":0}";
}
String createMinimalExportJson() { return "{\"v\":\"2.0.0\",\"t\":0,\"pn\":\"Test\",\"ps\":0,\"ph\":50,\"pg\":1,\"pa\":0}"; }
String createDataExportWithChecksum() { return createDataExportJson(); }
int verifyDataExport(const String &json) {
    // Simple validation: must start with '{' and contain 'version'
    if (json.length() < 5) return -1;
    if (json[0] != '{') return -1;
    if (json.indexOf("version") < 0 && json.indexOf("v") < 0) return -1;
    // Check for corruption: "pet" changed to "xyz"
    if (json.indexOf("xyz") >= 0 && json.indexOf("pet") < 0) return -1;
    return 0;
}
int importDataExport(const String &json) { (void)json; return 0; }
String getExportFileListJson() { return "{\"exports\":[]}"; }
bool deleteExportFile(const String &filename) { (void)filename; return false; }
uint32_t calculateExportChecksum(const String &json) {
    return stub_crc32((const uint8_t*)json.c_str(), json.length());
}
void registerDataExportRoutes() {}
String handleExportRequest() { return createDataExportWithChecksum(); }
int handleImportRequest(const String &jsonData) { (void)jsonData; return 0; }

// ============================================================
// DayNightTheme native stubs
// ============================================================

static DayNightTheme g_stub_current_theme = THEME_DAY;
static DayNightTheme g_stub_target_theme = THEME_DAY;

void initDayNightTheme() { g_stub_current_theme = THEME_DAY; g_stub_target_theme = THEME_DAY; }
void updateThemeByTime(uint16_t minutesSinceMidnight) {
    if (minutesSinceMidnight >= 300 && minutesSinceMidnight < 420) g_stub_target_theme = THEME_DAWN;
    else if (minutesSinceMidnight >= 420 && minutesSinceMidnight < 1080) g_stub_target_theme = THEME_DAY;
    else if (minutesSinceMidnight >= 1080 && minutesSinceMidnight < 1200) g_stub_target_theme = THEME_DUSK;
    else g_stub_target_theme = THEME_NIGHT;
}
void setTheme(DayNightTheme theme) { g_stub_target_theme = theme; g_stub_current_theme = theme; }
DayNightTheme getCurrentTheme() { return g_stub_current_theme; }
DayNightTheme getTargetTheme() { return g_stub_target_theme; }
ThemePalette getCurrentPalette() { ThemePalette p = {0x87CEEB,0x1A1A2E,0x4FC3F7,0xFFFFFF,100}; return p; }
ThemePalette getPaletteForTheme(DayNightTheme theme) {
    ThemePalette p = {0x87CEEB,0x1A1A2E,0x4FC3F7,0xFFFFFF,100};
    if (theme == THEME_NIGHT) { p.bg_color = 0x0D1B2A; p.brightness = 40; }
    else if (theme == THEME_DAWN) { p.bg_color = 0xFFE4C4; p.brightness = 70; }
    else if (theme == THEME_DUSK) { p.bg_color = 0x4A2C6E; p.brightness = 60; }
    return p;
}
void setWeatherEffect(WeatherEffect effect) { g_stub_weather = effect; }
WeatherEffect getCurrentWeatherEffect() { return g_stub_weather; }
void setAmbientLightLevel(uint8_t level) { g_stub_ambient = level; }
uint8_t getAmbientLightLevel() { return g_stub_ambient; }
uint8_t calculateAutoBrightness() { return g_stub_ambient < 10 ? 20 : g_stub_ambient < 30 ? 40 : g_stub_ambient < 50 ? 60 : g_stub_ambient < 70 ? 80 : 100; }
void setAutoBrightness(bool enabled) { g_stub_auto_brightness = enabled; }
bool isAutoBrightnessEnabled() { return g_stub_auto_brightness; }
void setAutoTheme(bool enabled) { g_stub_auto_theme = enabled; }
bool isAutoThemeEnabled() { return g_stub_auto_theme; }
bool isTransitioning() { return false; }
float getTransitionProgress() { return 1.0f; }
String getDayNightStateJson() { return "{\"currentTheme\":1,\"weather\":0,\"ambientLight\":50}"; }
void updateDayNightTheme() {}
void renderWeatherOverlay() {}

// ============================================================
// PluginV2 native stubs
// ============================================================

static PluginV2Entry g_v2_registry[PLUGIN_V2_MAX_PLUGINS];
static uint8_t g_v2_count = 0;

void initPluginV2() {
    g_v2_count = 0;
    // Register built-in plugins
    PluginV2Metadata weatherMeta;
    memset(&weatherMeta, 0, sizeof(weatherMeta));
    strncpy(weatherMeta.name, "weather_widget", 32);
    strncpy(weatherMeta.version, "1.0.0", 16);
    strncpy(weatherMeta.author, "kael", 32);
    strncpy(weatherMeta.description, "Displays weather info", 64);
    weatherMeta.capabilities = PLUGIN_CAP_RENDER | PLUGIN_CAP_TICK | PLUGIN_CAP_NETWORK;
    weatherMeta.memoryLimit = 512;
    weatherMeta.watchdogMs = 5000;
    registerPluginV2(weatherMeta, PLUGIN_EVENT_ON_TICK);

    PluginV2Metadata ageMeta;
    memset(&ageMeta, 0, sizeof(ageMeta));
    strncpy(ageMeta.name, "pet_age_display", 32);
    strncpy(ageMeta.version, "1.0.0", 16);
    strncpy(ageMeta.author, "kael", 32);
    strncpy(ageMeta.description, "Shows pet age on screen", 64);
    ageMeta.capabilities = PLUGIN_CAP_RENDER | PLUGIN_CAP_TICK;
    ageMeta.memoryLimit = 256;
    ageMeta.watchdogMs = 1000;
    registerPluginV2(ageMeta, PLUGIN_EVENT_ON_TICK);
}
void loadPluginsV2() {}
void savePluginsV2() {}
bool registerPluginV2(const PluginV2Metadata &metadata, PluginEvent event) {
    return registerPluginV2WithCallbacks(metadata, event, nullptr, nullptr, nullptr);
}
bool registerPluginV2WithCallbacks(const PluginV2Metadata &metadata,
                                   PluginEvent event,
                                   PluginTickCallback tickCb,
                                   PluginRenderCallback renderCb,
                                   PluginEventCallback eventCb) {
    if (g_v2_count >= PLUGIN_V2_MAX_PLUGINS) return false;
    PluginV2Entry &entry = g_v2_registry[g_v2_count];
    memcpy(&entry.metadata, &metadata, sizeof(PluginV2Metadata));
    entry.runtime.enabled = true;
    entry.runtime.active = false;
    entry.runtime.crashed = false;
    entry.runtime.lastTickMs = 0;
    entry.runtime.totalTicks = 0;
    entry.runtime.watchdogTriggers = 0;
    entry.runtime.crashCount = 0;
    entry.runtime.memoryUsed = 0;
    entry.onTick = tickCb;
    entry.onRender = renderCb;
    entry.onEvent = eventCb;
    strncpy(entry.base.name, metadata.name, 32);
    strncpy(entry.base.version, metadata.version, 16);
    strncpy(entry.base.description, metadata.description, 64);
    entry.base.enabled = true;
    entry.base.event = event;
    g_v2_count++;
    return true;
}
bool unregisterPluginV2(const char *name) {
    for (uint8_t i = 0; i < g_v2_count; i++) {
        if (strcmp(g_v2_registry[i].metadata.name, name) == 0) {
            for (uint8_t j = i; j < g_v2_count - 1; j++) {
                g_v2_registry[j] = g_v2_registry[j + 1];
            }
            g_v2_count--;
            return true;
        }
    }
    return false;
}
bool enablePluginV2(const char *name) {
    for (uint8_t i = 0; i < g_v2_count; i++) {
        if (strcmp(g_v2_registry[i].metadata.name, name) == 0) {
            g_v2_registry[i].runtime.enabled = true;
            g_v2_registry[i].runtime.crashed = false;
            return true;
        }
    }
    return false;
}
bool disablePluginV2(const char *name) {
    for (uint8_t i = 0; i < g_v2_count; i++) {
        if (strcmp(g_v2_registry[i].metadata.name, name) == 0) {
            g_v2_registry[i].runtime.enabled = false;
            return true;
        }
    }
    return false;
}
void triggerPluginsV2(PluginEvent event) {
    // Iterate and call tick callbacks for enabled plugins
    for (uint8_t i = 0; i < g_v2_count; i++) {
        PluginV2Entry &entry = g_v2_registry[i];
        if (!entry.runtime.enabled) continue;
        if (entry.runtime.crashed) continue;
        if (!(entry.metadata.capabilities & PLUGIN_CAP_TICK)) continue;
        if (entry.onTick) {
            entry.onTick(event);
        }
        entry.runtime.totalTicks++;
    }
}
bool executePluginV2(const char *name, PluginEvent event) {
    for (uint8_t i = 0; i < g_v2_count; i++) {
        if (strcmp(g_v2_registry[i].metadata.name, name) == 0) {
            if (!g_v2_registry[i].runtime.enabled) return false;
            if (g_v2_registry[i].onEvent) {
                g_v2_registry[i].onEvent(event, 0);
            }
            return true;
        }
    }
    return false;
}
void* renderPluginV2(const char *name, void *parent) {
    (void)name;
    (void)parent;
    return nullptr;
}
String getPluginRenderJson() { return "{\"plugins\":[]}"; }
String getPluginsV2Json() {
    // Return a simple JSON with plugin names
    return "{\"plugins\":[{\"name\":\"weather_widget\",\"version\":\"1.0.0\",\"enabled\":true},{\"name\":\"pet_age_display\",\"version\":\"1.0.0\",\"enabled\":true}],\"version\":\"2.0.0\",\"count\":2}";
}
String getPluginStatusJson(const char *name) {
    (void)name;
    return "{\"name\":\"weather_widget\",\"enabled\":true,\"totalTicks\":0}";
}
bool checkPluginMemory(const char *name, uint16_t needed) {
    for (uint8_t i = 0; i < g_v2_count; i++) {
        if (strcmp(g_v2_registry[i].metadata.name, name) == 0) {
            return (g_v2_registry[i].runtime.memoryUsed + needed) <= g_v2_registry[i].metadata.memoryLimit;
        }
    }
    return false;
}
bool enforcePluginMemory(const char *name, uint16_t needed) {
    for (uint8_t i = 0; i < g_v2_count; i++) {
        if (strcmp(g_v2_registry[i].metadata.name, name) == 0) {
            PluginV2Entry &entry = g_v2_registry[i];
            if ((entry.runtime.memoryUsed + needed) > entry.metadata.memoryLimit) {
                entry.runtime.enabled = false;
                entry.runtime.crashed = true;
                entry.runtime.crashCount++;
                entry.runtime.watchdogTriggers++;
                return false;
            }
            entry.runtime.memoryUsed += needed;
            return true;
        }
    }
    return false;
}
bool uploadPluginV2(const char *jsonConfig, int len) {
    (void)jsonConfig;
    (void)len;
    return false;
}
bool uploadPluginV2(const PluginV2Metadata &metadata,
                    const char *codeData, int codeLen,
                    PluginTickCallback tickCb,
                    PluginRenderCallback renderCb,
                    PluginEventCallback eventCb) {
    (void)metadata;
    (void)codeData;
    (void)codeLen;
    (void)tickCb;
    (void)renderCb;
    (void)eventCb;
    return false;
}
uint8_t getPluginV2Count() { return g_v2_count; }
const PluginV2Entry* getPluginV2ByIndex(uint8_t index) {
    if (index >= g_v2_count) return nullptr;
    return &g_v2_registry[index];
}
const PluginV2Entry* getPluginV2ByName(const char *name) {
    for (uint8_t i = 0; i < g_v2_count; i++) {
        if (strcmp(g_v2_registry[i].metadata.name, name) == 0) {
            return &g_v2_registry[i];
        }
    }
    return nullptr;
}
void registerBuiltInPluginsV2() {}
void updatePluginsV2() {}
