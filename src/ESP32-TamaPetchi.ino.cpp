# 1 "/tmp/tmpv07one9z"
#include <Arduino.h>
# 1 "/root/.hermes/profiles/kael/ESP32-TamaPetchi/src/ESP32-TamaPetchi.ino"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Esp.h>


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
#include "OTARollback.h"
#include "SoundPack.h"


#ifdef ENABLE_OLED
#include "OLED.h"
#endif


#ifndef DISABLE_IR_REMOTE
#include "IRRemote.h"
#endif


#ifndef DISABLE_MQTT
#include "MQTT.h"
#endif


#ifndef DISABLE_OTA_DELTA
#include "OTA_Delta.h"
#endif


#define WDT_TIMEOUT 10


unsigned long lastUpdateTime = 0;
void setup();
void loop();
void handleIRCommand(IRButton button, uint32_t rawCode);
#line 56 "/root/.hermes/profiles/kael/ESP32-TamaPetchi/src/ESP32-TamaPetchi.ino"
void setup() {
  Serial.begin(SERIAL_BAUD);


  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);


  randomSeed(analogRead(0));


  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }


  setupPowerManager();


  setupWiFi();


  setupOTA();


  initRollbackSystem();


  initSoundPack();


#ifndef DISABLE_MQTT
  setupMQTT();
#endif


  loadStats(APP_STATE.stats);


  loadPetData(APP_STATE.pet);
  loadAchievements(APP_STATE.pet);


  if (wasDeepSleepWake()) {
    Serial.println("Woke from deep sleep!");
    restoreFromRTC(APP_STATE.pet);
  }

  AppState::getInstance().previousState = APP_STATE.pet.state;


  registerHandlers(APP_STATE.server, APP_STATE.pet);


  APP_STATE.server.begin();


  webSocketBegin(WS_PORT);


#ifdef ENABLE_OLED
  setupOLED();
#endif


  setupButtons();
  if (isFactoryResetPressed()) {
    Serial.println("FACTORY RESET triggered via BOOT button!");


#ifndef DISABLE_RGB_LED
    flashRGBRed(3, 300, 200);
#endif

#ifdef ENABLE_OLED
    setupOLED();
    showFactoryResetOLED();
#endif

    delay(500);


    SPIFFS.format();
    Serial.println("SPIFFS formatted.");


    WiFi.disconnect(true, true);
    Serial.println("WiFi credentials cleared.");

    delay(1000);
    Serial.println("Restarting...");
    ESP.restart();

  }


  setupRGBLED();


#ifndef DISABLE_IR_REMOTE
  setupIRRemote();
  setIRCommandCallback(handleIRCommand);
#endif


  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  Serial.println("TamaPetchi initialized — WDT active");
}

void loop() {

  esp_task_wdt_reset();

  APP_STATE.server.handleClient();


  webSocketLoop();


  handleOTA();


#ifndef DISABLE_MQTT
  handleMQTT();
#endif


  checkButtons(APP_STATE.pet);


#ifndef DISABLE_IR_REMOTE
  checkIRRemote();
#endif


  handleWebSocketBroadcast();


  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= PET_UPDATE_INTERVAL) {
    lastUpdateTime = currentMillis;
    updatePet(APP_STATE.pet);
    checkAchievements(APP_STATE.pet);
    savePetData(APP_STATE.pet);
    playStateMelody(APP_STATE.pet);


    statsTick(APP_STATE.stats, PET_UPDATE_INTERVAL);
    updateBatteryLevel(APP_STATE.pet);
    handlePowerManager(APP_STATE.pet, currentMillis);

#ifdef ENABLE_OLED
    updateOLED(APP_STATE.pet);
#endif


    updateRGBLED(APP_STATE.pet);
  }


  if (!isFirmwareConfirmed() && (millis() - getStableStartTime() >= 300000UL)) {
    confirmFirmwareStable();
  }
}




#ifndef DISABLE_IR_REMOTE
void handleIRCommand(IRButton button, uint32_t rawCode) {
  Serial.printf("[IR] Command received: 0x%02X\n", button);

  switch (button) {
    case IR_BTN_CH_MINUS:
      if (APP_STATE.pet.isAlive) {
        feedPet(APP_STATE.pet);
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_CH:
      if (APP_STATE.pet.isAlive && APP_STATE.pet.energy >= PLAY_ENERGY_MIN) {
        APP_STATE.pet.state = "playing";
        APP_STATE.pet.happiness = min(STAT_MAX, APP_STATE.pet.happiness + PLAY_HAPPINESS_GAIN);
        APP_STATE.pet.energy = max(STAT_MIN, APP_STATE.pet.energy - PLAY_ENERGY_COST);
        APP_STATE.pet.hunger = max(STAT_MIN, APP_STATE.pet.hunger - PLAY_HUNGER_COST);
        APP_STATE.pet.gameCooldown = 3;
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_CH_PLUS:
      if (APP_STATE.pet.isAlive) {
        cleanPet(APP_STATE.pet);
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_PREV:
      if (APP_STATE.pet.isAlive && APP_STATE.pet.state != "sleeping") {
        APP_STATE.pet.state = "sleeping";
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_NEXT:
      if (APP_STATE.pet.isAlive && APP_STATE.pet.state == "sleeping") {
        APP_STATE.pet.state = "normal";
        savePetData(APP_STATE.pet);
      }
      break;

    case IR_BTN_PLAY:
      APP_STATE.pet.soundEnabled = !APP_STATE.pet.soundEnabled;
      savePetData(APP_STATE.pet);
      Serial.printf("[IR] Sound: %s\n", APP_STATE.pet.soundEnabled ? "ON" : "OFF");
      break;

    case IR_BTN_0:
      if (!APP_STATE.pet.isAlive) {

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
    case IR_BTN_3:
      {
        int slot = (int)(button - IR_BTN_1);
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
#endif