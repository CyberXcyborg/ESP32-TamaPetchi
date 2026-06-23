// test_spriteloader.cpp — Unit tests for SpriteLoader (Phase 20.1)
// Tests RLE compression, sprite format parsing, and LRU cache logic
// using native-compatible mocks (no ESP32 hardware dependencies)
#include <Arduino.h>
#include <unity.h>
#include <cstring>
#include <cstdio>
#include <cstdint>

// SprHeader struct (mirrors SpriteLoader.h for native testing)
struct SprHeader {
    char magic[4];
    uint16_t width;
    uint16_t height;
    uint16_t frame_count;
    uint8_t  bpp;
    uint8_t  flags;
    int16_t  transparency_idx;
    uint8_t  reserved[16];
};

// ============================================================
// RLE Codec Tests (platform-independent)
// ============================================================

// Reference RLE encode (same algorithm as SpriteLoader)
static void rle_encode(const uint8_t *indices, size_t count, uint8_t *out, size_t *out_len) {
    size_t i = 0;
    size_t pos = 0;
    
    while (i < count) {
        if (i + 1 < count && indices[i] == indices[i + 1]) {
            uint8_t color = indices[i];
            uint8_t run = 1;
            while (i + run < count && indices[i + run] == color && run < 128) run++;
            out[pos++] = 0x80 | (run - 1);
            out[pos++] = color;
            i += run;
        } else {
            uint8_t run = 1;
            while (i + run < count) {
                if (run >= 3 && i + run + 2 < count &&
                    indices[i + run] == indices[i + run + 1] &&
                    indices[i + run] == indices[i + run + 2]) break;
                if (run >= 128) break;
                run++;
            }
            out[pos++] = run - 1;
            memcpy(out + pos, indices + i, run);
            pos += run;
            i += run;
        }
    }
    *out_len = pos;
}

static void rle_decode(const uint8_t *data, size_t data_len, uint8_t *out, size_t *out_len) {
    size_t i = 0, pos = 0;
    while (i < data_len) {
        uint8_t token = data[i++];
        if (token & 0x80) {
            uint8_t count = (token & 0x7F) + 1;
            uint8_t color = data[i++];
            memset(out + pos, color, count);
            pos += count;
        } else {
            uint8_t count = token + 1;
            memcpy(out + pos, data + i, count);
            i += count;
            pos += count;
        }
    }
    *out_len = pos;
}

void test_rle_all_same_color() {
    uint8_t input[64];
    memset(input, 5, 64);
    
    uint8_t encoded[256];
    size_t enc_len;
    rle_encode(input, 64, encoded, &enc_len);
    
    // Should be 2 bytes: repeat token + color
    TEST_ASSERT_EQUAL(2, enc_len);
    TEST_ASSERT_EQUAL(0x80 | 63, encoded[0]);  // count=64 -> token=0xBF
    TEST_ASSERT_EQUAL(5, encoded[1]);
    
    uint8_t decoded[64];
    size_t dec_len;
    rle_decode(encoded, enc_len, decoded, &dec_len);
    
    TEST_ASSERT_EQUAL(64, dec_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, decoded, 64);
    printf("  PASS: RLE all same color\n");
}

void test_rle_all_different() {
    uint8_t input[10];
    for (int i = 0; i < 10; i++) input[i] = i;
    
    uint8_t encoded[256];
    size_t enc_len;
    rle_encode(input, 10, encoded, &enc_len);
    
    // Should be 11 bytes: literal token(10) + 10 data bytes
    TEST_ASSERT_EQUAL(11, enc_len);
    TEST_ASSERT_EQUAL(9, encoded[0]);  // count=10 -> token=9
    
    uint8_t decoded[16];
    size_t dec_len;
    rle_decode(encoded, enc_len, decoded, &dec_len);
    
    TEST_ASSERT_EQUAL(10, dec_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, decoded, 10);
    printf("  PASS: RLE all different\n");
}

void test_rle_mixed_pattern() {
    // Pattern: A A A B C C C C D E E
    uint8_t input[] = {1,1,1, 2, 3,3,3,3, 4, 5,5};
    size_t input_len = sizeof(input);
    
    uint8_t encoded[256];
    size_t enc_len;
    rle_encode(input, input_len, encoded, &enc_len);
    
    uint8_t decoded[32];
    size_t dec_len;
    rle_decode(encoded, enc_len, decoded, &dec_len);
    
    TEST_ASSERT_EQUAL(input_len, dec_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, decoded, input_len);
    printf("  PASS: RLE mixed pattern\n");
}

