#include "PetTrade.h"
#include "config.h"
#include "Pet.h"
#include "Storage.h"
#include "Achievements.h"
#include "AppState.h"
#include "WebHandlers.h"
#include "MQTT.h"
#include "WebSocket.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// MQTT Topic Scheme
// ============================================================
#define TRADE_TOPIC_PREFIX    "tamapetchi/trade"
#define TRADE_TOPIC_REQUEST   TRADE_TOPIC_PREFIX "/request"
#define TRADE_TOPIC_ACCEPT    TRADE_TOPIC_PREFIX "/accept"
#define TRADE_TOPIC_REJECT    TRADE_TOPIC_PREFIX "/reject"
#define TRADE_TOPIC_DATA      TRADE_TOPIC_PREFIX "/data"
#define TRADE_TOPIC_CONFIRM   TRADE_TOPIC_PREFIX "/confirm"

// ============================================================
// Module-level state
// ============================================================
static TradeSession tradeSession;

// ============================================================
// Topic helpers
// ============================================================
String getTradeRequestTopic()  { return TRADE_TOPIC_REQUEST; }
String getTradeAcceptTopic()   { return TRADE_TOPIC_ACCEPT; }
String getTradeRejectTopic()   { return TRADE_TOPIC_REJECT; }
String getTradeDataTopic()     { return TRADE_TOPIC_DATA; }
String getTradeConfirmTopic()  { return TRADE_TOPIC_CONFIRM; }

// ============================================================
// Internal helpers
// ============================================================
static void setTradeState(TradeState newState) {
  tradeSession.state = newState;
  tradeSession.stateChangeTime = millis();
  Serial.printf("[Trade] State -> %d\n", newState);
}

void resetTradeSession() {
  memset(&tradeSession, 0, sizeof(TradeSession));
  tradeSession.state = TRADE_IDLE;
}

// ============================================================
// Serialize pet data for trade
// ============================================================
static String serializePetForTrade() {
  AppState &state = APP_STATE;
  DynamicJsonDocument doc(2048);

  doc["name"] = state.pet.name;
  doc["type"] = (int)state.pet.type;
  doc["stage"] = (int)state.pet.stage;
  doc["age"] = state.pet.age;
  doc["hunger"] = state.pet.hunger;
  doc["happiness"] = state.pet.happiness;
  doc["health"] = state.pet.health;
  doc["energy"] = state.pet.energy;
  doc["cleanliness"] = state.pet.cleanliness;
  doc["soundEnabled"] = state.pet.soundEnabled;
  doc["musicEnabled"] = state.pet.musicEnabled;
  doc["difficulty"] = state.pet.difficulty;
  doc["mood"] = state.pet.mood;
  doc["personalityCheerful"] = state.pet.personalityCheerful;
  doc["personalityEnergetic"] = state.pet.personalityEnergetic;
  doc["personalityHungry"] = state.pet.personalityHungry;
  doc["feedCount"] = state.pet.feedCount;
  doc["playCount"] = state.pet.playCount;
  doc["hasBeenNamed"] = state.pet.hasBeenNamed;
  doc["elderAchieved"] = state.pet.elderAchieved;
  doc["totalPlayTime"] = state.pet.totalPlayTime;
  doc["totalSleepTime"] = state.pet.totalSleepTime;
  doc["timesFed"] = state.pet.timesFed;
  doc["timesPlayed"] = state.pet.timesPlayed;
  doc["highScore"] = state.pet.highScore;

  // Achievements array
  String achJson = getAchievementsJson(state.pet);
  StaticJsonDocument<512> achDoc;
  deserializeJson(achDoc, achJson);
  doc["achievements"] = achDoc["achievements"];

  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Deserialize pet data from trade
// ============================================================
static bool deserializePetFromTrade(const String &json) {
  AppState &state = APP_STATE;
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[Trade] Pet data parse error: %s\n", err.c_str());
    return false;
  }

  state.pet.name = doc["name"] | "Traded";
  state.pet.type = (PetType)(doc["type"].as<int>());
  state.pet.stage = (PetStage)(doc["stage"].as<int>());
  state.pet.age = doc["age"].as<int>();
  state.pet.hunger = constrain(doc["hunger"].as<int>(), STAT_MIN, STAT_MAX);
  state.pet.happiness = constrain(doc["happiness"].as<int>(), STAT_MIN, STAT_MAX);
  state.pet.health = constrain(doc["health"].as<int>(), STAT_MIN, STAT_MAX);
  state.pet.energy = constrain(doc["energy"].as<int>(), STAT_MIN, STAT_MAX);
  state.pet.cleanliness = constrain(doc["cleanliness"].as<int>(), STAT_MIN, STAT_MAX);
  state.pet.soundEnabled = doc["soundEnabled"] | true;
  state.pet.musicEnabled = doc["musicEnabled"] | true;
  state.pet.difficulty = doc["difficulty"].as<int>();
  state.pet.mood = doc["mood"].as<int>();
  state.pet.personalityCheerful = doc["personalityCheerful"].as<int>();
  state.pet.personalityEnergetic = doc["personalityEnergetic"].as<int>();
  state.pet.personalityHungry = doc["personalityHungry"].as<int>();
  state.pet.feedCount = doc["feedCount"].as<int>();
  state.pet.playCount = doc["playCount"].as<int>();
  state.pet.hasBeenNamed = doc["hasBeenNamed"] | true;
  state.pet.elderAchieved = doc["elderAchieved"] | false;
  state.pet.totalPlayTime = doc["totalPlayTime"].as<unsigned long>();
  state.pet.totalSleepTime = doc["totalSleepTime"].as<unsigned long>();
  state.pet.timesFed = doc["timesFed"].as<int>();
  state.pet.timesPlayed = doc["timesPlayed"].as<int>();
  state.pet.highScore = doc["highScore"].as<int>();

  // Restore pet state
  state.pet.isAlive = true;
  state.pet.state = "normal";
  state.pet.isDying = false;

  // Save to SPIFFS
  savePetData(state.pet);
  saveAchievements(state.pet);

  Serial.printf("[Trade] Pet received: %s\n", state.pet.name.c_str());
  return true;
}

