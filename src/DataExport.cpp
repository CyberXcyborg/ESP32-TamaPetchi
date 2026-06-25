#include "DataExport.h"
#include "AppState_v2.h"
#include "Storage_v2.h"
#include "Pet_v2.h"
#include <ArduinoJson.h>


// ============================================================
// Module-level state
// ============================================================
static bool dataExportInitialized = false;
static uint32_t lastExportTimestamp = 0;
static String lastExportError;

// ============================================================
// CRC32 checksum calculation
// Uses CRC32 for speed; sufficient for integrity checking
// ============================================================
static uint32_t crc32_bytes(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

// ============================================================
// Public API
// ============================================================

void initDataExport() {
    dataExportInitialized = true;
    
    // Ensure export directory exists
    StorageV2::begin();
    if (!StorageV2::exists("/exports")) {
        StorageV2::mkdir("/exports");
    }
    
    Serial.println("[DataExport] Initialized");
}

String createDataExportJson() {
    if (!dataExportInitialized) {
        initDataExport();
    }

    // Build export JSON from real pet data via AppState
    DynamicJsonDocument doc(3072);
    doc["version"] = DATA_EXPORT_VERSION;
    doc["timestamp"] = millis();
    doc["format"] = "json";

    // Use real pet data from AppStateV2 (v2.0 PetEngine)
    const PetEngine &pet = AppStateV2::getInstance().pet;
    const PetData &petData = pet.getData();

    JsonObject petObj = doc.createNestedObject("pet");
    petObj["name"] = petData.name;
    petObj["stage"] = petData.stage;
    petObj["state"] = petData.state;
    petObj["hunger"] = petData.hunger;
    petObj["happiness"] = petData.happiness;
    petObj["energy"] = petData.energy;
    petObj["cleanliness"] = petData.cleanliness;
    petObj["health"] = petData.health;
    petObj["age"] = petData.age_minutes / 60;  // hours
    petObj["generation"] = petData.generation;
    petObj["alive"] = pet.isAlive();

    // Add export metadata
    doc["exportVersion"] = DATA_EXPORT_VERSION;
    doc["exportTimestamp"] = millis();
    doc["deviceId"] = "tamapetchi_v2";

    // Add settings section if not present
    if (!doc.containsKey("settings")) {
        JsonObject settings = doc.createNestedObject("settings");
        settings["soundEnabled"] = true;
        settings["language"] = "en";
        settings["theme"] = "auto";
    }

    // Add voice config
    JsonObject voice = doc.createNestedObject("voice");
    voice["enabled"] = true;
    voice["volume"] = 70;
    voice["activePack"] = "default";

    // Calculate integrity checksum on pet data
    String petJson;
    serializeJson(doc["pet"], petJson);
    uint32_t petChecksum = crc32_bytes((const uint8_t*)petJson.c_str(), petJson.length());
    doc["petChecksum"] = petChecksum;

    // Full checksum
    String fullJson;
    serializeJson(doc, fullJson);
    // Remove the full_checksum field itself from checksum calculation
    doc.remove("fullChecksum");
    String checkJson;
    serializeJson(doc, checkJson);
    uint32_t fullChecksum = crc32_bytes((const uint8_t*)checkJson.c_str(), checkJson.length());
    doc["fullChecksum"] = fullChecksum;

    String output;
    serializeJson(doc, output);
    
    lastExportTimestamp = millis();
    return output;
}

String createMinimalExportJson() {
    // Minimal export for BLE (small payload) — uses PetEngine only (v2.0)
    DynamicJsonDocument doc(512);
    doc["v"] = DATA_EXPORT_VERSION;
    doc["t"] = millis();

    const PetEngine &pet = AppStateV2::getInstance().pet;
    const PetData &petData = pet.getData();
    doc["pn"] = petData.name;
    doc["ps"] = petData.stage;
    doc["ph"] = petData.happiness;
    doc["pg"] = petData.generation;
    doc["pa"] = petData.age_minutes / 60;

    String output;
    serializeJson(doc, output);
    return output;
}

String createDataExportWithChecksum() {
    String json = createDataExportJson();
    // Checksum is already embedded in the export
    return json;
}

int verifyDataExport(const String &json) {
    DynamicJsonDocument doc(3072);
    DeserializationError err = deserializeJson(doc, json);
    if (err) return EXPORT_ERR_INVALID_FORMAT;

    // Verify version compatibility
    const char *ver = doc["version"];
    if (!ver) return EXPORT_ERR_INVALID_FORMAT;

    // Verify pet checksum if present
    if (doc.containsKey("petChecksum")) {
        uint32_t storedPetChecksum = doc["petChecksum"];
        String petJson;
        serializeJson(doc["pet"], petJson);
        uint32_t calcPetChecksum = crc32_bytes((const uint8_t*)petJson.c_str(), petJson.length());
        if (storedPetChecksum != calcPetChecksum) {
            lastExportError = "Pet checksum mismatch";
            return EXPORT_ERR_INTEGRITY;
        }
    }

    // Verify full checksum if present
    if (doc.containsKey("fullChecksum")) {
        uint32_t storedFullChecksum = doc["fullChecksum"];
        doc.remove("fullChecksum");
        String checkJson;
        serializeJson(doc, checkJson);
        uint32_t calcFullChecksum = crc32_bytes((const uint8_t*)checkJson.c_str(), checkJson.length());
        if (storedFullChecksum != calcFullChecksum) {
            lastExportError = "Full checksum mismatch";
            return EXPORT_ERR_INTEGRITY;
        }
    }

    return EXPORT_OK;
}

int importDataExport(const String &json) {
    // First verify integrity
    int verifyResult = verifyDataExport(json);
    if (verifyResult != EXPORT_OK) return verifyResult;

    DynamicJsonDocument doc(3072);
    DeserializationError err = deserializeJson(doc, json);
    if (err) return EXPORT_ERR_INVALID_FORMAT;

    // Restore pet state via PetEngine fromJson
    PetEngine &pet = AppStateV2::getInstance().pet;
    if (!pet.fromJson(json)) {
        lastExportError = "PetEngine fromJson failed";
        return EXPORT_ERR_STORAGE;
    }

    Serial.println("[DataExport] Import successful");
    return EXPORT_OK;
}

String getExportFileListJson() {
    DynamicJsonDocument doc(512);
    JsonArray list = doc.createNestedArray("exports");

    File dir = StorageV2::open("/exports");
    if (dir) {
        File file = dir.openNextFile();
        while (file) {
            JsonObject item = list.createNestedObject();
            item["name"] = String(file.name());
            item["size"] = file.size();
            file = dir.openNextFile();
        }
        dir.close();
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool deleteExportFile(const String &filename) {
    String path = "/exports/" + filename;
    if (StorageV2::exists(path)) {
        return StorageV2::remove(path);
    }
    return false;
}

uint32_t calculateExportChecksum(const String &json) {
    return crc32_bytes((const uint8_t*)json.c_str(), json.length());
}

void registerDataExportRoutes() {
    // Routes are registered in WebHandlers.cpp
    // This function is called during setup
    Serial.println("[DataExport] Routes registered");
}

String handleExportRequest() {
    return createDataExportWithChecksum();
}

int handleImportRequest(const String &jsonData) {
    return importDataExport(jsonData);
}
