#include "PluginV2.h"
#include "AppState.h"
#include "Storage_v2.h"
#include <ArduinoJson.h>

// ============================================================
// Module-level state
// ============================================================
static PluginV2Entry pluginRegistry[PLUGIN_V2_MAX_PLUGINS];
static uint8_t pluginCount = 0;
static bool pluginV2Initialized = false;

// ============================================================
// Built-in plugins
// ============================================================

// Weather Widget — displays weather info on screen
static void pluginWeatherWidget(PluginEvent event) {
    // Stub: would fetch weather data and render on LVGL
    (void)event;
}

// Pet Age Display — shows pet age in days/hours
static void pluginPetAgeDisplay(PluginEvent event) {
    // Stub: would render age on LVGL screen
    (void)event;
}

// ============================================================
// Public API
// ============================================================

void initPluginV2() {
    pluginV2Initialized = true;
    pluginCount = 0;
    
    // Register built-in plugins
    registerBuiltInPluginsV2();
    
    // Load saved plugin state
    loadPluginsV2();
    
    Serial.printf("[PluginV2] Initialized — %d plugins loaded\n", pluginCount);
}

void loadPluginsV2() {
    if (!StorageV2::exists("/plugins_v2.json")) return;
    
    File f = StorageV2::open("/plugins_v2.json", "r");
    if (!f) return;
    
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return;
    
    // Load enabled/disabled state for each plugin
    JsonArray arr = doc["plugins"];
    if (arr) {
        for (JsonObject obj : arr) {
            const char *name = obj["name"];
            bool enabled = obj["enabled"] | true;
            
            // Find plugin in registry
            for (uint8_t i = 0; i < pluginCount; i++) {
                if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
                    pluginRegistry[i].runtime.enabled = enabled;
                    break;
                }
            }
        }
    }
}

void savePluginsV2() {
    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.createNestedArray("plugins");
    
    for (uint8_t i = 0; i < pluginCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["name"] = pluginRegistry[i].metadata.name;
        obj["enabled"] = pluginRegistry[i].runtime.enabled;
    }
    
    File f = StorageV2::open("/plugins_v2.json", "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
}

bool registerPluginV2(const PluginV2Metadata &metadata, PluginEvent event) {
    if (pluginCount >= PLUGIN_V2_MAX_PLUGINS) return false;
    
    PluginV2Entry &entry = pluginRegistry[pluginCount];
    memcpy(&entry.metadata, &metadata, sizeof(PluginV2Metadata));
    
    // Initialize runtime
    entry.runtime.enabled = true;
    entry.runtime.active = false;
    entry.runtime.lastTickMs = 0;
    entry.runtime.totalTicks = 0;
    entry.runtime.watchdogTriggers = 0;
    entry.runtime.memoryUsed = 0;
    
    // Set base plugin info
    strncpy(entry.base.name, metadata.name, 32);
    strncpy(entry.base.version, metadata.version, 16);
    strncpy(entry.base.description, metadata.description, 64);
    entry.base.enabled = true;
    entry.base.event = event;
    
    pluginCount++;
    return true;
}

bool unregisterPluginV2(const char *name) {
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
            // Shift remaining plugins
            for (uint8_t j = i; j < pluginCount - 1; j++) {
                pluginRegistry[j] = pluginRegistry[j + 1];
            }
            pluginCount--;
            return true;
        }
    }
    return false;
}

bool enablePluginV2(const char *name) {
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
            pluginRegistry[i].runtime.enabled = true;
            savePluginsV2();
            return true;
        }
    }
    return false;
}

bool disablePluginV2(const char *name) {
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
            pluginRegistry[i].runtime.enabled = false;
            savePluginsV2();
            return true;
        }
    }
    return false;
}

void triggerPluginsV2(PluginEvent event) {
    if (!pluginV2Initialized) return;
    
    unsigned long now = millis();
    
    for (uint8_t i = 0; i < pluginCount; i++) {
        PluginV2Entry &entry = pluginRegistry[i];
        if (!entry.runtime.enabled) continue;
        if (entry.base.event != event) continue;
        
        // Watchdog check
        if (now - entry.runtime.lastTickMs < entry.metadata.watchdogMs) {
            entry.runtime.watchdogTriggers++;
            continue; // Skip this tick
        }
        
        // Execute plugin
        entry.runtime.active = true;
        entry.runtime.lastTickMs = now;
        entry.runtime.totalTicks++;
        
        // Call the plugin action
        // In full implementation, this would call the registered callback
        if (strcmp(entry.metadata.name, "weather_widget") == 0) {
            pluginWeatherWidget(event);
        } else if (strcmp(entry.metadata.name, "pet_age_display") == 0) {
            pluginPetAgeDisplay(event);
        }
        
        entry.runtime.active = false;
    }
}

String getPluginRenderJson() {
    DynamicJsonDocument doc(512);
    JsonArray arr = doc.createNestedArray("plugins");
    
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (!pluginRegistry[i].runtime.enabled) continue;
        if (!(pluginRegistry[i].metadata.capabilities & PLUGIN_CAP_RENDER)) continue;
        
        JsonObject obj = arr.createNestedObject();
        obj["name"] = pluginRegistry[i].metadata.name;
        obj["version"] = pluginRegistry[i].metadata.version;
        // Render data would be populated by actual render callbacks
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String getPluginsV2Json() {
    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.createNestedArray("plugins");
    
    for (uint8_t i = 0; i < pluginCount; i++) {
        PluginV2Entry &entry = pluginRegistry[i];
        JsonObject obj = arr.createNestedObject();
        obj["name"] = entry.metadata.name;
        obj["version"] = entry.metadata.version;
        obj["author"] = entry.metadata.author;
        obj["description"] = entry.metadata.description;
        obj["enabled"] = entry.runtime.enabled;
        obj["capabilities"] = entry.metadata.capabilities;
        obj["memoryUsed"] = entry.runtime.memoryUsed;
        obj["totalTicks"] = entry.runtime.totalTicks;
    }
    
    doc["version"] = PLUGIN_V2_VERSION;
    doc["count"] = pluginCount;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String getPluginStatusJson(const char *name) {
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
            DynamicJsonDocument doc(256);
            PluginV2Entry &entry = pluginRegistry[i];
            doc["name"] = entry.metadata.name;
            doc["enabled"] = entry.runtime.enabled;
            doc["active"] = entry.runtime.active;
            doc["totalTicks"] = entry.runtime.totalTicks;
            doc["watchdogTriggers"] = entry.runtime.watchdogTriggers;
            doc["memoryUsed"] = entry.runtime.memoryUsed;
            String output;
            serializeJson(doc, output);
            return output;
        }
    }
    return "{}";
}

bool checkPluginMemory(const char *name, uint16_t needed) {
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
            return (pluginRegistry[i].runtime.memoryUsed + needed) <= pluginRegistry[i].metadata.memoryLimit;
        }
    }
    return false;
}

void registerBuiltInPluginsV2() {
    // Weather Widget
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
    
    // Pet Age Display
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

void updatePluginsV2() {
    if (!pluginV2Initialized) return;
    
    // Trigger tick-capable plugins
    triggerPluginsV2(PLUGIN_EVENT_ON_TICK);
}
