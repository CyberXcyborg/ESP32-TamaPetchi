#include "MQTT.h"
#include "config.h"
#include "Pet.h"
#include "Storage.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Forward declarations from WebHandlers.cpp
extern bool checkRateLimit(const String &clientIP);

// Forward declarations within MQTT module
static void publishHADiscovery();

// ============================================================
// MQTT Configuration — all from config.h
// ============================================================
// MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASS, MQTT_CLIENT_ID
// defined in config.h with sensible defaults

// ============================================================
// Module-level state
// ============================================================
static WiFiClient mqttWiFiClient;
static PubSubClient mqttClient(mqttWiFiClient);

static unsigned long lastReconnectAttempt = 0;
static unsigned long lastStatePublish = 0;
static bool mqttEnabled = true;

// Reconnect interval starts at 5s, backs off to 30s
#define MQTT_RECONNECT_INITIAL  5000UL
#define MQTT_RECONNECT_MAX      30000UL
static unsigned long reconnectInterval = MQTT_RECONNECT_INITIAL;

// Publish state every 5 minutes
#define MQTT_STATE_INTERVAL     300000UL

// Topic prefixes
#define TOPIC_PREFIX            "tamapetchi"
#define TOPIC_STATE             TOPIC_PREFIX "/state"
#define TOPIC_COMMAND           TOPIC_PREFIX "/command"
#define TOPIC_NOTIFICATION      TOPIC_PREFIX "/notification"
#define TOPIC_AVAILABILITY      TOPIC_PREFIX "/status"
#define TOPIC_CONFIG            TOPIC_PREFIX "/config"

// Home Assistant auto-discovery prefix
#define HA_DISCOVERY_PREFIX     "homeassistant"

// ============================================================
// Internal helpers
// ============================================================
static bool mqttConnect() {
  if (!mqttEnabled) return false;
  if (mqttClient.connected()) return true;

  unsigned long now = millis();
  if (now - lastReconnectAttempt < reconnectInterval) return false;
  lastReconnectAttempt = now;

  Serial.print("[MQTT] Connecting to ");
  Serial.print(MQTT_BROKER);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(" ... ");

  bool result;
  if (strlen(MQTT_USER) > 0) {
    result = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS,
                                TOPIC_AVAILABILITY, 0, true, "offline");
  } else {
    result = mqttClient.connect(MQTT_CLIENT_ID,
                                TOPIC_AVAILABILITY, 0, true, "offline");
  }

  if (result) {
    Serial.println("connected!");
    reconnectInterval = MQTT_RECONNECT_INITIAL; // Reset backoff

    // Subscribe to command topic
    mqttClient.subscribe(TOPIC_COMMAND);

    // Publish online status
    mqttClient.publish(TOPIC_AVAILABILITY, "online", true);

    // Publish Home Assistant auto-discovery
    publishHADiscovery();

    // Publish current state immediately
    mqttPublishState();
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
    // Exponential backoff
    reconnectInterval = min(reconnectInterval * 2, MQTT_RECONNECT_MAX);
  }

  return result;
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to String
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  String cmd(message);

  Serial.printf("[MQTT] Received: %s -> %s\n", topic, cmd.c_str());

  // Only process command topic
  if (String(topic) != TOPIC_COMMAND) return;

  // Parse JSON command
  StaticJsonDocument<256> jsonDoc;
  DeserializationError err = deserializeJson(jsonDoc, cmd);
  if (err) {
    Serial.println("[MQTT] Invalid JSON command");
    return;
  }

  // We need access to the pet — use extern
  extern Pet pet;
  String action = jsonDoc["action"] | "";

  if (action == "feed" && pet.isAlive) {
    feedPet(pet);
    savePetData(pet);
    Serial.println("[MQTT] Command: feed");
  } else if (action == "play" && pet.isAlive && pet.energy >= PLAY_ENERGY_MIN) {
    playPet(pet);
    savePetData(pet);
    Serial.println("[MQTT] Command: play");
  } else if (action == "clean" && pet.isAlive) {
    cleanPet(pet);
    savePetData(pet);
    Serial.println("[MQTT] Command: clean");
  } else if (action == "sleep" && pet.isAlive && pet.state != "sleeping") {
    sleepPet(pet);
    savePetData(pet);
    Serial.println("[MQTT] Command: sleep");
  } else if (action == "heal" && pet.isAlive) {
    healPet(pet);
    savePetData(pet);
    Serial.println("[MQTT] Command: heal");
  } else if (action == "revive" && !pet.isAlive) {
    if (canRevive(pet)) {
      revivePet(pet);
      savePetData(pet);
      Serial.println("[MQTT] Command: revive");
    }
  } else if (action == "reset") {
    initPet(pet);
    savePetData(pet);
    Serial.println("[MQTT] Command: reset");
  } else if (action == "toggle_sound") {
    pet.soundEnabled = !pet.soundEnabled;
    savePetData(pet);
    Serial.println("[MQTT] Command: toggle_sound");
  } else {
    Serial.printf("[MQTT] Unknown or invalid action: %s\n", action.c_str());
    return;
  }

  // Publish updated state after command
  mqttPublishState();
}

