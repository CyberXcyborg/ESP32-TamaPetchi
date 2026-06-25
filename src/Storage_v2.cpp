// ============================================================
// Storage_v2.cpp — LittleFS Storage Implementation
// ============================================================

#include "Storage_v2.h"
#include <LittleFS.h>
#include <FS.h>

bool StorageV2::_mounted = false;

bool StorageV2::begin() {
    if (!_mounted) {
        _mounted = LittleFS.begin(true);  // true = format if mount fails
    }
    return _mounted;
}

bool StorageV2::exists(const char *path) {
    if (!_mounted) return false;
    return LittleFS.exists(path);
}

String StorageV2::read(const char *path) {
    if (!_mounted || !LittleFS.exists(path)) return String();
    
    File file = LittleFS.open(path, "r");
    if (!file) return String();
    
    String content = file.readString();
    file.close();
    return content;
}

bool StorageV2::write(const char *path, const String &data) {
    if (!_mounted) return false;
    
    File file = LittleFS.open(path, "w");
    if (!file) return false;
    
    size_t written = file.print(data);
    file.close();
    
    return written == data.length();
}

bool StorageV2::remove(const char *path) {
    if (!_mounted) return false;
    return LittleFS.remove(path);
}

bool StorageV2::mkdir(const char *path) {
    if (!_mounted) return false;
    return LittleFS.mkdir(path);
}

bool StorageV2::append(const char *path, const String &data) {
    if (!_mounted) return false;
    File file = LittleFS.open(path, "a");
    if (!file) return false;
    size_t written = file.print(data);
    file.close();
    return written == data.length();
}

File StorageV2::open(const char *path, const char *mode) {
    if (!_mounted) return File();
    return LittleFS.open(path, mode);
}

bool StorageV2::format() {
    if (_mounted) {
        LittleFS.end();
        _mounted = false;
    }
    return LittleFS.format();
}
