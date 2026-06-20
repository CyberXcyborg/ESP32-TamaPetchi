// HAL_Native.cpp — Native (test) HAL implementation
// Returns fixed/mock values for unit testing without hardware

#include "HAL.h"
#include <cstring>
#include <cstdio>

// ============================================================
// Native Display — logs to stdout
// ============================================================
class NativeDisplay : public IDisplay {
public:
  void init() override { /* no-op */ }
  void clear() override {}
  void showText(const char *text, int x, int y) override {
    (void)x; (void)y;
    // Suppress in tests to keep output clean
  }
  void showStatusBar(const char *status) override { (void)status; }
  void showPetState(const char *state, int, int, int, int) override {
    (void)state;
  }
  void showNotification(const char *msg) override { (void)msg; }
  void refresh() override {}
};

// ============================================================
// Native Storage — in-memory key-value store
// ============================================================
class NativeStorage : public IStorage {
private:
  static const int MAX_FILES = 32;
  static const int MAX_PATH = 64;
  static const int MAX_DATA = 4096;
  struct FileEntry {
    char path[MAX_PATH];
    char data[MAX_DATA];
    size_t len;
    bool used;
  };
  FileEntry files[MAX_FILES];

  int findFile(const char *path) {
    for (int i = 0; i < MAX_FILES; i++) {
      if (files[i].used && strcmp(files[i].path, path) == 0) return i;
    }
    return -1;
  }

public:
  NativeStorage() {
    memset(files, 0, sizeof(files));
  }

  bool begin(bool = false) override { return true; }

  bool exists(const char *path) override {
    return findFile(path) >= 0;
  }

  String readFile(const char *path) override {
    int idx = findFile(path);
    if (idx < 0) return "";
    return String(files[idx].data);
  }

  bool writeFile(const char *path, const String &data) override {
    int idx = findFile(path);
    if (idx < 0) {
      // Find free slot
      for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) { idx = i; break; }
      }
    }
    if (idx < 0) return false;
    strncpy(files[idx].path, path, MAX_PATH - 1);
    strncpy(files[idx].data, data.c_str(), MAX_DATA - 1);
    files[idx].len = data.length();
    files[idx].used = true;
    return true;
  }

  bool remove(const char *path) override {
    int idx = findFile(path);
    if (idx < 0) return false;
    files[idx].used = false;
    return true;
  }

  bool format() override {
    memset(files, 0, sizeof(files));
    return true;
  }
};

// ============================================================
// Native WiFi — always "connected" with fixed IP
// ============================================================
class NativeWiFi : public IWiFi {
  bool connected = true;
  bool apActive = false;

public:
  bool begin(const char *, const char *) override {
    connected = true;
    return true;
  }
  bool startAP(const char *, const char *) override {
    apActive = true;
    return true;
  }
  String getLocalIP() override { return "192.168.1.42"; }
  String getAPIP() override { return "192.168.4.1"; }
  bool isConnected() override { return connected; }
  void disconnect() override { connected = false; }
  void setTxPower(int) override {}
  void enableSleep() override {}
  void disableSleep() override {}
};

// ============================================================
// Native GPIO — all pins read 0, writes are no-ops
// ============================================================
class NativeGPIO : public IGPIO {
  int pinValues[40] = {0};

public:
  int readPin(int pin) override {
    if (pin >= 0 && pin < 40) return pinValues[pin];
    return 0;
  }
  void writePin(int pin, int value) override {
    if (pin >= 0 && pin < 40) pinValues[pin] = value;
  }
  void setPinMode(int, int) override {}
  void playTone(int, int, int) override {}
  void stopTone(int) override {}
  int readAnalog(int) override { return 2048; } // mid-range
};

// ============================================================
// Native Buzzer — no-op
// ============================================================
class NativeBuzzer : public IBuzzer {
public:
  void init(int, int) override {}
  void playNote(int, int) override {}
  void playMelody(const int *, const int *, int) override {}
  void stopAll() override {}
};

// ============================================================
// Native Power — no-op (can't sleep in test)
// ============================================================
class NativePower : public IPower {
public:
  void enableWakeOnPin(int, int) override {}
  void enableWakeOnTimer(unsigned long) override {}
  void enterDeepSleep() override {} // no-op in test
  int getWakeupCause() override { return 0; }
  int readBatteryPercent() override { return 75; } // fixed mock value
};

// ============================================================
// Native RTC — in-memory buffer
// ============================================================
class NativeRTC : public IRTC {
  uint8_t buffer[256] = {0};

public:
  void writeData(uint32_t offset, const uint8_t *data, size_t len) override {
    if (offset + len <= 256) memcpy(buffer + offset, data, len);
  }
  void readData(uint32_t offset, uint8_t *data, size_t len) override {
    if (offset + len <= 256) memcpy(data, buffer + offset, len);
  }
  void writeWord(uint32_t offset, uint32_t value) override {
    writeData(offset, (const uint8_t *)&value, sizeof(value));
  }
  uint32_t readWord(uint32_t offset) override {
    uint32_t value = 0;
    readData(offset, (uint8_t *)&value, sizeof(value));
    return value;
  }
};

// ============================================================
// Singleton instances
// ============================================================
static NativeDisplay s_display;
static NativeStorage s_storage;
static NativeWiFi s_wifi;
static NativeGPIO s_gpio;
static NativeBuzzer s_buzzer;
static NativePower s_power;
static NativeRTC s_rtc;

IDisplay &getDisplay()  { return s_display; }
IStorage &getStorage()  { return s_storage; }
IWiFi    &getWiFi()     { return s_wifi; }
IGPIO    &getGPIO()     { return s_gpio; }
IBuzzer  &getBuzzer()   { return s_buzzer; }
IPower   &getPower()    { return s_power; }
IRTC     &getRTC()      { return s_rtc; }
