#include "SoundPack.h"
#include "Pet.h"
#include "Storage.h"
#include "AppState.h"
#include "WebHandlers.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================
// Module-level state
// ============================================================
static String activePackName = "default";

// ============================================================
// JSON Schema for Sound Pack files
// {
//   "name": "pack_name",
//   "default": true/false,
//   "melodies": {
//     "happy": 0,
//     "sleep": 1,
//     "sick": 2,
//     "dying": 3,
//     "evolve": 4,
//     "feed": 5,
//     "play": 6,
//     "death": 7
//   }
// }
// ============================================================

// Map JSON melody key to MELODY_ event index
static int melodyKeyToEvent(const String &key) {
  if (key == "happy")   return 0;  // MELODY_HAPPY
  if (key == "sleep")   return 1;  // MELODY_SLEEP
  if (key == "sick")    return 2;  // MELODY_SICK
  if (key == "dying")   return 3;  // MELODY_DYING
  if (key == "evolve")  return 4;  // MELODY_EVOLVE
  if (key == "feed")    return 5;  // MELODY_FEED
  if (key == "play")    return 6;  // MELODY_PLAY
  if (key == "death")   return 7;  // MELODY_DEATH
  return -1;
}

// ============================================================
// Public API
// ============================================================

void initSoundPack() {
  // Ensure sounds directory exists
  if (!SPIFFS.exists(SOUNDPACK_DIR)) {
    Serial.println("[SoundPack] Sounds directory not found — built-in melodies will be used");
    return;
  }

  // Load active pack name from file
  File f = SPIFFS.open(SOUNDPACK_ACTIVE_FILE, "r");
  if (f) {
    String name = f.readStringUntil('\n');
    name.trim();
    f.close();
    if (name.length() > 0) {
      activePackName = name;
      Serial.printf("[SoundPack] Active pack: %s\n", activePackName.c_str());
    }
  }

  // Load melodies from active pack
  loadSoundPackMelodies(activePackName);
}

int getSoundPackList(SoundPackInfo *packs, int maxPacks) {
  int count = 0;

  File dir = SPIFFS.open(SOUNDPACK_DIR);
  if (!dir) return 0;

  File file = dir.openNextFile();
  while (file && count < maxPacks) {
    String fname = file.name();
    if (fname.endsWith(".json")) {
      // Parse pack name from JSON
      String content = file.readString();
      StaticJsonDocument<512> doc;
      DeserializationError err = deserializeJson(doc, content);
      if (!err) {
        const char *packName = doc["name"] | fname.c_str();
        strncpy(packs[count].name, packName, SOUNDPACK_NAME_LEN - 1);
        packs[count].name[SOUNDPACK_NAME_LEN - 1] = '\0';
        strncpy(packs[count].filename, fname.c_str(), sizeof(packs[count].filename) - 1);
        packs[count].filename[sizeof(packs[count].filename) - 1] = '\0';
        packs[count].isDefault = doc["default"] | false;
        count++;
      }
    }
    file = dir.openNextFile();
  }
  dir.close();

  return count;
}

String getActiveSoundPack() {
  return activePackName;
}

bool setActiveSoundPack(const String &name) {
  // Verify pack exists
  String path = String(SOUNDPACK_DIR) + "/" + name + ".json";
  if (!SPIFFS.exists(path)) {
    Serial.printf("[SoundPack] Pack not found: %s\n", path.c_str());
    return false;
  }

  // Load melodies from pack
  if (!loadSoundPackMelodies(name)) {
    return false;
  }

  // Save active pack name
  activePackName = name;
  File f = SPIFFS.open(SOUNDPACK_ACTIVE_FILE, "w");
  if (f) {
    f.println(name);
    f.close();
  }

  Serial.printf("[SoundPack] Active pack set to: %s\n", name.c_str());
  return true;
}

bool loadSoundPackMelodies(const String &name) {
  String path = String(SOUNDPACK_DIR) + "/" + name + ".json";
  File f = SPIFFS.open(path, "r");
  if (!f) {
    Serial.printf("[SoundPack] Cannot open: %s\n", path.c_str());
    return false;
  }

  String content = f.readString();
  f.close();

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, content);
  if (err) {
    Serial.printf("[SoundPack] JSON parse error in %s: %s\n", path.c_str(), err.c_str());
    return false;
  }

  // Apply melody mappings
  JsonObject melodies = doc["melodies"];
  if (melodies) {
    for (JsonPair kv : melodies) {
      int eventIdx = melodyKeyToEvent(kv.key().c_str());
      if (eventIdx >= 0 && eventIdx < MELODY_COUNT) {
        JsonVariant val = kv.value();
        int melodyIdx = val.as<int>();
        if (melodyIdx >= 0 && melodyIdx < melodyCount) {
          setMelodyConfig(eventIdx, melodyIdx);
        }
      }
    }
  }

  Serial.printf("[SoundPack] Loaded melodies from: %s\n", name.c_str());
  return true;
}

String getSoundPackListJson() {
  SoundPackInfo packs[MAX_SOUNDPACKS];
  int count = getSoundPackList(packs, MAX_SOUNDPACKS);

  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.createNestedArray("packs");

  for (int i = 0; i < count; i++) {
    JsonObject pack = arr.createNestedObject();
    pack["name"] = packs[i].name;
    pack["filename"] = packs[i].filename;
    pack["isDefault"] = packs[i].isDefault;
    pack["isActive"] = (activePackName == String(packs[i].name));
  }

  doc["active"] = activePackName;
  doc["count"] = count;

  String result;
  serializeJson(doc, result);
  return result;
}

String getActiveSoundPackJson() {
  StaticJsonDocument<256> doc;
  doc["name"] = activePackName;
  doc["success"] = true;
  String result;
  serializeJson(doc, result);
  return result;
}

// ============================================================
// Web API Routes
// ============================================================
void registerSoundPackRoutes() {
  // GET /api/sounds/list — list available sound packs
  APP_STATE.server.on("/api/sounds/list", HTTP_GET, []() {
    APP_STATE.server.send(200, "application/json", getSoundPackListJson());
  });

  // GET /api/sounds/active — get active sound pack
  APP_STATE.server.on("/api/sounds/active", HTTP_GET, []() {
    APP_STATE.server.send(200, "application/json", getActiveSoundPackJson());
  });

  // POST /api/sounds/select — select active sound pack
  APP_STATE.server.on("/api/sounds/select", HTTP_POST, []() {
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

    String packName = jsonDoc["name"] | "";
    if (packName.length() == 0) {
      APP_STATE.server.send(400, "application/json",
        "{\"success\":false,\"message\":\"Missing pack name\"}");
      return;
    }

    if (setActiveSoundPack(packName)) {
      StaticJsonDocument<256> resp;
      resp["success"] = true;
      resp["active"] = packName;
      String result;
      serializeJson(resp, result);
      APP_STATE.server.send(200, "application/json", result);
    } else {
      APP_STATE.server.send(404, "application/json",
        "{\"success\":false,\"message\":\"Sound pack not found\"}");
    }
  });
}