// ============================================================
// Home Assistant Auto-Discovery
// ============================================================
static void publishHADiscovery() {
  // Pet state sensor
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi State";
    doc["unique_id"] = "tamapetchi_state";
    doc["state_topic"] = TOPIC_STATE;
    doc["value_template"] = "{{ value_json.state }}";
    doc["icon"] = "mdi:paw";
    doc["availability_topic"] = TOPIC_AVAILABILITY;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + MQTT_CLIENT_ID + "/state/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  // Hunger sensor
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi Hunger";
    doc["unique_id"] = "tamapetchi_hunger";
    doc["state_topic"] = TOPIC_STATE;
    doc["value_template"] = "{{ value_json.hunger }}";
    doc["unit_of_measurement"] = "%";
    doc["icon"] = "mdi:food";
    doc["availability_topic"] = TOPIC_AVAILABILITY;

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + MQTT_CLIENT_ID + "/hunger/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  // Happiness sensor
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi Happiness";
    doc["unique_id"] = "tamapetchi_happiness";
    doc["state_topic"] = TOPIC_STATE;
    doc["value_template"] = "{{ value_json.happiness }}";
    doc["unit_of_measurement"] = "%";
    doc["icon"] = "mdi:emoticon-happy";
    doc["availability_topic"] = TOPIC_AVAILABILITY;

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + MQTT_CLIENT_ID + "/happiness/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  // Health sensor
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi Health";
    doc["unique_id"] = "tamapetchi_health";
    doc["state_topic"] = TOPIC_STATE;
    doc["value_template"] = "{{ value_json.health }}";
    doc["unit_of_measurement"] = "%";
    doc["icon"] = "mdi:heart-pulse";
    doc["availability_topic"] = TOPIC_AVAILABILITY;

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + MQTT_CLIENT_ID + "/health/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  // Battery sensor
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi Battery";
    doc["unique_id"] = "tamapetchi_battery";
    doc["state_topic"] = TOPIC_STATE;
    doc["value_template"] = "{{ value_json.battery }}";
    doc["unit_of_measurement"] = "%";
    doc["icon"] = "mdi:battery";
    doc["availability_topic"] = TOPIC_AVAILABILITY;

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + MQTT_CLIENT_ID + "/battery/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  // Feed button
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi Feed";
    doc["unique_id"] = "tamapetchi_feed";
    doc["command_topic"] = TOPIC_COMMAND;
    doc["payload_press"] = "{\"action\":\"feed\"}";
    doc["icon"] = "mdi:food";
    doc["availability_topic"] = TOPIC_AVAILABILITY;

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/button/" + MQTT_CLIENT_ID + "/feed/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  // Play button
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi Play";
    doc["unique_id"] = "tamapetchi_play";
    doc["command_topic"] = TOPIC_COMMAND;
    doc["payload_press"] = "{\"action\":\"play\"}";
    doc["icon"] = "mdi:gamepad-variant";
    doc["availability_topic"] = TOPIC_AVAILABILITY;

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/button/" + MQTT_CLIENT_ID + "/play/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  // Clean button
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "TamaPetchi Clean";
    doc["unique_id"] = "tamapetchi_clean";
    doc["command_topic"] = TOPIC_COMMAND;
    doc["payload_press"] = "{\"action\":\"clean\"}";
    doc["icon"] = "mdi:shower-head";
    doc["availability_topic"] = TOPIC_AVAILABILITY;

    String payload;
    serializeJson(doc, payload);
    String topic = String(HA_DISCOVERY_PREFIX) + "/button/" + MQTT_CLIENT_ID + "/clean/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
  }

  Serial.println("[MQTT] HA auto-discovery published");
}

