#ifndef HAL_H
#define HAL_H

// ============================================================
// Hardware Abstraction Layer (HAL) — Phase 13.2
//
// Provides abstract interfaces for all hardware peripherals so
// that:
// 1. Unit tests can run on native (Linux/macOS) without ESP32
// 2. Hardware can be swapped (e.g. different OLED, different
//    board) without touching business logic
// 3. Compile-time feature flags still work (stub out unused hw)
//
// Implementations:
//   HAL_ESP32.cpp  — Real ESP32 hardware (Wire, SPIFFS, GPIO)
//   HAL_Native.cpp — Native (test) stubs returning fixed values
//
// Selection: the build system includes the right .cpp based on
// the platform. ESP32 builds compile HAL_ESP32.cpp; native
// builds compile HAL_Native.cpp.
// ============================================================

#include <Arduino.h>

// ============================================================
// Display Interface (OLED)
// ============================================================
class IDisplay {
public:
  virtual ~IDisplay() {}
  virtual void init() = 0;
  virtual void clear() = 0;
  virtual void showText(const char *text, int x, int y) = 0;
  virtual void showStatusBar(const char *status) = 0;
  virtual void showPetState(const char *state, int hunger, int happiness,
                           int health, int energy) = 0;
  virtual void showNotification(const char *msg) = 0;
  virtual void refresh() = 0;
};

// ============================================================
// Storage Interface (SPIFFS / filesystem)
// ============================================================
class IStorage {
public:
  virtual ~IStorage() {}
  virtual bool begin(bool formatOnFail = false) = 0;
  virtual bool exists(const char *path) = 0;
  virtual String readFile(const char *path) = 0;
  virtual bool writeFile(const char *path, const String &data) = 0;
  virtual bool remove(const char *path) = 0;
  virtual bool format() = 0;
};

// ============================================================
// WiFi Interface
// ============================================================
class IWiFi {
public:
  virtual ~IWiFi() {}
  virtual bool begin(const char *ssid, const char *password) = 0;
  virtual bool startAP(const char *ssid, const char *password) = 0;
  virtual String getLocalIP() = 0;
  virtual String getAPIP() = 0;
  virtual bool isConnected() = 0;
  virtual void disconnect() = 0;
  virtual void setTxPower(int power) = 0;
  virtual void enableSleep() = 0;
  virtual void disableSleep() = 0;
};

// ============================================================
// GPIO Interface (buttons, buzzer, RGB LED)
// ============================================================
class IGPIO {
public:
  virtual ~IGPIO() {}
  virtual int readPin(int pin) = 0;
  virtual void writePin(int pin, int value) = 0;
  virtual void setPinMode(int pin, int mode) = 0;
  virtual void playTone(int pin, int frequency, int durationMs) = 0;
  virtual void stopTone(int pin) = 0;
  virtual int readAnalog(int pin) = 0;
};

// ============================================================
// Buzzer Interface
// ============================================================
class IBuzzer {
public:
  virtual ~IBuzzer() {}
  virtual void init(int pin, int channel) = 0;
  virtual void playNote(int frequency, int durationMs) = 0;
  virtual void playMelody(const int *notes, const int *durations,
                         int length) = 0;
  virtual void stopAll() = 0;
};

// ============================================================
// Power Interface (deep sleep, wake, battery)
// ============================================================
class IPower {
public:
  virtual ~IPower() {}
  virtual void enableWakeOnPin(int pin, int level) = 0;
  virtual void enableWakeOnTimer(unsigned long us) = 0;
  virtual void enterDeepSleep() = 0;
  virtual int getWakeupCause() = 0;
  virtual int readBatteryPercent() = 0;
};

// ============================================================
// RTC Memory Interface (survives deep sleep)
// ============================================================
class IRTC {
public:
  virtual ~IRTC() {}
  virtual void writeData(uint32_t offset, const uint8_t *data, size_t len) = 0;
  virtual void readData(uint32_t offset, uint8_t *data, size_t len) = 0;
  virtual void writeWord(uint32_t offset, uint32_t value) = 0;
  virtual uint32_t readWord(uint32_t offset) = 0;
};

// ============================================================
// HAL Factory — returns the right implementation
// The concrete type is selected at compile time.
// ============================================================
#if defined(UNIT_TEST) || defined(__linux__) || defined(__APPLE__)
  #define HAL_NATIVE
#else
  #define HAL_ESP32
#endif

// Factory functions — each returns a singleton of the right impl
IDisplay &getDisplay();
IStorage &getStorage();
IWiFi &getWiFi();
IGPIO &getGPIO();
IBuzzer &getBuzzer();
IPower &getPower();
IRTC &getRTC();

#endif // HAL_H