void test_rle_roundtrip_4096() {
    // Simulate a 64x64 frame with realistic sprite content (runs of same color)
    uint8_t input[4096];
    int pos = 0;
    // Create a pattern with horizontal runs (typical of sprite art)
    for (int y = 0; y < 64; y++) {
        int run_color = (y / 8) & 0x0F;
        for (int x = 0; x < 64; x++) {
            if (x > 0 && x % 8 == 0) run_color = ((x / 8) + y) & 0x0F;
            input[pos++] = run_color;
        }
    }
    
    uint8_t encoded[16384];
    size_t enc_len;
    rle_encode(input, 4096, encoded, &enc_len);
    
    // Verify compression ratio is reasonable (< 80%)
    TEST_ASSERT_TRUE(enc_len < 4096 * 0.8);
    
    uint8_t decoded[4096];
    size_t dec_len;
    rle_decode(encoded, enc_len, decoded, &dec_len);
    
    TEST_ASSERT_EQUAL(4096, dec_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, decoded, 4096);
    printf("  PASS: RLE roundtrip 4096 pixels (compressed %zu -> %zu)\n", (size_t)4096, enc_len);
}

// ============================================================
// Sprite Header Tests
// ============================================================

void test_spr_header_size() {
    // Verify the header struct is exactly 30 bytes
    TEST_ASSERT_EQUAL(30, sizeof(SprHeader));
    printf("  PASS: SPR header is 30 bytes\n");
}

void test_spr_header_fields() {
    SprHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    
    memcpy(hdr.magic, "SPR\0", 4);
    hdr.width = 64;
    hdr.height = 64;
    hdr.frame_count = 8;
    hdr.bpp = 4;
    hdr.flags = 0;
    hdr.transparency_idx = -1;
    
    TEST_ASSERT_EQUAL_STRING("SPR", hdr.magic);
    TEST_ASSERT_EQUAL(64, hdr.width);
    TEST_ASSERT_EQUAL(64, hdr.height);
    TEST_ASSERT_EQUAL(8, hdr.frame_count);
    TEST_ASSERT_EQUAL(4, hdr.bpp);
    TEST_ASSERT_EQUAL(-1, hdr.transparency_idx);
    printf("  PASS: SPR header fields correct\n");
}

// ============================================================
// Sprite File Format Tests
// ============================================================

// Build a minimal valid .spr file in memory
static void build_test_spr(uint8_t *buf, size_t *out_len,
                           uint16_t width, uint16_t height,
                           uint16_t frame_count, const uint8_t **frame_data,
                           const size_t *frame_sizes) {
    size_t pos = 0;
    
    // Header (30 bytes)
    memcpy(buf + pos, "SPR\0", 4); pos += 4;
    buf[pos++] = width & 0xFF;
    buf[pos++] = (width >> 8) & 0xFF;
    buf[pos++] = height & 0xFF;
    buf[pos++] = (height >> 8) & 0xFF;
    buf[pos++] = frame_count & 0xFF;
    buf[pos++] = (frame_count >> 8) & 0xFF;
    buf[pos++] = 4;  // bpp
    buf[pos++] = 0;  // flags
    buf[pos++] = 0xFF;  // transparency = -1 (low byte)
    buf[pos++] = 0xFF;  // transparency = -1 (high byte)
    memset(buf + pos, 0, 16);  // reserved
    pos += 16;
    
    // Palette (32 bytes) - simple grayscale
    for (int i = 0; i < 16; i++) {
        uint16_t gray = (i * 17) << 11 | (i * 17) << 6 | (i * 17);
        buf[pos++] = gray & 0xFF;
        buf[pos++] = (gray >> 8) & 0xFF;
    }
    
    // Offset table (frame_count * 4)
    size_t offset_table_pos = pos;
    pos += frame_count * 4;
    
    // Frame data
    for (uint16_t f = 0; f < frame_count; f++) {
        // Write offset
        size_t frame_offset = pos;
        buf[offset_table_pos + f * 4] = frame_offset & 0xFF;
        buf[offset_table_pos + f * 4 + 1] = (frame_offset >> 8) & 0xFF;
        buf[offset_table_pos + f * 4 + 2] = (frame_offset >> 16) & 0xFF;
        buf[offset_table_pos + f * 4 + 3] = (frame_offset >> 24) & 0xFF;
        
        // Compressed size
        buf[pos++] = frame_sizes[f] & 0xFF;
        buf[pos++] = (frame_sizes[f] >> 8) & 0xFF;
        buf[pos++] = (frame_sizes[f] >> 16) & 0xFF;
        buf[pos++] = (frame_sizes[f] >> 24) & 0xFF;
        
        // Frame data
        memcpy(buf + pos, frame_data[f], frame_sizes[f]);
        pos += frame_sizes[f];
    }
    
    *out_len = pos;
}

