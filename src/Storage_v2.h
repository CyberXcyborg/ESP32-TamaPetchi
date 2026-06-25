// ============================================================
// Storage_v2.h — LittleFS Storage for v2.0
// Replaces SPIFFS with LittleFS (ESP-IDF native)
// ============================================================

#ifndef STORAGE_V2_H
#define STORAGE_V2_H

#include <Arduino.h>
#include <FS.h>

class StorageV2 {
public:
    static bool begin();
    static bool exists(const char *path);
    static bool mkdir(const char *path);
    static String read(const char *path);
    static bool write(const char *path, const String &data);
    static bool append(const char *path, const String &data);
    static bool remove(const char *path);
    static bool format();
    
    // File access (for directory listing, etc.)
    static File open(const char *path, const char *mode = "r");
    
private:
    static bool _mounted;
};

#endif // STORAGE_V2_H
