#include "WebSocket.h"
#include "AppState.h"
#include "Pet.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// Forward declaration — defined in WebHandlers.cpp
extern void cleanupRateBuckets();

// ============================================================
// Module-level state
// ============================================================
static WebSocketsServer webSocket = WebSocketsServer(WS_PORT);
static unsigned long lastBroadcast = 0;
// Phase 11.5: Broadcast only on state change, with 30s keepalive

// ============================================================
// WebSocket Event Handler
// ============================================================
static void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client #%u disconnected\n", num);
      break;

    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] Client #%u connected from %s\n", num, ip.toString().c_str());
      // Send initial state immediately on connect
      AppState &state = APP_STATE;
      StaticJsonDocument<512> jsonDoc;
      jsonDoc["type"] = "init";
      jsonDoc["hunger"] = state.pet.hunger;
      jsonDoc["happiness"] = state.pet.happiness;
      jsonDoc["health"] = state.pet.health;
      jsonDoc["energy"] = state.pet.energy;
      jsonDoc["cleanliness"] = state.pet.cleanliness;
      jsonDoc["isAlive"] = state.pet.isAlive;
      jsonDoc["state"] = state.pet.state;
      jsonDoc["age"] = state.pet.age;

      const char *stageStr = "baby";
      switch (state.pet.stage) {
        case BABY:  stageStr = "baby";  break;
        case CHILD: stageStr = "child"; break;
        case ADULT: stageStr = "adult"; break;
        case ELDER: stageStr = "elder"; break;
      }
      jsonDoc["stage"] = stageStr;
      jsonDoc["isNight"] = state.pet.isNight;
      jsonDoc["weather"] = getWeatherName(state.pet.weather);
      jsonDoc["isDying"] = state.pet.isDying;
      jsonDoc["isEvolving"] = state.pet.isEvolving;
      jsonDoc["mood"] = state.pet.mood;
      jsonDoc["moodName"] = getMoodName(state.pet.mood);
      jsonDoc["moodEmoji"] = getMoodEmoji(state.pet.mood);

      String msg;
      serializeJson(jsonDoc, msg);
      webSocket.sendTXT(num, msg);
      break;
    }

    case WStype_TEXT: {
      // Handle incoming messages from client
      String message = String((char *)payload);
      Serial.printf("[WS] Received from #%u: %s\n", num, message.c_str());

      // Parse incoming JSON
      StaticJsonDocument<256> jsonDoc;
      DeserializationError error = deserializeJson(jsonDoc, message);
      if (error) {
        // Not valid JSON — ignore
        break;
      }

      // Handle ping/pong for heartbeat
      const char *msgType = jsonDoc["type"] | "";
      if (strcmp(msgType, "ping") == 0) {
        // Respond with pong
        webSocket.sendTXT(num, "{\"type\":\"pong\"}");
      }
      break;
    }

    case WStype_BIN:
      // Binary frames not used — ignore
      break;

    case WStype_ERROR:
      Serial.printf("[WS] Error on client #%u\n", num);
      break;

    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      // Fragmented frames — not used
      break;
  }
}

// ============================================================
// Public API
// ============================================================

void webSocketBegin(uint16_t port) {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.printf("[WS] WebSocket server started on port %d\n", port);
}

void webSocketLoop() {
  webSocket.loop();
}

void webSocketBroadcast(const String &data) {
  // Broadcast to all connected clients
  // Note: broadcastTXT takes non-const String& so we need a mutable copy
  String payload = data;
  webSocket.broadcastTXT(payload);
}

int webSocketConnectedClients() {
  return webSocket.connectedClients();
}

uint16_t webSocketPort() {
  return WS_PORT;
}

// ============================================================
// Periodic broadcast — call from loop() instead of handleSSEClients()
// Phase 11.5: Only broadcast on actual state change, not every interval
static int lastHunger = -1, lastHappiness = -1, lastHealth = -1;
static int lastEnergy = -1, lastCleanliness = -1, lastAge = -1;
static bool lastIsAlive = false;
static String lastState = "";
static PetStage lastStage = BABY;
static bool lastIsNight = false;
static int lastMood = -1;

void handleWebSocketBroadcast() {
  AppState &state = APP_STATE;
  if (!state.pet.isAlive) return;

  // Phase 11.5: Only broadcast if something actually changed
  bool changed = (state.pet.hunger != lastHunger ||
                  state.pet.happiness != lastHappiness ||
                  state.pet.health != lastHealth ||
                  state.pet.energy != lastEnergy ||
                  state.pet.cleanliness != lastCleanliness ||
                  state.pet.age != lastAge ||
                  state.pet.isAlive != lastIsAlive ||
                  state.pet.state != lastState ||
                  state.pet.stage != lastStage ||
                  state.pet.isNight != lastIsNight ||
                  state.pet.mood != lastMood);

  unsigned long now = millis();
  // Force broadcast every 30s even if nothing changed (keepalive)
  bool forceBroadcast = (now - lastBroadcast > 30000);

  if (!changed && !forceBroadcast) return;

  // Update last known values
  lastHunger = state.pet.hunger;
  lastHappiness = state.pet.happiness;
  lastHealth = state.pet.health;
  lastEnergy = state.pet.energy;
  lastCleanliness = state.pet.cleanliness;
  lastAge = state.pet.age;
  lastIsAlive = state.pet.isAlive;
  lastState = state.pet.state;
  lastStage = state.pet.stage;
  lastIsNight = state.pet.isNight;
  lastMood = state.pet.mood;
  lastBroadcast = now;

  // Phase 7.2: Periodic rate limit bucket cleanup
  cleanupRateBuckets();

  // Phase 11.5: Use compact StaticJsonDocument (smaller than DynamicJsonDocument)
  StaticJsonDocument<384> jsonDoc;
  jsonDoc["t"] = "u";  // Short key names for smaller payload
  jsonDoc["h"] = state.pet.hunger;
  jsonDoc["ha"] = state.pet.happiness;
  jsonDoc["he"] = state.pet.health;
  jsonDoc["e"] = state.pet.energy;
  jsonDoc["c"] = state.pet.cleanliness;
  jsonDoc["a"] = state.pet.age;

  const char *stageStr = "baby";
  switch (state.pet.stage) {
    case BABY:  stageStr = "baby";  break;
    case CHILD: stageStr = "child"; break;
    case ADULT: stageStr = "adult"; break;
    case ELDER: stageStr = "elder"; break;
  }
  jsonDoc["s"] = stageStr;
  jsonDoc["n"] = state.pet.isNight;
  jsonDoc["st"] = state.pet.state;
  jsonDoc["mo"] = state.pet.mood;

  String payload;
  serializeJson(jsonDoc, payload);
  webSocketBroadcast(payload);
}

// ============================================================
// Notification broadcast — called when events happen
// ============================================================
void webSocketBroadcastNotification(const String &notificationType, const String &message) {
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["type"] = "notification";
  jsonDoc["notificationType"] = notificationType;
  jsonDoc["message"] = message;
  jsonDoc["timestamp"] = millis();

  String payload;
  serializeJson(jsonDoc, payload);
  webSocketBroadcast(payload);
}
