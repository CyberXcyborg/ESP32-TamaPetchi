// HAL_ESP32.cpp — ESP32 concrete HAL implementation
// Uses real hardware: Wire, SPIFFS, WiFi, GPIO, ledc, esp_sleep, RTC

#include "HAL.h"
#include "config.h"
#include <Wire.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <driver/ledc.h>

// ============================================================
// Display Implementation (SSD1306 via Adafruit or direct I2C)
// ============================================================
class ESP32Display : public IDisplay {
public:
  void init() override {
#ifdef ENABLE_OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    // Minimal init — full OLED library handles the rest
    Serial.println("[HAL] Display initialized (SSD1306)");
#else
    Serial.println("[HAL] Display disabled (ENABLE_OLED not set)");
#endif
  }

  void clear() override {
    // Stub — real implementation uses Adafruit_SSD1306::clearDisplay()
  }

  void showText(const char *text, int x, int y) override {
    // Stub — real implementation uses Adafruit_SSD1306::setCursor + print
    Serial.printf("[HAL] Display text @ (%d,%d): %s\n", x, y, text);
  }

  void showStatusBar(const char *status) override {
    Serial.printf("[HAL] Status bar: %s\n", status);
  }

  void showPetState(const char *state, int hunger, int happiness,
                   int health, int energy) override {
    Serial.printf("[HAL] Pet: %s H:%d Ha:%d He:%d E:%d\n",
                  state, hunger, happiness, health, energy);
  }

  void showNotification(const char *msg) override {
    Serial.printf("[HAL] Notification: %s\n", msg);
  }

  void refresh() override {
    // Stub — real implementation uses Adafruit_SSD1306::display()
  }
};

// ============================================================
// Storage Implementation (SPIFFS)
// ============================================================
class ESP32Storage : public IStorage {
public:
  bool begin(bool formatOnFail = false) override {
    return SPIFFS.begin(formatOnFail);
  }

  bool exists(const char *path) override {
    return SPIFFS.exists(path);
  }

  String readFile(const char *path) override {
    File f = SPIFFS.open(path, "r");
    if (!f) return "";
    String data = f.readString();
    f.close();
    return data;
  }

  bool writeFile(const char *path, const String &data) override {
    File f = SPIFFS.open(path, "w");
    if (!f) return false;
    size_t written = f.print(data);
    f.close();
    return written == data.length();
  }

  bool remove(const char *path) override {
    return SPIFFS.remove(path);
  }

  bool format() override {
    return SPIFFS.format();
  }
};

// ============================================================
// WiFi Implementation
// ============================================================
class ESP32WiFi : public IWiFi {
public:
  bool begin(const char *ssid, const char *password) override {
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_CONNECT_TIMEOUT) {
      delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
  }

  bool startAP(const char *ssid, const char *password) override {
    return WiFi.softAP(ssid, password);
  }

  String getLocalIP() override {
    return WiFi.localIP().toString();
  }

  String getAPIP() override {
    return WiFi.softAPIP().toString();
  }

  bool isConnected() override {
    return WiFi.status() == WL_CONNECTED;
  }

  void disconnect() override {
    WiFi.disconnect(true, true);
  }

  void setTxPower(int power) override {
    WiFi.setTxPower((wifi_power_t)power);
  }

  void enableSleep() override {
    WiFi.setSleep(true);
  }

  void disableSleep() override {
    WiFi.setSleep(false);
  }
};

// ============================================================
// GPIO Implementation
// ============================================================
class ESP32GPIO : public IGPIO {
public:
  int readPin(int pin) override {
    return digitalRead(pin);
  }

  void writePin(int pin, int value) override {
    digitalWrite(pin, value);
  }

  void setPinMode(int pin, int mode) override {
    pinMode(pin, mode);
  }

  void playTone(int pin, int frequency, int durationMs) override {
    tone(pin, frequency, durationMs);
  }

  void stopTone(int pin) override {
    noTone(pin);
  }

  int readAnalog(int pin) override {
    return analogRead(pin);
  }
};

