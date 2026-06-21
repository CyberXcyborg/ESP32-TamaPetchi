#ifndef ERRORCODE_H
#define ERRORCODE_H

// ============================================================
// Phase 10.6: Structured Error Codes
//
// All API responses use a consistent error object format:
//   { "success": false, "error": "<CODE>", "message": "<description>" }
//
// Error codes are namespaced by category:
//   ERR_SPIFFS_*  — SPIFFS storage errors
//   ERR_JSON_*    — JSON parsing/serialization errors
//   ERR_PET_*     — Pet state errors
//   ERR_WIFI_*    — WiFi connection errors
//   ERR_OTA_*     — OTA update errors
//   ERR_MQTT_*    — MQTT connection errors
//   ERR_RATE_*    — Rate limiting errors
//   ERR_AUTH_*    — Authentication errors
//   ERR_PARAM_*   — Invalid parameter errors
//   ERR_MEMORY_*  — Memory allocation errors
//   ERR_SYSTEM_*  — System-level errors
// ============================================================

// SPIFFS errors
#define ERR_SPIFFS_READ_FAIL    "spiffs_read_fail"
#define ERR_SPIFFS_WRITE_FAIL   "spiffs_write_fail"
#define ERR_SPIFFS_FORMAT_FAIL  "spiffs_format_fail"
#define ERR_SPIFFS_FULL         "spiffs_full"
#define ERR_SPIFFS_CORRUPT      "spiffs_corrupt"

// JSON errors
#define ERR_JSON_PARSE_FAIL     "json_parse_fail"
#define ERR_JSON_SERIALIZE_FAIL "json_serialize_fail"
#define ERR_JSON_TOO_LARGE      "json_too_large"

// JSON error integer codes
#define ERR_INT_JSON_PARSE_FAIL     100
#define ERR_INT_JSON_SERIALIZE_FAIL 101
#define ERR_INT_JSON_TOO_LARGE      102

// Pet state errors
#define ERR_PET_NOT_FOUND       "pet_not_found"
#define ERR_PET_DEAD            "pet_dead"
#define ERR_PET_NO_SLOTS        "pet_no_free_slots"
#define ERR_PET_INVALID_SLOT    "pet_invalid_slot"
#define ERR_PET_INVALID_NAME    "pet_invalid_name"
#define ERR_PET_INVALID_TYPE    "pet_invalid_type"
#define ERR_PET_SAVE_FAIL       "pet_save_fail"

// WiFi errors
#define ERR_WIFI_CONNECT_FAIL   "wifi_connect_fail"
#define ERR_WIFI_AP_FAIL        "wifi_ap_fail"
#define ERR_WIFI_CONFIG_FAIL    "wifi_config_fail"

// OTA errors
#define ERR_OTA_BEGIN_FAIL      "ota_begin_fail"
#define ERR_OTA_WRITE_FAIL      "ota_write_fail"
#define ERR_OTA_END_FAIL        "ota_end_fail"
#define ERR_OTA_AUTH_FAIL       "ota_auth_fail"
#define ERR_OTA_NO_SPACE        "ota_no_space"

// MQTT errors
#define ERR_MQTT_CONNECT_FAIL   "mqtt_connect_fail"
#define ERR_MQTT_PUBLISH_FAIL   "mqtt_publish_fail"
#define ERR_MQTT_SUBSCRIBE_FAIL "mqtt_subscribe_fail"

// Rate limiting
#define ERR_RATE_LIMIT          "rate_limit"

// Authentication
#define ERR_AUTH_REQUIRED       "auth_required"
#define ERR_AUTH_INVALID        "auth_invalid"

// Parameter validation
#define ERR_PARAM_MISSING       "param_missing"
#define ERR_PARAM_INVALID       "param_invalid"
#define ERR_PARAM_OUT_OF_RANGE  "param_out_of_range"

// Memory
#define ERR_MEMORY_ALLOC_FAIL   "memory_alloc_fail"
#define ERR_MEMORY_LOW          "memory_low"

// System
#define ERR_SYSTEM_BUSY         "system_busy"
#define ERR_SYSTEM_RESTARTING   "system_restarting"
#define ERR_SYSTEM_UNKNOWN      "system_unknown"

// Backup errors (integer codes for function return values)
#define ERR_BACKUP_VERSION_MISSING    700
#define ERR_BACKUP_NO_PET             701
#define ERR_BACKUP_NO_CHECKSUM        702
#define ERR_BACKUP_CHECKSUM_MISMATCH  703
#define ERR_BACKUP_MIGRATE_FAIL       704

// Backup error strings (for API responses)
#define ERR_STR_BACKUP_VERSION_MISSING    "backup_version_missing"
#define ERR_STR_BACKUP_NO_PET             "backup_no_pet"
#define ERR_STR_BACKUP_NO_CHECKSUM        "backup_no_checksum"
#define ERR_STR_BACKUP_CHECKSUM_MISMATCH  "backup_checksum_mismatch"
#define ERR_STR_BACKUP_MIGRATE_FAIL       "backup_migrate_fail"

// Success
#define ERR_OK                        0

#endif // ERRORCODE_H
