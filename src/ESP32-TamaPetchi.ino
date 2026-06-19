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
#include "AppState.h"
#include "WebSocket.h"
#include "i18n.h"

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

// Phase 6: Watchdog configuration
#define WDT_TIMEOUT 10  // 10 seconds without feed → reset

// Timing
unsigned long lastUpdateTime = 0;

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
  loadStats(APP_STATE.stats);

  // Load pet data from SPIFFS
  loadPetData(APP_STATE.pet);
  loadAchievements(APP_STATE.pet);

  // Phase 6: Check if we woke from deep sleep and restore state
  if (wasDeepSleepWake()) {
    Serial.println("Woke from deep sleep!");
    restoreFromRTC(APP_STATE.pet);
  }

  AppState::getInstance().previousState = APP_STATE.pet.state;

  // Register web server routes
  registerHandlers(APP_STATE.server, APP_STATE.pet);

  // Start server
  APP_STATE.server.begin();

  // Phase 10.2: Start WebSocket server
  webSocketBegin(WS_PORT);

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

  APP_STATE.server.handleClient();

  // Phase 10.2: Handle WebSocket events
  webSocketLoop();

  // Phase 5: Handle OTA
  handleOTA();

  // Phase 7.5: Handle MQTT
#ifndef DISABLE_MQTT
  handleMQTT();
#endif

  // Phase 6: Check physical buttons
  checkButtons(APP_STATE.pet);

  // Check IR remote
#ifndef DISABLE_IR_REMOTE
  checkIRRemote();
#endif

  // Phase 10.2: WebSocket periodic broadcast + rate limit cleanup
  handleWebSocketBroadcast();

  // Update pet stats every interval
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= PET_UPDATE_INTERVAL) {
    lastUpdateTime = currentMillis;
    updatePet(APP_STATE.pet);
    checkAchievements(APP_STATE.pet);
    savePetData(APP_STATE.pet);
    playStateMelody(APP_STATE.pet);

    // Phase 5: Update stats & battery
    statsTick(APP_STATE.stats, PET_UPDATE_INTERVAL);
    updateBatteryLevel(APP_STATE.pet);
    handlePowerManager(APP_STATE.pet, currentMillis);

#ifdef ENABLE_OLED
    updateOLED(APP_STATE.pet);
#endif

    // Phase 6: Update RGB LED status
    updateRGBLED(APP_STATE.pet);
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
      if (APP_STATE.pet.isAlive) {
        feedPet(APP_STATE.pet);
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_CH:  // Play
      if (APP_STATE.pet.isAlive && APP_STATE.pet.energy >= PLAY_ENERGY_MIN) {
        APP_STATE.pet.state = "playing";
        APP_STATE.pet.happiness = min(STAT_MAX, APP_STATE.pet.happiness + PLAY_HAPPINESS_GAIN);
        APP_STATE.pet.energy = max(STAT_MIN, APP_STATE.pet.energy - PLAY_ENERGY_COST);
        APP_STATE.pet.hunger = max(STAT_MIN, APP_STATE.pet.hunger - PLAY_HUNGER_COST);
        APP_STATE.pet.gameCooldown = 3;
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_CH_PLUS:  // Clean
      if (APP_STATE.pet.isAlive) {
        cleanPet(APP_STATE.pet);
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_PREV:  // Sleep
      if (APP_STATE.pet.isAlive && APP_STATE.pet.state != "sleeping") {
        APP_STATE.pet.state = "sleeping";
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_NEXT:  // Wake
      if (APP_STATE.pet.isAlive && APP_STATE.pet.state == "sleeping") {
        APP_STATE.pet.state = "normal";
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_PLAY:  // Toggle sound
      APP_STATE.pet.soundEnabled = !APP_STATE.pet.soundEnabled;
      savePetData(APP_STATE.pet);
      Serial.printf("[IR] Sound: %s\n", APP_STATE.pet.soundEnabled ? "ON" : "OFF");
      break;

    case IR_BTN_0:  // Reset pet
      if (!APP_STATE.pet.isAlive) {
        // Revive
        APP_STATE.pet.isAlive = true;
        APP_STATE.pet.state = "normal";
        APP_STATE.pet.health = 50;
        APP_STATE.pet.hunger = 50;
        APP_STATE.pet.happiness = 50;
        APP_STATE.pet.energy = 50;
        APP_STATE.pet.cleanliness = 50;
        APP_STATE.pet.isDying = false;
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_1:
    case IR_BTN_2:
    case IR_BTN_3:  // Switch pet slot (multi-pet)
      {
        int slot = (int)(button - IR_BTN_1); // 0, 1, or 2
        if (switchPet(APP_STATE.multiPet, slot)) {
          loadPetData(APP_STATE.pet);
          Serial.printf("[IR] Switched to pet slot %d\n", slot);
        }
      }
      break;

    default:
      break;
  }
}
#endif // !DISABLE_IR_REMOTE
