#ifndef PLUGIN_V2_H
#define PLUGIN_V2_H

#include <Arduino.h>
#include "config_v2.h"
#include "Plugin.h"

// ============================================================
// Phase 24.4: Plugin System v2
// Extended plugin system with LVGL-based UI rendering,
// sandboxing, metadata format, and example plugins.
// ============================================================

#define PLUGIN_V2_VERSION "2.0.0"
#define PLUGIN_V2_MAX_PLUGINS 8
#define PLUGIN_V2_MAX_MEMORY 1024  // Max memory per plugin in bytes
#define PLUGIN_V2_WATCHDOG_MS 1000 // Max execution time per plugin

// Plugin v2 capabilities (flags)
enum PluginCapability {
    PLUGIN_CAP_NONE = 0,
    PLUGIN_CAP_TICK = 1 << 0,       // Runs on every tick
    PLUGIN_CAP_RENDER = 1 << 1,     // Has LVGL render callback
    PLUGIN_CAP_BUTTON = 1 << 2,     // Handles button input
    PLUGIN_CAP_NETWORK = 1 << 3,    // Uses network
    PLUGIN_CAP_STORAGE = 1 << 4,    // Uses storage
};

// Plugin v2 metadata
struct PluginV2Metadata {
    char name[32];
    char version[16];
    char author[32];
    char description[64];
    uint8_t capabilities;       // PluginCapability flags
    uint16_t memoryLimit;       // Memory limit in bytes
    uint16_t watchdogMs;        // Watchdog timeout in ms
};

// Plugin v2 runtime state
struct PluginV2Runtime {
    bool enabled;
    bool active;
    bool crashed;               // Set true if plugin crashed (auto-disabled)
    unsigned long lastTickMs;
    uint32_t totalTicks;
    uint32_t watchdogTriggers;
    uint32_t crashCount;
    uint16_t memoryUsed;
};

// Plugin v2 callback types
typedef void (*PluginTickCallback)(PluginEvent event);
#ifdef ESP32
#include <lvgl.h>
typedef void (*PluginRenderCallback)(lv_obj_t *parent);
#else
// In native/test builds, lv_obj_t is not available; use void* as placeholder
typedef void (*PluginRenderCallback)(void *parent);
#endif
typedef void (*PluginEventCallback)(PluginEvent event, int32_t data);

// Plugin v2 entry
struct PluginV2Entry {
    PluginV2Metadata metadata;
    PluginV2Runtime runtime;
    PluginInfo base;            // Legacy plugin info
    // Callback pointers (nullptr = not set)
    PluginTickCallback onTick;
    PluginRenderCallback onRender;
    PluginEventCallback onEvent;
};

// Initialize plugin v2 system
void initPluginV2();

// Load plugins from storage
void loadPluginsV2();

// Save plugin state to storage
void savePluginsV2();

// Register a plugin v2
bool registerPluginV2(const PluginV2Metadata &metadata, PluginEvent event);

// Register a plugin v2 with callbacks
bool registerPluginV2WithCallbacks(const PluginV2Metadata &metadata,
                                   PluginEvent event,
                                   PluginTickCallback tickCb,
                                   PluginRenderCallback renderCb,
                                   PluginEventCallback eventCb);

// Unregister a plugin v2
bool unregisterPluginV2(const char *name);

// Enable/disable a plugin
bool enablePluginV2(const char *name);
bool disablePluginV2(const char *name);

// Execute all plugins for a given event (with watchdog)
void triggerPluginsV2(PluginEvent event);

// Execute a specific plugin by name (with crash isolation)
bool executePluginV2(const char *name, PluginEvent event, bool active = true);

// Render a plugin's UI element using LVGL
// Returns lv_obj_t* on ESP32, nullptr in native/test builds
#ifdef ESP32
lv_obj_t* renderPluginV2(const char *name, lv_obj_t *parent);
#else
void* renderPluginV2(const char *name, void *parent);
#endif

// Get plugin render data (for LVGL widgets)
String getPluginRenderJson();

// Get plugin list as JSON
String getPluginsV2Json();

// Get plugin status as JSON
String getPluginStatusJson(const char *name);

// Plugin sandbox: check memory limit
bool checkPluginMemory(const char *name, uint16_t needed);

// Plugin sandbox: enforce memory limit (disables plugin if over limit)
bool enforcePluginMemory(const char *name, uint16_t needed);

// Register built-in v2 plugins
void registerBuiltInPluginsV2();

// Task update (call in main loop)
void updatePluginsV2();

// Plugin upload/install flow
// Accepts plugin metadata + bytecode/config JSON, stores in LittleFS, registers it
bool uploadPluginV2(const char *jsonConfig, int len);

// Plugin upload with full metadata
bool uploadPluginV2(const PluginV2Metadata &metadata,
                    const char *codeData, int codeLen,
                    PluginTickCallback tickCb,
                    PluginRenderCallback renderCb,
                    PluginEventCallback eventCb);

// Get the number of registered plugins
uint8_t getPluginV2Count();

// Get plugin entry by index (returns nullptr if out of range)
const PluginV2Entry* getPluginV2ByIndex(uint8_t index);

// Get plugin entry by name (returns nullptr if not found)
const PluginV2Entry* getPluginV2ByName(const char *name);

#endif // PLUGIN_V2_H
