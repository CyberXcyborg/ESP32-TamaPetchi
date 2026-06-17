// Stub implementations for hardware-dependent functions
// Allows Pet.cpp and other modules to link in native test environment
#include "Arduino.h"
#include "Pet.h"
#include "config.h"

// --- Buzzer stubs ---
void soundFeed() {}
void soundPlay() {}
void soundDeath() {}
void soundWake() {}
void playMelody(const int *melody, int length, int tempo) {}
void playStateMelody(const Pet &pet) {}
void stopMusic() {}
void playMelodyById(int melodyId) {}
void setMelodyConfig(int event, int melodyIndex) {}
int getMelodyConfig(int event) { return 0; }
String getMelodyConfigJson() { return "{}"; }
void setMelodyConfigFromJson(const String &json) {}

// --- Melody library stubs ---
int melodyConfig[MELODY_COUNT] = {0,1,2,3,4,5,6,7};
const int *melodyLibrary[8] = {nullptr};
const int melodyLengths[8] = {0};
const char *melodyNames[8] = {nullptr};
const int melodyCount = 0;

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
void initOTADelta() {}
void handleOTADelta() {}

// --- WiFi Manager stub ---
void initWiFiManager() {}
void handleWiFiManager() {}

// --- Power Manager stub ---
void initPowerManager() {}
void updatePowerManager() {}

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
