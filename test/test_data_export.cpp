// test/test_data_export.cpp — Phase 24.2 Data Export System Tests
#include "Arduino.h"
#include "DataExport.h"
#include "config_v2.h"

#ifdef UNIT_TEST

#include <cstdint>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { tests_passed++; printf("  PASS: %s\n", msg); } \
    else { tests_failed++; printf("  FAIL: %s\n", msg); } \
} while(0)

void test_export_init() {
    printf("[test_export_init]\n");
    initDataExport();
    TEST_ASSERT(true, "Data export initialized without crash");
}

void test_export_create_json() {
    printf("[test_export_create_json]\n");
    String json = createDataExportJson();
    TEST_ASSERT(json.length() > 0, "Export JSON not empty");
    TEST_ASSERT(json.indexOf("version") >= 0, "Export contains version");
    TEST_ASSERT(json.indexOf("pet") >= 0, "Export contains pet section");
    TEST_ASSERT(json.indexOf("settings") >= 0, "Export contains settings section");
}

void test_export_minimal() {
    printf("[test_export_minimal]\n");
    String json = createMinimalExportJson();
    TEST_ASSERT(json.length() > 0, "Minimal export not empty");
    TEST_ASSERT(json.indexOf("v") >= 0, "Minimal export has version field");
    TEST_ASSERT(json.length() < 512, "Minimal export is small (<512 bytes)");
}

void test_export_with_checksum() {
    printf("[test_export_with_checksum]\n");
    String json = createDataExportWithChecksum();
    TEST_ASSERT(json.indexOf("petChecksum") >= 0, "Export has pet checksum");
    TEST_ASSERT(json.indexOf("fullChecksum") >= 0, "Export has full checksum");
}

void test_export_verify() {
    printf("[test_export_verify]\n");
    String json = createDataExportWithChecksum();
    int result = verifyDataExport(json);
    TEST_ASSERT(result == 0, "Valid export verifies OK");
}

void test_export_verify_corrupt() {
    printf("[test_export_verify_corrupt]\n");
    String json = createDataExportWithChecksum();
    // Corrupt the JSON
    json.replace("pet", "xyz");
    int result = verifyDataExport(json);
    TEST_ASSERT(result != 0, "Corrupted export fails verification");
}

void test_export_invalid() {
    printf("[test_export_invalid]\n");
    int result = verifyDataExport("not json at all");
    TEST_ASSERT(result != 0, "Invalid JSON fails verification");
}

void test_export_checksum_calc() {
    printf("[test_export_checksum_calc]\n");
    String testStr = "test data for checksum";
    uint32_t crc1 = calculateExportChecksum(testStr);
    uint32_t crc2 = calculateExportChecksum(testStr);
    TEST_ASSERT(crc1 == crc2, "Same data produces same checksum");
    
    uint32_t crc3 = calculateExportChecksum("different data");
    TEST_ASSERT(crc1 != crc3, "Different data produces different checksum");
}

void test_export_file_list() {
    printf("[test_export_file_list]\n");
    String json = getExportFileListJson();
    TEST_ASSERT(json.length() > 0, "File list JSON not empty");
    TEST_ASSERT(json.indexOf("exports") >= 0, "File list has exports array");
}

int run_data_export_tests() {
    printf("\n=== Data Export Tests ===\n");
    test_export_init();
    test_export_create_json();
    test_export_minimal();
    test_export_with_checksum();
    test_export_verify();
    test_export_verify_corrupt();
    test_export_invalid();
    test_export_checksum_calc();
    test_export_file_list();
    
    printf("Data Export Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed;
}

#endif // UNIT_TEST
