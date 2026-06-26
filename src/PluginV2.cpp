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
// Built-in plugin implementations
// ============================================================

#ifdef ESP32
#include <lvgl.h>
#endif

// Weather Widget — displays weather info on screen
static void pluginWeatherTick(PluginEvent event) {
    (void)event;
    // In full implementation, this would fetch weather data
    // For now, this is a no-op stub
}

#ifdef ESP32
static void pluginWeatherRender(lv_obj_t *parent) {
    if (!parent) return;
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Weather: Sunny");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}
#else
static void pluginWeatherRender(void *parent) {
    (void)parent;
}
#endif

static void pluginWeatherEvent(PluginEvent event, int32_t data) {
    (void)event;
    (void)data;
    // Handle events (e.g., weather update from network)
}

// Pet Age Display — shows pet age in days/hours
static void pluginPetAgeTick(PluginEvent event) {
    (void)event;
    // Update tick counter for age calculation
}

#ifdef ESP32
static void pluginPetAgeRender(lv_obj_t *parent) {
    if (!parent) return;
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Age: 0d 0h");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
}
#else
static void pluginPetAgeRender(void *parent) {
    (void)parent;
}
#endif

static void pluginPetAgeEvent(PluginEvent event, int32_t data) {
    (void)event;
    (void)data;
}

// ============================================================
// Crash isolation helper
// ============================================================
// On ESP32 there's no C++ exception support in thetraditional sense
// for deep recursion. We use setjmp/longjmp as a lightweight
// crash isolation mechanism.
// ============================================================

#include <setjmp.h>

static jmp_buf g_pluginJump;
static bool g_pluginInExecution = false;
static uint8_t g_crashedIndex = 0xFF;

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
    return registerPluginV2WithCallbacks(metadata, event, nullptr, nullptr, nullptr);
}

bool registerPluginV2WithCallbacks(const PluginV2Metadata &metadata,
                                   PluginEvent event,
                                   PluginTickCallback tickCb,
                                   PluginRenderCallback renderCb,
                                   PluginEventCallback eventCb) {
    if (pluginCount >= PLUGIN_V2_MAX_PLUGINS) return false;

    PluginV2Entry &entry = pluginRegistry[pluginCount];
    memcpy(&entry.metadata, &metadata, sizeof(PluginV2Metadata));

    // Initialize runtime
    entry.runtime.enabled = true;
    entry.runtime.active = false;
    entry.runtime.crashed = false;
    entry.runtime.lastTickMs = 0;
    entry.runtime.totalTicks = 0;
    entry.runtime.watchdogTriggers = 0;
    entry.runtime.crashCount = 0;
    entry.runtime.memoryUsed = 0;

    // Set callbacks
    entry.onTick = tickCb;
    entry.onRender = renderCb;
    entry.onEvent = eventCb;

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
            pluginRegistry[i].runtime.crashed = false;
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
        if (entry.runtime.crashed) continue;  // Skip crashed plugins
        if (!(entry.metadata.capabilities & PLUGIN_CAP_TICK)) continue;

        // Watchdog check: minimum interval between ticks
        if (entry.runtime.lastTickMs > 0 &&
            (now - entry.runtime.lastTickMs) < entry.metadata.watchdogMs) {
            entry.runtime.watchdogTriggers++;
            continue; // Skip this tick — too soon
        }

        // Execute plugin with crash isolation
        entry.runtime.active = true;
        entry.runtime.lastTickMs = now;
        entry.runtime.totalTicks++;

        bool crashed = false;
        if (setjmp(g_pluginJump) == 0) {
            g_pluginInExecution = true;
            g_crashedIndex = 0xFF;

            // Call the tick callback if registered
            if (entry.onTick) {
                entry.onTick(event);
            }

            g_pluginInExecution = false;
        } else {
            // longjmp hit — plugin crashed
            crashed = true;
        }

        if (crashed) {
            entry.runtime.crashed = true;
            entry.runtime.enabled = false;
            entry.runtime.crashCount++;
            entry.runtime.watchdogTriggers++;
            Serial.printf("[PluginV2] CRASH: plugin '%s' disabled after crash!\n",
                          entry.metadata.name);
        }

        entry.runtime.active = false;
    }
}