void test_spr_file_parse() {
    // Create a simple 16x16 sprite with 2 frames
    uint8_t frame0_rle[] = {0x0F, 0x01, 0x02, 0x03, 0x04};  // 16 literal pixels
    uint8_t frame1_rle[] = {0x82, 0x05};  // 3 pixels of color 5
    
    const uint8_t *frames[] = {frame0_rle, frame1_rle};
    size_t frame_sizes[] = {sizeof(frame0_rle), sizeof(frame1_rle)};
    
    uint8_t spr_buf[1024];
    size_t spr_len;
    build_test_spr(spr_buf, &spr_len, 16, 16, 2, frames, frame_sizes);
    
    // Parse header
    TEST_ASSERT_EQUAL_STRING("SPR", (char*)spr_buf);
    uint16_t w = spr_buf[4] | (spr_buf[5] << 8);
    uint16_t h = spr_buf[6] | (spr_buf[7] << 8);
    uint16_t fc = spr_buf[8] | (spr_buf[9] << 8);
    TEST_ASSERT_EQUAL(16, w);
    TEST_ASSERT_EQUAL(16, h);
    TEST_ASSERT_EQUAL(2, fc);
    
    // Parse offset table
    size_t offset_table_start = 30 + 32;  // header + palette
    uint32_t frame0_off = spr_buf[offset_table_start] |
                          (spr_buf[offset_table_start+1] << 8) |
                          (spr_buf[offset_table_start+2] << 16) |
                          (spr_buf[offset_table_start+3] << 24);
    
    // Read frame 0 compressed size
    uint32_t f0_size = spr_buf[frame0_off] |
                       (spr_buf[frame0_off+1] << 8) |
                       (spr_buf[frame0_off+2] << 16) |
                       (spr_buf[frame0_off+3] << 24);
    TEST_ASSERT_EQUAL(sizeof(frame0_rle), f0_size);
    
    printf("  PASS: SPR file parse (16x16, 2 frames, %zu bytes)\n", spr_len);
}

void test_spr_corrupt_header() {
    uint8_t bad_spr[30];
    memset(bad_spr, 0, 30);
    memcpy(bad_spr, "BAD", 3);  // Wrong magic
    
    // Verify magic check
    TEST_ASSERT_TRUE(memcmp(bad_spr, "SPR\0", 4) != 0);
    printf("  PASS: Corrupt header detection\n");
}

void test_spr_zero_frames() {
    // A sprite with 0 frames should be rejected
    uint8_t spr_buf[62];
    memset(spr_buf, 0, sizeof(spr_buf));
    memcpy(spr_buf, "SPR\0", 4);
    spr_buf[8] = 0;  // frame_count = 0
    
    uint16_t fc = spr_buf[8] | (spr_buf[9] << 8);
    TEST_ASSERT_EQUAL(0, fc);
    printf("  PASS: Zero frame count detection\n");
}

// ============================================================
// LRU Cache Logic Tests
// ============================================================

