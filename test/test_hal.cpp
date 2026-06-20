// test_hal.cpp — Unit tests for Hardware Abstraction Layer
#include "HAL.h"
#include <cassert>
#include <cstdio>

// ============================================================
// Test: Native Display — all methods callable without crash
// ============================================================
void test_display_basic() {
  IDisplay &disp = getDisplay();
  disp.init();
  disp.clear();
  disp.showText("Hello", 0, 0);
  disp.showStatusBar("Test");
  disp.showPetState("normal", 50, 60, 70, 80);
  disp.showNotification("Test notification");
  disp.refresh();
  printf("  PASS: Display basic operations\n");
}

// ============================================================
// Test: Native Storage — write, read, exists, remove
// ============================================================
void test_storage_basic() {
  IStorage &stor = getStorage();
  assert(stor.begin(true));

  // Write and read
  assert(stor.writeFile("/test.txt", "hello world"));
  assert(stor.exists("/test.txt"));
  String data = stor.readFile("/test.txt");
  assert(data == "hello world");

  // Non-existent file
  assert(!stor.exists("/nonexistent.txt"));
  String empty = stor.readFile("/nonexistent.txt");
  assert(empty == "");

  // Remove
  assert(stor.remove("/test.txt"));
  assert(!stor.exists("/test.txt"));

  printf("  PASS: Storage basic operations\n");
}

// ============================================================
// Test: Native Storage — format clears all files
// ============================================================
void test_storage_format() {
  IStorage &stor = getStorage();
  assert(stor.begin(true));
  assert(stor.writeFile("/a.txt", "aaa"));
  assert(stor.writeFile("/b.txt", "bbb"));
  assert(stor.exists("/a.txt"));
  assert(stor.exists("/b.txt"));

  assert(stor.format());
  assert(!stor.exists("/a.txt"));
  assert(!stor.exists("/b.txt"));

  printf("  PASS: Storage format\n");
}

// ============================================================
// Test: Native WiFi — connect, AP, disconnect
// ============================================================
void test_wifi_basic() {
  IWiFi &wifi = getWiFi();
  assert(wifi.begin("test_ssid", "test_pass"));
  assert(wifi.isConnected());
  assert(wifi.getLocalIP() == "192.168.1.42");

  assert(wifi.startAP("TamaPetchi-Setup", "tamapetchi"));
  assert(wifi.getAPIP() == "192.168.4.1");

  wifi.disconnect();
  assert(!wifi.isConnected());

  printf("  PASS: WiFi basic operations\n");
}

// ============================================================
// Test: Native GPIO — read/write digital, analog
// ============================================================
void test_gpio_basic() {
  IGPIO &gpio = getGPIO();
  gpio.setPinMode(0, 0); // INPUT
  gpio.writePin(1, 1);
  int val = gpio.readPin(1);
  assert(val == 1);

  int analog = gpio.readAnalog(34);
  assert(analog == 2048); // mock mid-range

  printf("  PASS: GPIO basic operations\n");
}

// ============================================================
// Test: Native Buzzer — init and play without crash
// ============================================================
void test_buzzer_basic() {
  IBuzzer &buzz = getBuzzer();
  buzz.init(25, 0);
  buzz.playNote(1000, 50);

  int notes[] = {262, 294, 330};
  int durations[] = {100, 100, 100};
  buzz.playMelody(notes, durations, 3);
  buzz.stopAll();

  printf("  PASS: Buzzer basic operations\n");
}

// ============================================================
// Test: Native Power — battery reading, wake config
// ============================================================
void test_power_basic() {
  IPower &pwr = getPower();
  pwr.enableWakeOnPin(0, 0);
  pwr.enableWakeOnTimer(60000000UL); // 60 seconds
  int pct = pwr.readBatteryPercent();
  assert(pct == 75); // mock value

  printf("  PASS: Power basic operations\n");
}

// ============================================================
// Test: Native RTC — write/read words and data
// ============================================================
void test_rtc_basic() {
  IRTC &rtc = getRTC();

  // Write and read word
  rtc.writeWord(0, 0xDEADBEEF);
  uint32_t val = rtc.readWord(0);
  assert(val == 0xDEADBEEF);

  // Write and read raw data
  uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
  rtc.writeData(4, testData, 4);
  uint8_t readBack[4] = {0};
  rtc.readData(4, readBack, 4);
  for (int i = 0; i < 4; i++) {
    assert(readBack[i] == testData[i]);
  }

  printf("  PASS: RTC basic operations\n");
}

// ============================================================
// Test: Multiple storage files
// ============================================================
void test_storage_multiple_files() {
  IStorage &stor = getStorage();
  assert(stor.begin(true));

  assert(stor.writeFile("/pet_data.json", "{\"name\":\"Tama\"}"));
  assert(stor.writeFile("/stats.json", "{\"feedCount\":10}"));
  assert(stor.writeFile("/settings.json", "{\"volume\":80}"));

  assert(stor.readFile("/pet_data.json") == "{\"name\":\"Tama\"}");
  assert(stor.readFile("/stats.json") == "{\"feedCount\":10}");
  assert(stor.readFile("/settings.json") == "{\"volume\":80}");

  printf("  PASS: Storage multiple files\n");
}

// ============================================================
// Test: Storage overwrite
// ============================================================
void test_storage_overwrite() {
  IStorage &stor = getStorage();
  assert(stor.begin(true));

  assert(stor.writeFile("/data.txt", "version1"));
  assert(stor.readFile("/data.txt") == "version1");

  assert(stor.writeFile("/data.txt", "version2"));
  assert(stor.readFile("/data.txt") == "version2");

  printf("  PASS: Storage overwrite\n");
}

// ============================================================
// Test: WiFi power management
// ============================================================
void test_wifi_power() {
  IWiFi &wifi = getWiFi();
  wifi.enableSleep();
  wifi.disableSleep();
  wifi.setTxPower(8);
  printf("  PASS: WiFi power management\n");
}

// ============================================================
// Main test runner
// ============================================================
int main() {
  printf("=== HAL Unit Tests ===\n");

  test_display_basic();
  test_storage_basic();
  test_storage_format();
  test_storage_multiple_files();
  test_storage_overwrite();
  test_wifi_basic();
  test_wifi_power();
  test_gpio_basic();
  test_buzzer_basic();
  test_power_basic();
  test_rtc_basic();

  printf("\n=== All 12 HAL tests PASSED ===\n");
  return 0;
}
