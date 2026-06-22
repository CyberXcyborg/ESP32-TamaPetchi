#ifndef PLUGIN_H
#define PLUGIN_H

#include <Arduino.h>

// ============================================================
// Phase 17.4: Plugin System
// Lightweight plugin interface for extending pet behavior.
// Plugins are JSON-defined event handlers stored in SPIFFS.
// ============================================================

// Plugin event types
enum PluginEvent {
    PLUGIN_EVENT_NONE = 0,
    PLUGIN_EVENT_ON_FEED,
    PLUGIN_EVENT_ON_PLAY,
    PLUGIN_EVENT_ON_SLEEP,
    PLUGIN_EVENT_ON_CLEAN,
    PLUGIN_EVENT_ON_HEAL,
    PLUGIN_EVENT_ON_TICK,
    PLUGIN_EVENT_ON_EVOLVE,
    PLUGIN_EVENT_ON_DEATH,
    PLUGIN_EVENT_ON_NAME_CHANGE,
    PLUGIN_EVENT_COUNT
};

// Plugin info structure
struct PluginInfo {
    char name[32];
    char version[16];
    char description[64];
    bool enabled;
    PluginEvent event;
    char action[128];  // Action to take when event fires (e.g., play melody, change stat)
};

// Plugin API
void initPluginManager();
void loadPluginsFromStorage();
void savePluginsToStorage();

// Execute plugins for a given event
void triggerPluginEvent(PluginEvent event);

// Get list of loaded plugins as JSON
String getPluginsJson();

// Plugin management
bool enablePlugin(const char* name);
bool disablePlugin(const char* name);
bool uploadPlugin(const char* json, int len);
bool deletePlugin(const char* name);

// Built-in plugins
void registerBuiltInPlugins();

// Build-in plugin: Holiday Events
void pluginHolidayEvents(PluginEvent event);

// Built-in plugin: Weather Mood
void pluginWeatherMood(PluginEvent event);

#endif // PLUGIN_H