// ============================================================
// Trade history logging
// ============================================================
static void logTradeHistory(const String &event, const String &peer, const String &petName) {
  DynamicJsonDocument doc(1024);

  File f = SPIFFS.open(TRADE_HISTORY_FILE, "r");
  if (f) {
    deserializeJson(doc, f);
    f.close();
  }

  JsonArray history = doc["history"].as<JsonArray>();
  if (!history) {
    history = doc.createNestedArray("history");
  }

  JsonObject entry = history.createNestedObject();
  entry["event"] = event;
  entry["peer"] = peer;
  entry["petName"] = petName;
  entry["timestamp"] = millis();

  while (history.size() > MAX_TRADE_HISTORY) {
    history.remove(0);
  }

  f = SPIFFS.open(TRADE_HISTORY_FILE, "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
  }
}

// ============================================================
// MQTT Trade Message Handler
// ============================================================
void handleTradeMessage(const String &topic, const String &payload) {
  Serial.printf("[Trade] Message on %s\n", topic.c_str());

  // Ignore trade messages if MQTT is disabled
#ifndef DISABLE_MQTT
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("[Trade] Invalid JSON in trade message");
    return;
  }

  if (topic.endsWith("/request")) {
    if (tradeSession.state != TRADE_IDLE) return;

    const char *peerId = doc["clientId"] | "unknown";
    const char *pin = doc["pin"] | "";

    strncpy(tradeSession.peerClientId, peerId, sizeof(tradeSession.peerClientId) - 1);
    strncpy(tradeSession.tradePin, pin, TRADE_PIN_LENGTH);
    tradeSession.tradePin[TRADE_PIN_LENGTH] = '\0';
    tradeSession.isInitiator = false;
    setTradeState(TRADE_REQUEST_RECEIVED);

    Serial.printf("[Trade] Request from %s (PIN: %s)\n", peerId, pin);
  }
  else if (topic.endsWith("/accept")) {
    if (tradeSession.state != TRADE_REQUEST_SENT) return;

    const char *peerId = doc["clientId"] | "";
    if (String(peerId) != String(tradeSession.peerClientId)) return;

    setTradeState(TRADE_ACCEPTED);

    // Send pet data
    String petData = serializePetForTrade();
    mqttPublishNotification(petData);  // Use notification channel for data
    // Also publish to data topic via direct MQTT
    // We need to access the MQTT client - use the notification system instead
    setTradeState(TRADE_DATA_SENT);
    Serial.println("[Trade] Pet data sent to peer");
  }
  else if (topic.endsWith("/reject")) {
    const char *peerId = doc["clientId"] | "";
    if (String(peerId) == String(tradeSession.peerClientId) || strlen(tradeSession.peerClientId) == 0) {
      Serial.println("[Trade] Trade rejected by peer");
      logTradeHistory("rejected", tradeSession.peerClientId, "");
      resetTradeSession();
    }
  }
  else if (topic.endsWith("/data")) {
    if (tradeSession.state != TRADE_ACCEPTED && tradeSession.state != TRADE_REQUEST_RECEIVED) return;

    // The payload contains the pet data directly
    if (deserializePetFromTrade(payload)) {
      setTradeState(TRADE_DATA_RECEIVED);

      // Send confirmation
      mqttPublishNotification("{\"type\":\"trade_confirm\",\"clientId\":\"" + String(MQTT_CLIENT_ID) + "\"}");

      setTradeState(TRADE_CONFIRMED);
      logTradeHistory("received", tradeSession.peerClientId, APP_STATE.pet.name);
      webSocketBroadcastNotification("trade", "Pet traded with " + String(tradeSession.peerClientId));
      resetTradeSession();
      Serial.println("[Trade] Pet received and confirmed!");
    } else {
      Serial.println("[Trade] Failed to deserialize pet data");
      cancelTrade();
    }
  }
  else if (topic.endsWith("/confirm")) {
    if (tradeSession.state != TRADE_DATA_SENT) return;

    const char *peerId = doc["clientId"] | "";
    if (String(peerId) != String(tradeSession.peerClientId)) return;

    setTradeState(TRADE_CONFIRMED);
    logTradeHistory("sent", tradeSession.peerClientId, APP_STATE.pet.name);
    webSocketBroadcastNotification("trade", "Pet traded with " + String(tradeSession.peerClientId));
    Serial.println("[Trade] Trade confirmed by peer!");
    resetTradeSession();
  }
