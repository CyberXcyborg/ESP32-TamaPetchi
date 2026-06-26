#include "Plugin.h"
#include "Pet.h"
#include "AppState.h"
#include "Storage.h"
#include "config.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>

// ============================================================
// Phase 17.4: Plugin System Implementation
// ============================================================

#define MAX_PLUGINS 8
#define PLUGIN_FILE "/plugins.json"

static PluginInfo plugins[MAX_PLUGINS];
static int pluginCount = 0;

// --- Plugin Manager ---

void initPluginManager() {
    pluginCount = 0;
    memset(plugins, 0, sizeof(plugins));
    registerBuiltInPlugins();
    loadPluginsFromStorage();
}

void registerBuiltInPlugins() {
    if (pluginCount >= MAX_PLUGINS) return;

    // Holiday Events plugin
    PluginInfo& hol = plugins[pluginCount++];
    strncpy(hol.name, "HolidayEvents", sizeof(hol.name) - 1);
    strncpy(hol.version, "1.0", sizeof(hol.version) - 1);
    strncpy(hol.description, "Special events for holidays", sizeof(hol.description) - 1);
    hol.enabled = false;
    hol.event = PLUGIN_EVENT_ON_TICK;
    strncpy(hol.action, "holiday_check", sizeof(hol.action) - 1);

    // Weather Mood plugin
    PluginInfo& wth = plugins[pluginCount++];
    strncpy(wth.name, "WeatherMood", sizeof(wth.version) - 1);
    strncpy(wth.version, "1.0", sizeof(wth.version) - 1);
    strncpy(wth.description, "Weather affects pet mood", sizeof(wth.description) - 1);
    wth.enabled = false;
    wth.event = PLUGIN_EVENT_ON_TICK;
    strncpy(wth.action, "weather_mood", sizeof(wth.action) - 1);
}

void loadPluginsFromStorage() {
    if (!SPIFFS.exists(PLUGIN_FILE)) return;

    File f = SPIFFS.open(PLUGIN_FILE, "r");
    if (!f) return;

    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) {
        f.close();
        return;
    }
    f.close();

    JsonArray arr = doc["plugins"].as<JsonArray>();
    for (JsonObject obj : arr) {
        if (pluginCount >= MAX_PLUGINS) break;

        const char* name = obj["name"] | "";
        // Find existing plugin and update enabled state
        for (int i = 0; i < pluginCount; i++) {
            if (strcmp(plugins[i].name, name) == 0) {
                plugins[i].enabled = obj["enabled"] | false;
                break;
            }
        }
    }
}

void savePluginsToStorage() {
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.createNestedArray("plugins");

    for (int i = 0; i < pluginCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["name"] = plugins[i].name;
        obj["version"] = plugins[i].version;
        obj["enabled"] = plugins[i].enabled;
    }

    File f = SPIFFS.open(PLUGIN_FILE, "w");
    if (!f) return;
    serializeJson(doc, f);
    f.close();
}

// --- Event Triggering ---

void triggerPluginEvent(PluginEvent event) {
    for (int i = 0; i < pluginCount; i++) {
        if (!plugins[i].enabled) continue;
        if (plugins[i].event != event && plugins[i].event != PLUGIN_EVENT_NONE) continue;

        // Execute plugin action
        if (strcmp(plugins[i].name, "HolidayEvents") == 0) {
            pluginHolidayEvents(event);
        } else if (strcmp(plugins[i].name, "WeatherMood") == 0) {
            pluginWeatherMood(event);
        }
    }
}

// --- Built-in Plugin Implementations ---

void pluginHolidayEvents(PluginEvent event) {
    if (event != PLUGIN_EVENT_ON_TICK) return;

    // Simple holiday check based on compile-time date
    // In a real implementation, this would use NTP time
    // For now, just a stat boost on "special days"
    AppState& state = APP_STATE;
    if (!state.pet.isAlive) return;

    // Give a small happiness boost when plugin is active
    // (simulating holiday cheer)
    if (state.pet.happiness < STAT_MAX) {
        state.pet.happiness = min(STAT_MAX, state.pet.happiness + 1);
    }
}

void pluginWeatherMood(PluginEvent event) {
    if (event != PLUGIN_EVENT_ON_TICK) return;

    AppState& state = APP_STATE;
    if (!state.pet.isAlive) return;

    // Simulate weather effect on mood
    // In a real implementation, this would fetch weather data
    // For now, use a pseudo-random variation
    int weatherEffect = (millis() / 600000) % 3 - 1;  // -1, 0, or 1
    state.pet.happiness = constrain(state.pet.happiness + weatherEffect, STAT_MIN, STAT_MAX);
}

// --- JSON Output ---

String getPluginsJson() {
    String json = "{\"plugins\":[";
    for (int i = 0; i < pluginCount; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"name\":\"" + String(plugins[i].name) + "\",";
        json += "\"version\":\"" + String(plugins[i].version) + "\",";
        json += "\"description\":\"" + String(plugins[i].description) + "\",";
        json += "\"enabled\":" + String(plugins[i].enabled ? "true" : "false") + ",";
        json += "\"event\":" + String(plugins[i].event);
        json += "}";
    }
    json += "]}";
    return json;
}

// --- Plugin Management ---

bool enablePlugin(const char* name) {
    for (int i = 0; i < pluginCount; i++) {
        if (strcmp(plugins[i].name, name) == 0) {
            plugins[i].enabled = true;
            savePluginsToStorage();
            return true;
        }
    }
    return false;
}

bool disablePlugin(const char* name) {
    for (int i = 0; i < pluginCount; i++) {
        if (strcmp(plugins[i].name, name) == 0) {
            plugins[i].enabled = false;
            savePluginsToStorage();
            return true;
        }
    }
    return false;
}

bool uploadPlugin(const char* json, int len) {
    if (pluginCount >= MAX_PLUGINS) return false;

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, json, len) != DeserializationError::Ok) return false;

    const char* name = doc["name"] | "";
    const char* version = doc["version"] | "1.0";
    const char* desc = doc["description"] | "";

    if (strlen(name) == 0) return false;

    // Check for duplicate
    for (int i = 0; i < pluginCount; i++) {
        if (strcmp(plugins[i].name, name) == 0) return false;
    }

    PluginInfo& p = plugins[pluginCount++];
    strncpy(p.name, name, sizeof(p.name) - 1);
    strncpy(p.version, version, sizeof(p.version) - 1);
    strncpy(p.description, desc, sizeof(p.description) - 1);
    p.enabled = false;
    p.event = PLUGIN_EVENT_ON_TICK;
    p.action[0] = '\0';

    savePluginsToStorage();
    return true;
}

bool deletePlugin(const char* name) {
    for (int i = 0; i < pluginCount; i++) {
        if (strcmp(plugins[i].name, name) == 0) {
            // Shift remaining plugins
            for (int j = i; j < pluginCount - 1; j++) {
                plugins[j] = plugins[j + 1];
            }
            pluginCount--;
            savePluginsToStorage();
            return true;
        }
    }
    return false;
}