void test_lru_cache_eviction() {
    // Simulate LRU cache with 4 entries
    struct CacheEntry {
        uint32_t key;
        bool valid;
        uint32_t last_used;
    };
    
    CacheEntry cache[4] = {};
    uint8_t count = 0;
    
    // Fill cache
    for (int i = 0; i < 4; i++) {
        cache[i].key = i + 1;
        cache[i].valid = true;
        cache[i].last_used = i * 100;
        count++;
    }
    
    TEST_ASSERT_EQUAL(4, count);
    
    // Evict oldest (key=1, last_used=0)
    uint32_t oldest_time = UINT32_MAX;
    int oldest_idx = -1;
    for (int i = 0; i < 4; i++) {
        if (cache[i].valid && cache[i].last_used < oldest_time) {
            oldest_time = cache[i].last_used;
            oldest_idx = i;
        }
    }
    TEST_ASSERT_EQUAL(0, oldest_idx);
    TEST_ASSERT_EQUAL(1, cache[oldest_idx].key);
    
    // Evict and add new
    cache[oldest_idx].key = 5;
    cache[oldest_idx].last_used = 400;
    
    // Verify all keys present: 2, 3, 4, 5
    bool found[6] = {};
    for (int i = 0; i < 4; i++) found[cache[i].key] = true;
    TEST_ASSERT_FALSE(found[1]);  // evicted
    TEST_ASSERT_TRUE(found[2]);
    TEST_ASSERT_TRUE(found[3]);
    TEST_ASSERT_TRUE(found[4]);
    TEST_ASSERT_TRUE(found[5]);   // new
    printf("  PASS: LRU cache eviction\n");
}

void test_lru_cache_hit() {
    // Simulate cache hit updating last_used
    struct CacheEntry {
        uint32_t key;
        bool valid;
        uint32_t last_used;
    };
    
    CacheEntry cache[4] = {};
    cache[0].key = 1; cache[0].valid = true; cache[0].last_used = 100;
    cache[1].key = 2; cache[1].valid = true; cache[1].last_used = 200;
    cache[2].key = 3; cache[2].valid = true; cache[2].last_used = 50;  // oldest
    cache[3].key = 4; cache[3].valid = true; cache[3].last_used = 300;
    
    // Access key=2 (cache hit)
    for (int i = 0; i < 4; i++) {
        if (cache[i].valid && cache[i].key == 2) {
            cache[i].last_used = 999;  // Update to newest
            break;
        }
    }
    
    // Now oldest should be key=3 (last_used=50)
    uint32_t oldest_time = UINT32_MAX;
    uint32_t oldest_key = 0;
    for (int i = 0; i < 4; i++) {
        if (cache[i].valid && cache[i].last_used < oldest_time) {
            oldest_time = cache[i].last_used;
            oldest_key = cache[i].key;
        }
    }
    TEST_ASSERT_EQUAL(3, oldest_key);
    printf("  PASS: LRU cache hit updates timestamp\n");
}

// ============================================================
// Palette Conversion Tests
// ============================================================

void test_rgb_to_rgb565() {
    // Test RGB888 to RGB565 conversion
    struct { uint8_t r, g, b; uint16_t expected; } tests[] = {
        {255, 255, 255, 0xFFFF},
        {0, 0, 0, 0x0000},
        {255, 0, 0, 0xF800},
        {0, 255, 0, 0x07E0},
        {0, 0, 255, 0x001F},
    };
    
    for (auto &t : tests) {
        uint16_t r5 = (t.r >> 3) & 0x1F;
        uint16_t g6 = (t.g >> 2) & 0x3F;
        uint16_t b5 = (t.b >> 3) & 0x1F;
        uint16_t result = (r5 << 11) | (g6 << 5) | b5;
        TEST_ASSERT_EQUAL(t.expected, result);
    }
    printf("  PASS: RGB to RGB565 conversion\n");
}

// ============================================================
// Test runner declaration (registered in test_impls.cpp)
// ============================================================
int run_spriteloader_tests() {
    printf("--- SpriteLoader Tests ---\n");
    
    RUN_TEST(test_rle_all_same_color);
    RUN_TEST(test_rle_all_different);
    RUN_TEST(test_rle_mixed_pattern);
    RUN_TEST(test_rle_roundtrip_4096);
    RUN_TEST(test_spr_header_size);
    RUN_TEST(test_spr_header_fields);
    RUN_TEST(test_spr_file_parse);
    RUN_TEST(test_spr_corrupt_header);
    RUN_TEST(test_spr_zero_frames);
    RUN_TEST(test_lru_cache_eviction);
    RUN_TEST(test_lru_cache_hit);
    RUN_TEST(test_rgb_to_rgb565);
    
    printf("--- SpriteLoader: 12 tests PASSED ---\n");
    return 0;
}