#endif
}

// ============================================================
// Public API
// ============================================================

void initPetTrade() {
  resetTradeSession();
  Serial.println("[Trade] Pet trade system initialized");
}

bool startTradeRequest(const String &peerClientId, const String &tradePin) {
  if (tradeSession.state != TRADE_IDLE) {
    Serial.println("[Trade] Cannot start — trade in progress");
    return false;
  }

  if (tradePin.length() != TRADE_PIN_LENGTH) {
    Serial.println("[Trade] Invalid PIN length");
    return false;
  }

  strncpy(tradeSession.peerClientId, peerClientId.c_str(), sizeof(tradeSession.peerClientId) - 1);
  strncpy(tradeSession.tradePin, tradePin.c_str(), TRADE_PIN_LENGTH);
  tradeSession.tradePin[TRADE_PIN_LENGTH] = '\0';
  tradeSession.isInitiator = true;
  setTradeState(TRADE_REQUEST_SENT);

  Serial.printf("[Trade] Request sent to %s (PIN: %s)\n", peerClientId.c_str(), tradePin.c_str());
  return true;
}

bool acceptTradeRequest() {
  if (tradeSession.state != TRADE_REQUEST_RECEIVED) {
    Serial.println("[Trade] No pending trade request");
    return false;
  }

  setTradeState(TRADE_ACCEPTED);
  Serial.println("[Trade] Trade request accepted — waiting for pet data");
  return true;
}