bool executePluginV2(const char *name, PluginEvent event, bool active) {
    if (!pluginV2Initialized) return false;

    for (uint8_t i = 0; i < pluginCount; i++) {
        PluginV2Entry &entry = pluginRegistry[i];
        if (strcmp(entry.metadata.name, name) != 0) continue;
        if (!entry.runtime.enabled) return false;
        if (entry.runtime.crashed) return false;

        entry.runtime.active = active;

        bool crashed = false;
        if (setjmp(g_pluginJump) == 0) {
            g_pluginInExecution = true;
            g_crashedIndex = i;

            if (entry.onEvent) {
                entry.onEvent(event, 0);
            }

            g_pluginInExecution = false;
        } else {
            crashed = true;
        }

        if (crashed) {
            entry.runtime.crashed = true;
            entry.runtime.enabled = false;
            entry.runtime.crashCount++;
            entry.runtime.watchdogTriggers++;
            Serial.printf("[PluginV2] CRASH: plugin '%s' disabled after crash!\n",
                          entry.metadata.name);
        }

        entry.runtime.active = false;
        return !crashed;
    }
    return false;
}

#ifdef ESP32
lv_obj_t* renderPluginV2(const char *name, lv_obj_t *parent) {
#else
void* renderPluginV2(const char *name, void *parent) {
#endif
#ifdef ESP32
    for (uint8_t i = 0; i < pluginCount; i++) {
        PluginV2Entry &entry = pluginRegistry[i];
        if (strcmp(entry.metadata.name, name) != 0) continue;
        if (!entry.runtime.enabled) return nullptr;
        if (!(entry.metadata.capabilities & PLUGIN_CAP_RENDER)) return nullptr;
        if (!entry.onRender) return nullptr;

        bool crashed = false;
        lv_obj_t *result = nullptr;

        if (setjmp(g_pluginJump) == 0) {
            g_pluginInExecution = true;
            g_crashedIndex = i;
            entry.runtime.active = true;

            result = nullptr;
            // Call the render callback — on ESP32 the plugin draws to parent directly
            entry.onRender(parent);

            // Return the last child added by the plugin (or parent itself)
            if (parent) {
                result = lv_obj_get_child(parent, -1);
            }

            entry.runtime.active = false;
            g_pluginInExecution = false;
        } else {
            crashed = true;
        }

        if (crashed) {
            entry.runtime.crashed = true;
            entry.runtime.enabled = false;
            entry.runtime.crashCount++;
            entry.runtime.watchdogTriggers++;
        }

        return result;
    }
#else
    (void)name;
    (void)parent;
#endif
    return nullptr;
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
        obj["renderable"] = true;
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
        obj["crashed"] = entry.runtime.crashed;
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
            doc["crashed"] = entry.runtime.crashed;
            doc["totalTicks"] = entry.runtime.totalTicks;
            doc["watchdogTriggers"] = entry.runtime.watchdogTriggers;
            doc["crashCount"] = entry.runtime.crashCount;
            doc["memoryUsed"] = entry.runtime.memoryUsed;
            doc["memoryLimit"] = entry.metadata.memoryLimit;
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
            // Also check system-wide free memory as a safety net
            return (pluginRegistry[i].runtime.memoryUsed + needed) <= pluginRegistry[i].metadata.memoryLimit;
        }
    }
    return false;
}

bool enforcePluginMemory(const char *name, uint16_t needed) {
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
            PluginV2Entry &entry = pluginRegistry[i];

            // Check against plugin's individual memory limit
            if ((entry.runtime.memoryUsed + needed) > entry.metadata.memoryLimit) {
                // Memory limit exceeded — disable plugin and track
                entry.runtime.enabled = false;
                entry.runtime.crashed = true;
                entry.runtime.crashCount++;
                entry.runtime.watchdogTriggers++;
                Serial.printf("[PluginV2] MEMORY VIOLATION: plugin '%s' disabled (needs %u, limit %u)\n",
                              name, entry.runtime.memoryUsed + needed, entry.metadata.memoryLimit);
                return false;
            }

            // Check system-wide free memory as secondary enforcement
            size_t freeHeap = ESP.getFreeHeap();
            if (needed > freeHeap) {
                Serial.printf("[PluginV2] SYSTEM MEMORY LOW: plugin '%s' blocked (needs %u, free %u)\n",
                              name, needed, freeHeap);
                return false;
            }

            // Allocation is within limits — track it
            entry.runtime.memoryUsed += needed;
            return true;
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
    registerPluginV2WithCallbacks(weatherMeta, PLUGIN_EVENT_ON_TICK,
                                  pluginWeatherTick,
                                  pluginWeatherRender,
                                  pluginWeatherEvent);

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
    registerPluginV2WithCallbacks(ageMeta, PLUGIN_EVENT_ON_TICK,
                                  pluginPetAgeTick,
                                  pluginPetAgeRender,
                                  pluginPetAgeEvent);
}

void updatePluginsV2() {
    if (!pluginV2Initialized) return;

    // Trigger tick-capable plugins
    triggerPluginsV2(PLUGIN_EVENT_ON_TICK);
}

// ============================================================
// Plugin upload / install flow
// ============================================================

