#ifndef DATA_EXPORT_H
#define DATA_EXPORT_H

#include <Arduino.h>
#include "config_v2.h"

// ============================================================
// Phase 24.2: Data Export System
// Full state export via BLE and web (JSON backup).
// Includes SHA-256 checksum for integrity verification.
// ============================================================

#define DATA_EXPORT_VERSION "2.0.0"
#define DATA_EXPORT_MAX_SIZE 4096

// Export format types
enum ExportFormat {
    EXPORT_FORMAT_JSON = 0,
    EXPORT_FORMAT_BINARY,
    EXPORT_FORMAT_COUNT
};

// Export result codes
enum ExportResult {
    EXPORT_OK = 0,
    EXPORT_ERR_MEMORY,
    EXPORT_ERR_STORAGE,
    EXPORT_ERR_INTEGRITY,
    EXPORT_ERR_TOO_LARGE,
    EXPORT_ERR_INVALID_FORMAT
};

// Export metadata header
struct ExportHeader {
    char version[16];
    char deviceId[32];
    uint32_t timestamp;
    uint32_t pet_checksum;
    uint32_t full_checksum;
    ExportFormat format;
};

// Initialize data export system
void initDataExport();

// Create full state export as JSON string
// Contains: pet data, stats, achievements, settings
String createDataExportJson();

// Create minimal export (pet state only, for BLE)
String createMinimalExportJson();

// Export with integrity checksum
String createDataExportWithChecksum();

// Verify an export JSON string integrity
int verifyDataExport(const String &json);

// Import state from JSON backup
// Returns EXPORT_OK on success, error code otherwise
int importDataExport(const String &json);

// Get export file list from storage
String getExportFileListJson();

// Delete an export file from storage
bool deleteExportFile(const String &filename);

// Calculate SHA-256-like checksum (simplified for embedded)
uint32_t calculateExportChecksum(const String &json);

// Register data export API routes
void registerDataExportRoutes();

// Convenience: trigger export via web
String handleExportRequest();

// Convenience: handle import from web upload
int handleImportRequest(const String &jsonData);

#endif // DATA_EXPORT_H