// ============================================================
// Public API
// ============================================================
void setupMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512); // Larger buffer for HA discovery messages
  Serial.println("[MQTT] Configured — will connect when WiFi is ready");
}

void handleMQTT() {
  if (!mqttEnabled) return;

  if (!mqttClient.connected()) {
    mqttConnect();
  } else {
    mqttClient.loop();

    // Periodic state publish
    unsigned long now = millis();
    if (now - lastStatePublish >= MQTT_STATE_INTERVAL) {
      lastStatePublish = now;
      mqttPublishState();
    }
  }
}

void mqttPublishState() {
  if (!mqttClient.connected()) return;

  extern Pet pet;
  StaticJsonDocument<512> doc;
  doc["hunger"] = pet.hunger;
  doc["happiness"] = pet.happiness;
  doc["health"] = pet.health;
  doc["energy"] = pet.energy;
  doc["cleanliness"] = pet.cleanliness;
  doc["isAlive"] = pet.isAlive;
  doc["state"] = pet.state;
  doc["age"] = pet.age;
  doc["stage"] = pet.stage;
  doc["mood"] = pet.mood;
  doc["battery"] = pet.batteryLevel;
  doc["isDying"] = pet.isDying;
  doc["isEvolving"] = pet.isEvolving;
  doc["name"] = pet.name;

  String payload;
  serializeJson(doc, payload);
  mqttClient.publish(TOPIC_STATE, payload.c_str());
}

void mqttPublishNotification(const String &message) {
  if (!mqttClient.connected()) return;

  StaticJsonDocument<256> doc;
  doc["message"] = message;
  doc["timestamp"] = millis();

  String payload;
  serializeJson(doc, payload);
  mqttClient.publish(TOPIC_NOTIFICATION, payload.c_str());
}

bool isMQTTConnected() {
  return mqttClient.connected();
}

String getMQTTStatusJson() {
  StaticJsonDocument<256> doc;
  doc["enabled"] = mqttEnabled;
  doc["connected"] = mqttClient.connected();
  doc["broker"] = MQTT_BROKER;
  doc["port"] = MQTT_PORT;
  doc["clientId"] = MQTT_CLIENT_ID;

  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Web UI Endpoints
// ============================================================
void registerMQTTRoutes(WebServer &server) {
  // GET /mqtt/status — current MQTT connection status
  server.on("/mqtt/status", HTTP_GET, [&server]() {
    server.send(200, "application/json", getMQTTStatusJson());
  });

  // POST /mqtt/config — update MQTT settings at runtime
  server.on("/mqtt/config", HTTP_POST, [&server]() {
    if (!checkRateLimit(server.client().remoteIP().toString())) {
      server.send(429, "application/json", "{\"success\":false,\"error\":\"rate_limit\",\"message\":\"Too many requests\"}");
      return;
    }
    String body = server.arg("plain");
    StaticJsonDocument<512> jsonDoc;
    DeserializationError err = deserializeJson(jsonDoc, body);
    if (err) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
      return;
    }

    if (jsonDoc["enabled"].is<bool>()) {
      mqttEnabled = jsonDoc["enabled"].as<bool>();
      if (!mqttEnabled && mqttClient.connected()) {
        mqttClient.disconnect();
      }
    }

    // Note: broker/port changes require reconnection
    if (jsonDoc["reconnect"].is<bool>() && jsonDoc["reconnect"].as<bool>()) {
      if (mqttClient.connected()) {
        mqttClient.disconnect();
      }
      reconnectInterval = MQTT_RECONNECT_INITIAL;
      mqttConnect();
    }

    server.send(200, "application/json", "{\"success\":true}");
  });

  // POST /mqtt/test — publish a test notification
  server.on("/mqtt/test", HTTP_POST, [&server]() {
    if (!checkRateLimit(server.client().remoteIP().toString())) {
      server.send(429, "application/json", "{\"success\":false,\"error\":\"rate_limit\"}");
      return;
    }
    mqttPublishNotification("Test notification from TamaPetchi");
    server.send(200, "application/json", "{\"success\":true}");
  });
}
