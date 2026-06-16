#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "config.h"
#include "Pet.h"
#include "Storage.h"
#include "WebHandlers.h"
#include "Achievements.h"
#include "MultiPet.h"
#include "Notifications.h"
#include "PowerManager.h"
#include "WiFiManager.h"
#include "OTA.h"
#include "Stats.h"

// OLED display (optional - enable with -DENABLE_OLED)
#ifdef ENABLE_OLED
#include "OLED.h"
#endif

// Web server
WebServer server(WEB_SERVER_PORT);

// Multi-pet state
MultiPetState multiPet;

// Statistics
GameStats gameStats;

// Timing
unsigned long lastUpdateTime = 0;

// Helper to convert stage to string
static const char* stageStr(PetStage stage) {
  switch (stage) {
    case BABY:  return "baby";
    case CHILD: return "child";
    case ADULT: return "adult";
    case ELDER: return "elder";
    default:    return "baby";
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  // Initialize random seed
  randomSeed(analogRead(0));

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }

  // Initialize power manager
  setupPowerManager();

  // Initialize multi-pet system
  loadMultiPetState(multiPet);

  // Ensure at least one pet exists
  if (multiPet.petCount == 0) {
    createPet(multiPet, "Tama", BLOB);
  }

  // Connect to WiFi
  setupWiFi();

  // Load statistics
  loadStats(gameStats);

  // Load notifications for active pet
  loadNotifications(multiPet.activePetIndex);

  // Register web server routes
  registerHandlers(server, multiPet, gameStats);

  // Register WiFi routes (for WiFi status, reset, connect)
  registerWiFiRoutes(server);

  // Start server
  server.begin();

  // Initialize OTA
  setupOTA();

  // Initialize OLED
#ifdef ENABLE_OLED
  setupOLED();
#endif

  Serial.println("TamaPetchi ready!");
  Serial.print("IP: ");
  Serial.println(getIPAddress());
}

void loop() {
  server.handleClient();

  // Handle OTA
  handleOTA();

  // Update pet stats every interval
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= PET_UPDATE_INTERVAL) {
    lastUpdateTime = currentMillis;

    Pet* pet = getActivePet(multiPet);
    if (!pet) return;

    // Track previous stage for evolution detection
    PetStage prevStage = pet->stage;

    updatePet(*pet);
    checkAchievements(*pet);
    savePetData(*pet);
    saveMultiPetState(multiPet);

    // Update statistics
    statsTick(gameStats, PET_UPDATE_INTERVAL);

    // Check for evolution notification
    if (pet->stage != prevStage) {
      statsOnEvolution(gameStats);
      addNotification(multiPet.activePetIndex, NOTIF_EVOLUTION_READY,
        "Your pet evolved to " + String(stageStr(pet->stage)) + "!");
    }

    // Check for death notification
    if (!pet->isAlive) {
      statsOnDeath(gameStats);
      addNotification(multiPet.activePetIndex, NOTIF_DEATH, "Your pet has died...");
    }

    // Check for low health notification
    if (pet->isAlive && pet->health < CRITICAL_HEALTH_MIN) {
      addNotification(multiPet.activePetIndex, NOTIF_LOW_HEALTH,
        "Your pet's health is critical!");
    }

    // Low battery check
    if (isLowBattery()) {
      addNotification(multiPet.activePetIndex, NOTIF_LOW_BATTERY,
        "Battery is low!");
    }

    // Save stats periodically
    saveStats(gameStats);

#ifdef ENABLE_OLED
    updateOLED(*pet);
#endif
  }

  // Handle power management
  Pet* pet = getActivePet(multiPet);
  if (pet) {
    handlePowerManager(*pet, currentMillis);
  }
}
