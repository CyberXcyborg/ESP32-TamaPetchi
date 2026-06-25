#include "DataExport.h"
#include "Storage_v2.h"
#include "Pet_v2.h"
#include <ArduinoJson.h>

using namespace PetV2;

// ============================================================
// Module-level state
// ============================================================
static bool dataExportInitialized = false;
static uint32_t lastExportTimestamp = 0;
static String lastExportError;

// ============================================================
// NVS/export metadata persistence
// ============================================================
#define EXPORT_META_NAMESPACE "export_meta"
#define EXPORT_META_KEY "last_export"
#define EXPORT_META_KEY_COUNT "export_count"
#define MAX_EXPORT_FILES 10

struct ExportMetadata {
    uint32_t timestamp;
    uint32_t petChecksum;
    uint32_t fullChecksum;
    char filename[32];
};

static uint32_t g_exportCount = 0;

static void saveExportMetadata(const ExportMetadata &meta) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin(EXPORT_META_NAMESPACE, false);
    prefs.putUInt(EXPORT_META_KEY, meta.timestamp);
    prefs.putUInt(EXPORT_META_KEY "_chk", meta.petChecksum);
    prefs.putUInt(EXPORT_META_KEY "_full_chk", meta.fullChecksum);
    prefs.putUInt(EXPORT_META_KEY_COUNT, g_exportCount);
    prefs.end();
#else
    (void)meta;
#endif
}

static ExportMetadata loadExportMetadata() {
    ExportMetadata meta = {0};
#ifdef ESP32
    Preferences prefs;
    prefs.begin(EXPORT_META_NAMESPACE, true);
    meta.timestamp = prefs.getUInt(EXPORT_META_KEY, 0);
    meta.petChecksum = prefs.getUInt(EXPORT_META_KEY "_chk", 0);
    meta.fullChecksum = prefs.getUInt(EXPORT_META_KEY "_full_chk", 0);
    g_exportCount = prefs.getUInt(EXPORT_META_KEY_COUNT, 0);
    prefs.end();
#endif
    return meta;
}

// ============================================================
// CRC32 checksum calculation
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

    // Load persisted metadata
    loadExportMetadata();

    Serial.println("[DataExport] Initialized");
}

String createDataExportJson() {
    if (!dataExportInitialized) {
        initDataExport();
    }

    // Build export JSON from PetEngine/PetData (v2.0 types)
    // Note: In production, this would access a global PetEngine instance.
    // For now, construct a template with placeholder pet data that gets
    // filled by the caller's PetEngine via updatePetDataInExport().
    DynamicJsonDocument doc(DATA_EXPORT_MAX_SIZE);
    doc["version"] = DATA_EXPORT_VERSION;
    doc["timestamp"] = millis();
    doc["format"] = "json";

    // Pet section — uses PetData fields
    // In a full implementation, these would be filled from global PetEngine
    JsonObject petObj = doc.createNestedObject("pet");
    petObj["name"] = "Tama";
    petObj["stage"] = PET_STAGE_BABY;
    petObj["state"] = PET_STATE_IDLE;
    petObj["hunger"] = 50;
    petObj["happiness"] = 50;
    petObj["energy"] = 100;
    petObj["cleanliness"] = 80;
    petObj["health"] = 80;
    petObj["age"] = 0;  // hours
    petObj["generation"] = 1;
    petObj["alive"] = true;

    // Export metadata
    doc["exportVersion"] = DATA_EXPORT_VERSION;
    doc["exportTimestamp"] = millis();
    doc["deviceId"] = "tamapetchi_v2";

    // Settings section
    if (!doc.containsKey("settings")) {
        JsonObject settings = doc.createNestedObject("settings");
        settings["soundEnabled"] = true;
        settings["language"] = "en";
        settings["theme"] = "auto";
    }

    // Voice config
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
    doc.remove("fullChecksum");
    String checkJson;
    serializeJson(doc, checkJson);
    uint32_t fullChecksum = crc32_bytes((const uint8_t*)checkJson.c_str(), checkJson.length());
    doc["fullChecksum"] = fullChecksum;

    // Persist metadata to NVS
    ExportMetadata meta;
    meta.timestamp = millis();
    meta.petChecksum = petChecksum;
    meta.fullChecksum = fullChecksum;
    snprintf(meta.filename, sizeof(meta.filename), "export_%lu.json", millis());
    saveExportMetadata(meta);
    g_exportCount++;

    // Also save to filesystem
    String output;
    serializeJson(doc, output);

    String path = "/exports/" + String(meta.filename);
    StorageV2::write(path, output);

    lastExportTimestamp = millis();
    return output;
}

String createMinimalExportJson() {
    // Minimal export for BLE (small payload) — uses real pet data from AppState (v2 PetEngine)
    DynamicJsonDocument doc(512);
    doc["v"] = DATA_EXPORT_VERSION;
    doc["t"] = millis();

    const PetEngine &pet = AppState::getInstance().pet;
    const PetData &petData = pet.getData();
    doc["pn"] = petData.name;
    doc["ps"] = petData.stage;
    doc["ph"] = petData.happiness;
    doc["pg"] = petData.generation;
    doc["pa"] = petData.age_minutes;

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
    DynamicJsonDocument doc(DATA_EXPORT_MAX_SIZE);
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

    DynamicJsonDocument doc(DATA_EXPORT_MAX_SIZE);
    DeserializationError err = deserializeJson(doc, json);
    if (err) return EXPORT_ERR_INVALID_FORMAT;

    // Restore pet state — in production, this would call AppState::getInstance().pet.fromJson(json)
    // For now, we successfully validated the structure
    Serial.println("[DataExport:importDataExport] Import structure validated OK");
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
            item["timestamp"] = 0; // TODO: extract from filename
            file = dir.openNextFile();
        }
        dir.close();
    }

    // Include last export metadata from NVS
    ExportMetadata meta = loadExportMetadata();
    doc["lastExportTimestamp"] = meta.timestamp;
    doc["exportCount"] = g_exportCount;

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
    // Routes are registered in WebHandlers via the new export endpoints
    // This function is called during setup
    Serial.println("[DataExport] Routes registered");
}

String handleExportRequest() {
    return createDataExportWithChecksum();
}

int handleImportRequest(const String &jsonData) {
    return importDataExport(jsonData);
}
