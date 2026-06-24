// Stub implementations for hardware-dependent functions
// Allows Pet.cpp and other modules to link in native test environment
#include "Arduino.h"
#include "Pet.h"
#include "Achievements.h"
#include "config.h"

// --- Buzzer stubs ---
// (Full buzzer melody implementations moved to test_impls.cpp)

// --- RGB LED stub ---
void initRGBLED() {}
void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {}

// --- Button stub ---
void initButtons() {}

// --- OLED stub ---
void initOLED() {}
void updateOLEDDisplay() {}

// --- IR Remote stub ---
void initIRRemote() {}
void handleIRRemote() {}

// --- MQTT stub ---
void initMQTT() {}
void handleMQTT() {}
void publishMqttState() {}
void publishMqttNotification(const String &msg) {}

// --- OTA stub ---
void initOTA() {}
void handleOTA() {}

// --- OTA Delta stub ---
// initOTADelta is defined in OTA_Delta_Native.cpp for native builds
void handleOTADelta() {}

// --- WiFi Manager stub ---
void initWiFiManager() {}
void handleWiFiManager() {}

// --- Power Manager stub ---
void initPowerManager() {}
void updatePowerManager() {}

// --- ESP32 Power Manager stubs for native test builds ---
#if defined(UNIT_TEST) || defined(__linux__) || defined(__APPLE__)
// Stub ESP32 RTC_DATA_ATTR macro
#ifndef RTC_DATA_ATTR
#define RTC_DATA_ATTR
#endif

// Stub ESP32 sleep functions
namespace esp_sleep {
  inline void enable_ext0_wakeup(int, int) {}
  inline void enable_timer_wakeup(unsigned long long) {}
  inline void enable_wakeup_source(int) {}
  inline int get_wakeup_cause() { return 0; }
  inline void deep_sleep_start() {}
  inline void light_sleep_start() {}
  inline void start() {}
}

// Stub ESP32 WiFi power functions
namespace esp_wifi {
  inline void set_max_tx_power(int) {}
}

// Stub WiFi functions for power management
inline void WiFi_setSleep(bool) {}
#endif

// --- Provisioning stubs (ESP32-specific functions) ---
// These stubs allow test_provisioning.cpp to link in native builds
// where WiFi.h, SPIFFS.h, and WebServer.h are not available
#if defined(UNIT_TEST) || defined(__linux__) || defined(__APPLE__)
#include <stdint.h>
// Stub for functions that Provisioning.cpp calls but tests don't need
// The actual Provisioning.cpp is not compiled in native builds
// (only test_provisioning.cpp + stubs.cpp + HAL_Native.cpp are)
#endif

// --- MultiPet stub ---
void initMultiPet() {}

// --- Stats stub ---
void initStats() {}
void updateStats() {}

// --- Notifications stub ---
void initNotifications() {}

// --- Web Handlers stub ---
void initWebServer() {}
void handleWebServer() {}

// --- Achievements stub ---
void initAchievements() {}
void checkAchievements() {}

// --- Storage savePetDataForce stub ---
void savePetDataForce(const Pet &pet) {}

// --- Master test runner for native builds ---
// Each test file defines its own run function to avoid main() collisions
int run_hal_tests();
int run_pet_statemachine_tests();
int run_provisioning_tests();
int run_power_tests();
int run_ota_delta_tests();
int run_spriteloader_tests();
int run_animation_tests();
int run_screenmanager_tests();
int run_migrated_screen_tests();
int run_phase21_tests();
int run_phase22_tests();
int run_phase22_5_tests();
int run_phase23_tests();
int run_voice_prompt_tests();
int run_data_export_tests();
int run_day_night_tests();
int run_plugin_v2_tests();

int main() {
  printf("=== TamaPetchi Native Unit Tests ===\\n\\n");
  run_hal_tests();
  printf("\\n");
  run_pet_statemachine_tests();
  printf("\\n");
  run_provisioning_tests();
  printf("\\n");
  run_power_tests();
  printf("\\n");
  run_ota_delta_tests();
  printf("\\n");
  run_spriteloader_tests();
  printf("\\n");
  run_animation_tests();
  printf("\\n");
  run_screenmanager_tests();
  printf("\\n");
  run_migrated_screen_tests();
  printf("\\n=== MIGRATED SCREENS DONE ===\\n");
  fflush(stdout);
  printf("About to call run_phase21_tests...\\n");
  fflush(stdout);
  run_phase21_tests();
  printf("\\n=== PHASE 21 DONE ===\\n");
  fflush(stdout);
  printf("About to call run_phase22_tests...\\n");
  fflush(stdout);
  run_phase22_tests();
  printf("\\n=== PHASE 22 DONE ===\\n");
  fflush(stdout);
  printf("About to call run_phase22_5_tests...\\n");
  fflush(stdout);
  run_phase22_5_tests();
  printf("\\n=== PHASE 22.5 DONE ===\\n");
  fflush(stdout);
  printf("About to call run_phase23_tests...\\n");
  fflush(stdout);
  run_phase23_tests();
  printf("\\n=== PHASE 23 DONE ===\\n");
  fflush(stdout);
  printf("About to call run_voice_prompt_tests...\\n");
  fflush(stdout);
  run_voice_prompt_tests();
  printf("\\n=== VOICE PROMPTS DONE ===\\n");
  fflush(stdout);
  printf("About to call run_data_export_tests...\\n");
  fflush(stdout);
  run_data_export_tests();
  printf("\\n=== DATA EXPORT DONE ===\\n");
  fflush(stdout);
  printf("About to call run_day_night_tests...\\n");
  fflush(stdout);
  run_day_night_tests();
  printf("\\n=== DAY/NIGHT DONE ===\\n");
  fflush(stdout);
  printf("About to call run_plugin_v2_tests...\\n");
  fflush(stdout);
  run_plugin_v2_tests();
  printf("\\n=== PLUGIN V2 DONE ===\\n");
  fflush(stdout);
  printf("\\n=== ALL DONE ===\\n");
  fflush(stdout);
  printf("\\n=== All native tests PASSED ===\\n");
  return 0;
}