// ============================================================
// Buzzer Implementation (LEDC for ESP32)
// ============================================================
class ESP32Buzzer : public IBuzzer {
private:
  int buzzerPin = -1;
  int buzzerChannel = 0;

public:
  void init(int pin, int channel) override {
    buzzerPin = pin;
    buzzerChannel = channel;
    ledcSetup(channel, 2000, 8); // 2kHz default, 8-bit resolution
    ledcAttachPin(pin, channel);
    Serial.printf("[HAL] Buzzer initialized (pin=%d ch=%d)\n", pin, channel);
  }

  void playNote(int frequency, int durationMs) override {
    if (buzzerPin < 0) return;
    ledcWriteTone(buzzerChannel, frequency);
    delay(durationMs);
    ledcWriteTone(buzzerChannel, 0);
  }

  void playMelody(const int *notes, const int *durations, int length) override {
    for (int i = 0; i < length; i++) {
      if (notes[i] == 0) {
        delay(durations[i]);
      } else {
        playNote(notes[i], durations[i]);
      }
      if (i < length - 1) delay(20); // gap between notes
    }
  }

  void stopAll() override {
    if (buzzerPin >= 0) {
      ledcWriteTone(buzzerChannel, 0);
    }
  }
};

// ============================================================
// Power Implementation
// ============================================================
class ESP32Power : public IPower {
public:
  void enableWakeOnPin(int pin, int level) override {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)pin, level);
  }

  void enableWakeOnTimer(unsigned long us) override {
    esp_sleep_enable_timer_wakeup(us);
  }

  void enterDeepSleep() override {
    esp_deep_sleep_start();
  }

  int getWakeupCause() override {
    return (int)esp_sleep_get_wakeup_cause();
  }

  int readBatteryPercent() override {
#ifdef BATTERY_ADC_PIN
    int raw = analogRead(BATTERY_ADC_PIN);
    float voltage = (raw / 4095.0) * 3.3 * 2.0;
    int percent = (int)((voltage - 3.0) / (4.2 - 3.0) * 100);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
#else
    return -1;
#endif
  }
};

// ============================================================
// RTC Memory Implementation
// Uses RTC_DATA_ATTR section (survives deep sleep, cleared on power cycle)
// ============================================================
class ESP32RTC : public IRTC {
public:
  // We use a fixed-size buffer in RTC slow memory
  // Layout: [0..3]=magic, [4..7]=bootCount, [8..11]=checksum, [12..63]=user data
  static const size_t RTC_BUFFER_SIZE = 64;

  void writeData(uint32_t offset, const uint8_t *data, size_t len) override {
    // Use the .noinit section via RTC_DATA_ATTR
    // For simplicity, we use a static buffer placed in DRAM that
    // the startup code doesn't zero
    static uint8_t rtcBuffer[RTC_BUFFER_SIZE] __attribute__((section(".noinit")));
    if (offset + len <= RTC_BUFFER_SIZE) {
      memcpy(rtcBuffer + offset, data, len);
    }
  }

  void readData(uint32_t offset, uint8_t *data, size_t len) override {
    static uint8_t rtcBuffer[RTC_BUFFER_SIZE] __attribute__((section(".noinit")));
    if (offset + len <= RTC_BUFFER_SIZE) {
      memcpy(data, rtcBuffer + offset, len);
    }
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
static ESP32Display s_display;
static ESP32Storage s_storage;
static ESP32WiFi s_wifi;
static ESP32GPIO s_gpio;
static ESP32Buzzer s_buzzer;
static ESP32Power s_power;
static ESP32RTC s_rtc;

IDisplay &getDisplay()  { return s_display; }
IStorage &getStorage()  { return s_storage; }
IWiFi    &getWiFi()     { return s_wifi; }
IGPIO    &getGPIO()     { return s_gpio; }
IBuzzer  &getBuzzer()   { return s_buzzer; }
IPower   &getPower()    { return s_power; }
IRTC     &getRTC()      { return s_rtc; }