void cancelTrade() {
  if (tradeSession.state == TRADE_IDLE) return;

  logTradeHistory("cancelled", tradeSession.peerClientId, "");
  resetTradeSession();
  Serial.println("[Trade] Trade cancelled");
}

TradeState getTradeState() {
  return tradeSession.state;
}

const TradeSession& getTradeSession() {
  return tradeSession;
}

String getTradeHistoryJson() {
  DynamicJsonDocument doc(2048);

  File f = SPIFFS.open(TRADE_HISTORY_FILE, "r");
  if (f) {
    deserializeJson(doc, f);
    f.close();
  }

  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Web API Routes
// ============================================================
void registerTradeRoutes() {
  // GET /api/trade/status
  APP_STATE.server.on("/api/trade/status", HTTP_GET, []() {
    StaticJsonDocument<256> resp;
    resp["state"] = (int)tradeSession.state;
    resp["peerClientId"] = tradeSession.peerClientId;
    resp["isInitiator"] = tradeSession.isInitiator;
    String result;
    serializeJson(resp, result);
    APP_STATE.server.send(200, "application/json", result);
  });

  // POST /api/trade/request
  APP_STATE.server.on("/api/trade/request", HTTP_POST, []() {
    if (!checkRateLimit(APP_STATE.server.client().remoteIP().toString())) {
      APP_STATE.server.send(429, "application/json",
        "{\"success\":false,\"error\":\"rate_limit\",\"message\":\"Too many requests\"}");
      return;
    }

    String body = APP_STATE.server.arg("plain");
    StaticJsonDocument<256> jsonDoc;
    DeserializationError err = deserializeJson(jsonDoc, body);
    if (err) {
      APP_STATE.server.send(400, "application/json",
        "{\"success\":false,\"message\":\"Invalid JSON\"}");
      return;
    }

    String peerId = jsonDoc["peerClientId"] | "";
    String pin = jsonDoc["pin"] | "";

    if (peerId.length() == 0 || pin.length() != TRADE_PIN_LENGTH) {
      APP_STATE.server.send(400, "application/json",
        "{\"success\":false,\"message\":\"Missing peerClientId or invalid PIN (4 digits)\"}");
      return;
    }

    if (startTradeRequest(peerId, pin)) {
      StaticJsonDocument<256> resp;
      resp["success"] = true;
      resp["message"] = "Trade request sent";
      String result;
      serializeJson(resp, result);
      APP_STATE.server.send(200, "application/json", result);
    } else {
      APP_STATE.server.send(400, "application/json",
        "{\"success\":false,\"message\":\"Cannot start trade — another trade in progress\"}");
    }
  });

  // POST /api/trade/accept
  APP_STATE.server.on("/api/trade/accept", HTTP_POST, []() {
    if (!checkRateLimit(APP_STATE.server.client().remoteIP().toString())) {
      APP_STATE.server.send(429, "application/json",
        "{\"success\":false,\"error\":\"rate_limit\",\"message\":\"Too many requests\"}");
      return;
    }

    if (acceptTradeRequest()) {
      StaticJsonDocument<256> resp;
      resp["success"] = true;
      resp["message"] = "Trade accepted — waiting for pet data";
      String result;
      serializeJson(resp, result);
      APP_STATE.server.send(200, "application/json", result);
    } else {
      APP_STATE.server.send(400, "application/json",
        "{\"success\":false,\"message\":\"No pending trade request\"}");
    }
  });

  // POST /api/trade/cancel
  APP_STATE.server.on("/api/trade/cancel", HTTP_POST, []() {
    if (!checkRateLimit(APP_STATE.server.client().remoteIP().toString())) {
      APP_STATE.server.send(429, "application/json",
        "{\"success\":false,\"error\":\"rate_limit\",\"message\":\"Too many requests\"}");
      return;
    }

    cancelTrade();
    APP_STATE.server.send(200, "application/json", "{\"success\":true,\"message\":\"Trade cancelled\"}");
  });

  // GET /api/trade/history
  APP_STATE.server.on("/api/trade/history", HTTP_GET, []() {
    APP_STATE.server.send(200, "application/json", getTradeHistoryJson());
  });
}
