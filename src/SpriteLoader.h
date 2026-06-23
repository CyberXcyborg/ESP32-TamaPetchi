// ============================================================
// SpriteLoader.h — .spr Sprite File Loader for v2.0
// Loads .spr files from LittleFS into PSRAM, LRU frame cache
// ============================================================

#ifndef SPRITE_LOADER_H
#define SPRITE_LOADER_H

#include <Arduino.h>
#include "config_v2.h"

// .spr file header (30 bytes)
struct SprHeader {
    char magic[4];       // "SPR\0"
    uint16_t width;
    uint16_t height;
    uint16_t frame_count;
    uint8_t  bpp;         // bits per pixel (4 = 16 colors)
    uint8_t  flags;       // bit 0: has transparency
    int16_t  transparency_idx; // -1 = none
    uint8_t  reserved[16];
};

// Maximum cache frames in PSRAM
#define SPRITE_CACHE_MAX_FRAMES  8
// Max decoded frame size: 64x64x2 = 8192 bytes (16bpp RGB565)
#define SPRITE_MAX_FRAME_SIZE   (64 * 64 * 2)

// Sprite load error codes
enum SpriteError {
    SPRITE_OK = 0,
    SPRITE_ERR_NOT_FOUND = -1,
    SPRITE_ERR_BAD_MAGIC = -2,
    SPRITE_ERR_CORRUPT = -3,
    SPRITE_ERR_OOM = -4,
    SPRITE_ERR_INVALID_FRAME = -5
};

// Cached frame entry
struct SpriteCacheEntry {
    uint32_t key;           // (sprite_id << 8) | frame_index
    uint16_t *pixel_data;   // PSRAM buffer (RGB565)
    uint32_t last_used;     // millis() of last access
    bool valid;
};

class SpriteLoader {
public:
    static bool begin();
    
    // Load a .spr file and return a sprite handle (0 on error)
    // path: LittleFS path, e.g. "/sprites/baby_idle.spr"
    static int load(const char *path);
    
    // Get decoded frame data for a sprite
    // sprite_id: handle from load()
    // frame_index: 0-based frame number
    // buffer: output buffer (must be at least width*height*2 bytes)
    // buffer_size: size of output buffer
    // Returns: width*height on success, negative on error
    static int getFrame(int sprite_id, uint8_t frame_index,
                        uint16_t *buffer, size_t buffer_size);
    
    // Get frame dimensions for a loaded sprite
    static uint16_t getWidth(int sprite_id);
    static uint16_t getHeight(int sprite_id);
    static uint16_t getFrameCount(int sprite_id);
    
    // Unload a sprite (close file, free cache entries)
    static void unload(int sprite_id);
    
    // Unload all sprites
    static void unloadAll();
    
    // Get last error string
    static const char* getLastError() { return _last_error; }
    
    // Memory stats
    static size_t getCacheUsedBytes() { return _cache_used_bytes; }
    static uint8_t getCacheEntryCount() { return _cache_count; }

private:
    static constexpr uint8_t MAX_SPRITES = 4;
    
    struct SpriteSlot {
        bool in_use;
        char path[32];
        SprHeader header;
        uint32_t file_offset;  // Current file position in LittleFS
        File file;
        uint32_t frame_offsets[16]; // Up to 16 frame offsets
        uint16_t palette[16];  // RGB565 palette
    };
    
    static SpriteSlot _sprites[MAX_SPRITES];
    static SpriteCacheEntry _cache[SPRITE_CACHE_MAX_FRAMES];
    static size_t _cache_used_bytes;
    static uint8_t _cache_count;
    static char _last_error[64];
    
    // Find sprite slot by handle
    static SpriteSlot* findSlot(int sprite_id);
    
    // Find cache entry by key
    static int findCacheEntry(uint32_t key);
    
    // Evict oldest cache entry
    static void evictOldest();
    
    // Decode RLE frame data into RGB565 buffer
    static int decodeFrame(SpriteSlot *slot, uint8_t frame_index,
                           uint16_t *buffer, size_t buffer_size);
    
    // Read file bytes from LittleFS
    static bool readFileBytes(File &file, uint8_t *buf, size_t len);
};

#endif // SPRITE_LOADER_H
