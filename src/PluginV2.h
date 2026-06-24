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
    unsigned long lastTickMs;
    uint32_t totalTicks;
    uint32_t watchdogTriggers;
    uint16_t memoryUsed;
};

// Plugin v2 entry
struct PluginV2Entry {
    PluginV2Metadata metadata;
    PluginV2Runtime runtime;
    PluginInfo base;            // Legacy plugin info
};

// Initialize plugin v2 system
void initPluginV2();

// Load plugins from storage
void loadPluginsV2();

// Save plugin state to storage
void savePluginsV2();

// Register a plugin v2
bool registerPluginV2(const PluginV2Metadata &metadata, PluginEvent event);

// Unregister a plugin v2
bool unregisterPluginV2(const char *name);

// Enable/disable a plugin
bool enablePluginV2(const char *name);
bool disablePluginV2(const char *name);

// Execute all plugins for a given event (with watchdog)
void triggerPluginsV2(PluginEvent event);

// Get plugin render data (for LVGL widgets)
String getPluginRenderJson();

// Get plugin list as JSON
String getPluginsV2Json();

// Get plugin status as JSON
String getPluginStatusJson(const char *name);

// Plugin sandbox: check memory limit
bool checkPluginMemory(const char *name, uint16_t needed);

// Register built-in v2 plugins
void registerBuiltInPluginsV2();

// Task update (call in main loop)
void updatePluginsV2();

#endif // PLUGIN_V2_H
