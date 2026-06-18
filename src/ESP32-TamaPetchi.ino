#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Esp.h>  // For ESP.restart() and deep sleep functions

// Phase 6: Watchdog timer
#include <esp_task_wdt.h>

#include "config.h"
#include "Pet.h"
#include "Storage.h"
#include "WebHandlers.h"
#include "Achievements.h"
#include "Buttons.h"
#include "RGB_LED.h"
#include "MultiPet.h"
#include "OTA.h"
#include "WiFiManager.h"
#include "Stats.h"
#include "Notifications.h"
#include "PowerManager.h"
#include "PowerManagement.h"

// OLED display (optional - enable with -DENABLE_OLED)
#ifdef ENABLE_OLED
#include "OLED.h"
#endif

// IR Remote (optional - disable with -DENABLE_IR_REMOTE)
#ifndef DISABLE_IR_REMOTE
#include "IRRemote.h"
#endif

// MQTT (optional - disable with -DMQTT)
#ifndef DISABLE_MQTT
#include "MQTT.h"
#endif

// OTA Delta (optional - disable with -DENABLE_OTA_DELTA)
#ifndef DISABLE_OTA_DELTA
#include "OTA_Delta.h"
#endif

// Web server
WebServer server(WEB_SERVER_PORT);

// Pet instance
Pet pet;

// Phase 5: Multi-pet state
MultiPetState g_multiPet;

// Phase 5: Game statistics
GameStats g_stats;

// Wake-up message tracking (defined in WebHandlers.cpp)
extern bool showWakeMessage;
extern unsigned long wakeMessageStartTime;
extern String previousState;

// Timing
unsigned long lastUpdateTime = 0;

// Phase 6: Watchdog configuration
#define WDT_TIMEOUT 10  // 10 seconds without feed → reset

void setup() {
  Serial.begin(SERIAL_BAUD);

  // Phase 6: Initialize watchdog timer
  esp_task_wdt_init(WDT_TIMEOUT, true); // panic=true → reset on timeout
  esp_task_wdt_add(NULL); // Add current task to WDT

  // Initialize random seed
  randomSeed(analogRead(0));

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }

  // Phase 5: Initialize power manager (ADC, wake-up)
  setupPowerManager();

  // Phase 5: Connect to WiFi (with AP fallback)
  setupWiFi();

  // Phase 5: Initialize OTA
  setupOTA();

  // Phase 7.5: Initialize MQTT
#ifndef DISABLE_MQTT
  setupMQTT();
#endif

  // Phase 5: Load stats
  loadStats(g_stats);

  // Load pet data from SPIFFS
  loadPetData(pet);
  loadAchievements(pet);

  // Phase 6: Check if we woke from deep sleep and restore state
  if (wasDeepSleepWake()) {
    Serial.println("Woke from deep sleep!");
    restoreFromRTC(pet);
  }

  previousState = pet.state;

  // Register web server routes
  registerHandlers(server, pet);

  // Start server
  server.begin();

  // Initialize OLED
#ifdef ENABLE_OLED
  setupOLED();
#endif

  // Initialize buttons
  setupButtons();

  // Initialize RGB LED
  setupRGBLED();

  // Initialize IR remote
#ifndef DISABLE_IR_REMOTE
  setupIRRemote();
  setIRCommandCallback(handleIRCommand);
#endif

  // Phase 6: WiFi power optimization — reduce TX power in idle
  WiFi.setTxPower(WIFI_POWER_8_5dBm);  // Reduce from default 20dBm to save power

  Serial.println("TamaPetchi initialized — WDT active");
}

void loop() {
  // Phase 6: Feed the watchdog
  esp_task_wdt_reset();

  server.handleClient();

  // Phase 5: Handle OTA
  handleOTA();

  // Phase 7.5: Handle MQTT
#ifndef DISABLE_MQTT
  handleMQTT();
#endif

  // Phase 6: Check physical buttons
  checkButtons(pet);

  // Check IR remote
#ifndef DISABLE_IR_REMOTE
  checkIRRemote();
#endif

  // Phase 6: Handle SSE client cleanup and broadcast
  handleSSEClients();

  // Update pet stats every interval
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= PET_UPDATE_INTERVAL) {
    lastUpdateTime = currentMillis;
    updatePet(pet);
    checkAchievements(pet);
    savePetData(pet);
    playStateMelody(pet);

    // Phase 5: Update stats & battery
    statsTick(g_stats, PET_UPDATE_INTERVAL);
    updateBatteryLevel(pet);
    handlePowerManager(pet, currentMillis);

#ifdef ENABLE_OLED
    updateOLED(pet);
#endif

    // Phase 6: Update RGB LED status
    updateRGBLED(pet);
  }
}

// ============================================================
// Phase 7.5: IR Remote Command Handler
// ============================================================
#ifndef DISABLE_IR_REMOTE
void handleIRCommand(IRButton button, uint32_t rawCode) {
  Serial.printf("[IR] Command received: 0x%02X\n", button);

  switch (button) {
    case IR_BTN_CH_MINUS:  // Feed
      if (pet.isAlive) {
        feedPet(pet);
        savePetData(pet);
      }
      break;

    case IR_BTN_CH:  // Play
      if (pet.isAlive && pet.energy >= PLAY_ENERGY_MIN) {
        pet.state = "playing";
        pet.happiness = min(STAT_MAX, pet.happiness + PLAY_HAPPINESS_GAIN);
        pet.energy = max(STAT_MIN, pet.energy - PLAY_ENERGY_COST);
        pet.hunger = max(STAT_MIN, pet.hunger - PLAY_HUNGER_COST);
        pet.gameCooldown = 3;
        savePetData(pet);
      }
      break;

    case IR_BTN_CH_PLUS:  // Clean
      if (pet.isAlive) {
        cleanPet(pet);
        savePetData(pet);
      }
      break;

    case IR_BTN_PREV:  // Sleep
      if (pet.isAlive && pet.state != "sleeping") {
        pet.state = "sleeping";
        savePetData(pet);
      }
      break;

    case IR_BTN_NEXT:  // Wake
      if (pet.isAlive && pet.state == "sleeping") {
        pet.state = "normal";
        savePetData(pet);
      }
      break;

    case IR_BTN_PLAY:  // Toggle sound
      pet.soundEnabled = !pet.soundEnabled;
      savePetData(pet);
      Serial.printf("[IR] Sound: %s\n", pet.soundEnabled ? "ON" : "OFF");
      break;

    case IR_BTN_0:  // Reset pet
      if (!pet.isAlive) {
        // Revive
        pet.isAlive = true;
        pet.state = "normal";
        pet.health = 50;
        pet.hunger = 50;
        pet.happiness = 50;
        pet.energy = 50;
        pet.cleanliness = 50;
        pet.isDying = false;
        savePetData(pet);
      }
      break;

    case IR_BTN_1:
    case IR_BTN_2:
    case IR_BTN_3:  // Switch pet slot (multi-pet)
      {
        int slot = (int)(button - IR_BTN_1); // 0, 1, or 2
        if (switchPet(g_multiPet, slot)) {
          loadPetData(pet);
          Serial.printf("[IR] Switched to pet slot %d\n", slot);
        }
      }
      break;

    default:
      break;
  }
}
#endif // !DISABLE_IR_REMOTE