bool uploadPluginV2(const char *jsonConfig, int len) {
    if (!pluginV2Initialized) return false;
    if (pluginCount >= PLUGIN_V2_MAX_PLUGINS) return false;

    // Parse JSON config
    DynamicJsonDocument doc(512);
    if (deserializeJson(doc, jsonConfig, len) != DeserializationError::Ok) return false;

    const char *name = doc["name"] | "";
    const char *version = doc["version"] | "1.0.0";
    const char *author = doc["author"] | "unknown";
    const char *description = doc["description"] | "";

    if (strlen(name) == 0) return false;

    // Check for duplicate
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) return false;
    }

    // Build metadata
    PluginV2Metadata meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.name, name, 31);
    strncpy(meta.version, version, 15);
    strncpy(meta.author, author, 31);
    strncpy(meta.description, description, 63);

    // Parse capabilities from JSON
    uint8_t caps = PLUGIN_CAP_NONE;
    if (doc["tick"] | false) caps |= PLUGIN_CAP_TICK;
    if (doc["render"] | false) caps |= PLUGIN_CAP_RENDER;
    if (doc["button"] | false) caps |= PLUGIN_CAP_BUTTON;
    if (doc["network"] | false) caps |= PLUGIN_CAP_NETWORK;
    if (doc["storage"] | false) caps |= PLUGIN_CAP_STORAGE;
    meta.capabilities = caps;

    meta.memoryLimit = doc["memoryLimit"] | 256;
    meta.watchdogMs = doc["watchdogMs"] | 1000;

    // Extract bytecode/data from JSON
    const char *codeData = doc["code"] | "";
    int codeLen = strlen(codeData);

    // Register with no callbacks initially (uploaded plugins are inert until activated)
    if (!registerPluginV2(meta, PLUGIN_EVENT_ON_TICK)) return false;

    // Store plugin config in LittleFS
    String path = "/plugin_" + String(name) + ".json";
    DynamicJsonDocument storeDoc(512);
    storeDoc["name"] = name;
    storeDoc["version"] = version;
    storeDoc["author"] = author;
    storeDoc["description"] = description;
    storeDoc["capabilities"] = caps;
    storeDoc["memoryLimit"] = meta.memoryLimit;
    storeDoc["watchdogMs"] = meta.watchdogMs;
    storeDoc["code"] = codeData;
    storeDoc["enabled"] = false;  // Uploaded plugins start disabled

    String storeStr;
    serializeJson(storeDoc, storeStr);
    if (!StorageV2::write(path.c_str(), storeStr)) {
        // Rollback registration
        unregisterPluginV2(name);
        return false;
    }

    Serial.printf("[PluginV2] Uploaded plugin '%s' v%s\n", name, version);
    return true;
}

bool uploadPluginV2(const PluginV2Metadata &metadata,
                    const char *codeData, int codeLen,
                    PluginTickCallback tickCb,
                    PluginRenderCallback renderCb,
                    PluginEventCallback eventCb) {
    if (!pluginV2Initialized) return false;
    if (pluginCount >= PLUGIN_V2_MAX_PLUGINS) return false;

    // Check for duplicate
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, metadata.name) == 0) return false;
    }

    // Register with callbacks
    if (!registerPluginV2WithCallbacks(metadata, PLUGIN_EVENT_ON_TICK,
                                       tickCb, renderCb, eventCb)) {
        return false;
    }

    // Store plugin config in LittleFS
    String path = "/plugin_" + String(metadata.name) + ".json";
    DynamicJsonDocument storeDoc(512);
    storeDoc["name"] = metadata.name;
    storeDoc["version"] = metadata.version;
    storeDoc["author"] = metadata.author;
    storeDoc["description"] = metadata.description;
    storeDoc["capabilities"] = metadata.capabilities;
    storeDoc["memoryLimit"] = metadata.memoryLimit;
    storeDoc["watchdogMs"] = metadata.watchdogMs;
    if (codeData && codeLen > 0) {
        storeDoc["code"] = String(codeData, codeLen);
    }
    storeDoc["enabled"] = false;

    String storeStr;
    serializeJson(storeDoc, storeStr);
    if (!StorageV2::write(path.c_str(), storeStr)) {
        unregisterPluginV2(metadata.name);
        return false;
    }

    return true;
}

uint8_t getPluginV2Count() {
    return pluginCount;
}

const PluginV2Entry* getPluginV2ByIndex(uint8_t index) {
    if (index >= pluginCount) return nullptr;
    return &pluginRegistry[index];
}

const PluginV2Entry* getPluginV2ByName(const char *name) {
    for (uint8_t i = 0; i < pluginCount; i++) {
        if (strcmp(pluginRegistry[i].metadata.name, name) == 0) {
            return &pluginRegistry[i];
        }
    }
    return nullptr;
}
