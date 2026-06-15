#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "config.h"
#include "Pet.h"
#include "Storage.h"
#include "WebHandlers.h"

// OLED display (optional - enable with -DENABLE_OLED)
#ifdef ENABLE_OLED
#include "OLED.h"
#endif

// Web server
WebServer server(WEB_SERVER_PORT);

// Pet instance
Pet pet;

// Wake-up message tracking
bool showWakeMessage = false;
unsigned long wakeMessageStartTime = 0;
String previousState = "";

// Timing
unsigned long lastUpdateTime = 0;

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  // Load pet data from SPIFFS
  loadPetData(pet);
  previousState = pet.state;

  // Register web server routes
  setupWebServer(server, pet, showWakeMessage, wakeMessageStartTime, previousState);

  // Start server
  server.begin();

  // Initialize OLED
#ifdef ENABLE_OLED
  setupOLED();
#endif
}

void loop() {
  server.handleClient();

  // Update pet stats every interval
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= PET_UPDATE_INTERVAL) {
    lastUpdateTime = currentMillis;
    updatePet(pet);
    savePetData(pet);

#ifdef ENABLE_OLED
    updateOLED(pet);
#endif
  }
}
