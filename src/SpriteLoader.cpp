// ============================================================
// SpriteLoader.cpp — .spr Sprite File Loader Implementation
// ============================================================

#include "SpriteLoader.h"
#include "Storage_v2.h"
#include <LittleFS.h>
#include <esp_heap_caps.h>

// Static members
SpriteLoader::SpriteSlot SpriteLoader::_sprites[SpriteLoader::MAX_SPRITES];
SpriteCacheEntry SpriteLoader::_cache[SPRITE_CACHE_MAX_FRAMES];
size_t SpriteLoader::_cache_used_bytes = 0;
uint8_t SpriteLoader::_cache_count = 0;
char SpriteLoader::_last_error[64] = {0};

bool SpriteLoader::begin() {
    memset(_sprites, 0, sizeof(_sprites));
    memset(_cache, 0, sizeof(_cache));
    _cache_used_bytes = 0;
    _cache_count = 0;
    _last_error[0] = '\0';
    return true;
}

int SpriteLoader::load(const char *path) {
    // Find free slot
    int slot_idx = -1;
    for (int i = 0; i < MAX_SPRITES; i++) {
        if (!_sprites[i].in_use) {
            slot_idx = i;
            break;
        }
    }
    
    if (slot_idx < 0) {
        strncpy(_last_error, "No free sprite slots", sizeof(_last_error));
        return 0;  // 0 = invalid handle
    }
    
    // Check file exists
    if (!StorageV2::exists(path)) {
        snprintf(_last_error, sizeof(_last_error), "File not found: %s", path);
        return 0;
    }
    
    // Open file
    File file = LittleFS.open(path, "r");
    if (!file) {
        snprintf(_last_error, sizeof(_last_error), "Cannot open: %s", path);
        return 0;
    }
    
    // Read and validate header
    uint8_t header_buf[30];
    if (file.read(header_buf, 30) != 30) {
        file.close();
        strncpy(_last_error, "File too short for header", sizeof(_last_error));
        return 0;
    }
    
    // Parse header
    SprHeader *hdr = (SprHeader*)header_buf;
    if (memcmp(hdr->magic, "SPR\0", 4) != 0) {
        file.close();
        strncpy(_last_error, "Bad SPR magic", sizeof(_last_error));
        return 0;
    }
    
    if (hdr->bpp != 4) {
        file.close();
        snprintf(_last_error, sizeof(_last_error), "Unsupported bpp: %d", hdr->bpp);
        return 0;
    }
    
    if (hdr->width == 0 || hdr->width > 256 || hdr->height == 0 || hdr->height > 256) {
        file.close();
        snprintf(_last_error, sizeof(_last_error), "Bad dimensions: %dx%d",
                 hdr->width, hdr->height);
        return 0;
    }
    
    if (hdr->frame_count == 0 || hdr->frame_count > 16) {
        file.close();
        snprintf(_last_error, sizeof(_last_error), "Bad frame count: %d", hdr->frame_count);
        return 0;
    }
    
    // Read palette (16 x RGB565 = 32 bytes)
    uint8_t palette_buf[32];
    if (file.read(palette_buf, 32) != 32) {
        file.close();
        strncpy(_last_error, "Cannot read palette", sizeof(_last_error));
        return 0;
    }
    
    SpriteSlot *slot = &_sprites[slot_idx];
    slot->in_use = true;
    strncpy(slot->path, path, sizeof(slot->path) - 1);
    memcpy(&slot->header, hdr, sizeof(SprHeader));
    
    // Store palette
    for (int i = 0; i < 16; i++) {
        slot->palette[i] = palette_buf[i * 2] | (palette_buf[i * 2 + 1] << 8);
    }
    
    // Read frame offset table
    uint8_t offset_buf[64];  // Max 16 frames x 4 bytes
    size_t offset_bytes = hdr->frame_count * 4;
    if (file.read(offset_buf, offset_bytes) != offset_bytes) {
        slot->in_use = false;
        file.close();
        strncpy(_last_error, "Cannot read offset table", sizeof(_last_error));
        return 0;
    }
    
    for (uint16_t i = 0; i < hdr->frame_count; i++) {
        slot->frame_offsets[i] = offset_buf[i * 4] |
                                 (offset_buf[i * 4 + 1] << 8) |
                                 (offset_buf[i * 4 + 2] << 16) |
                                 (offset_buf[i * 4 + 3] << 24);
    }
    
    // Keep file open for frame reading
    slot->file = file;
    
    DEBUG_PRINTF("[Sprite] Loaded %s: %dx%d, %d frames (slot %d)\n",
                 path, hdr->width, hdr->height, hdr->frame_count, slot_idx);
    
    // Return handle (slot_idx + 1, so 0 can mean "invalid")
    return slot_idx + 1;
}

int SpriteLoader::getFrame(int sprite_id, uint8_t frame_index,
                            uint16_t *buffer, size_t buffer_size) {
    SpriteSlot *slot = findSlot(sprite_id);
    if (!slot) {
        strncpy(_last_error, "Invalid sprite handle", sizeof(_last_error));
        return SPRITE_ERR_INVALID_FRAME;
    }
    
    if (frame_index >= slot->header.frame_count) {
        snprintf(_last_error, sizeof(_last_error), "Frame %d >= count %d",
                 frame_index, slot->header.frame_count);
        return SPRITE_ERR_INVALID_FRAME;
    }
    
    uint32_t key = ((uint32_t)sprite_id << 8) | frame_index;
    
    // Check cache first
    int cache_idx = findCacheEntry(key);
    if (cache_idx >= 0) {
        SpriteCacheEntry *entry = &_cache[cache_idx];
        size_t frame_bytes = slot->header.width * slot->header.height * 2;
        if (buffer_size >= frame_bytes) {
            memcpy(buffer, entry->pixel_data, frame_bytes);
            entry->last_used = millis();
            return slot->header.width * slot->header.height;
        } else {
            strncpy(_last_error, "Buffer too small", sizeof(_last_error));
            return SPRITE_ERR_OOM;
        }
    }
    
    // Decode from file
    int result = decodeFrame(slot, frame_index, buffer, buffer_size);
    if (result <= 0) return result;
    
    // Cache the decoded frame
    size_t frame_bytes = slot->header.width * slot->header.height * 2;
    
    // Evict if needed
    while (_cache_count >= SPRITE_CACHE_MAX_FRAMES) {
        evictOldest();
    }
    
    // Allocate PSRAM buffer for cache
    uint16_t *cache_buf = (uint16_t*)heap_caps_malloc(frame_bytes, MALLOC_CAP_SPIRAM);
    if (cache_buf) {
        memcpy(cache_buf, buffer, frame_bytes);
        
        // Find free cache entry
        for (int i = 0; i < SPRITE_CACHE_MAX_FRAMES; i++) {
            if (!_cache[i].valid) {
                _cache[i].key = key;
                _cache[i].pixel_data = cache_buf;
                _cache[i].last_used = millis();
                _cache[i].valid = true;
                _cache_count++;
                _cache_used_bytes += frame_bytes;
                break;
            }
        }
    }
    
    return result;
}

uint16_t SpriteLoader::getWidth(int sprite_id) {
    SpriteSlot *slot = findSlot(sprite_id);
    return slot ? slot->header.width : 0;
}

uint16_t SpriteLoader::getHeight(int sprite_id) {
    SpriteSlot *slot = findSlot(sprite_id);
    return slot ? slot->header.height : 0;
}

uint16_t SpriteLoader::getFrameCount(int sprite_id) {
    SpriteSlot *slot = findSlot(sprite_id);
    return slot ? slot->header.frame_count : 0;
}

void SpriteLoader::unload(int sprite_id) {
    SpriteSlot *slot = findSlot(sprite_id);
    if (!slot) return;
    
    // Free cache entries for this sprite
    for (int i = 0; i < SPRITE_CACHE_MAX_FRAMES; i++) {
        if (_cache[i].valid && (_cache[i].key >> 8) == (uint32_t)sprite_id) {
            if (_cache[i].pixel_data) {
                free(_cache[i].pixel_data);
                _cache_used_bytes -= slot->header.width * slot->header.height * 2;
            }
            _cache[i].valid = false;
            _cache_count--;
        }
    }
    
    // Close file
    if (slot->file) {
        slot->file.close();
    }
    
    slot->in_use = false;
    DEBUG_PRINTF("[Sprite] Unloaded slot %d\n", sprite_id - 1);
}

void SpriteLoader::unloadAll() {
    for (int i = 0; i < MAX_SPRITES; i++) {
        if (_sprites[i].in_use) {
            unload(i + 1);
        }
    }
}

// --- Private methods ---

SpriteLoader::SpriteSlot* SpriteLoader::findSlot(int sprite_id) {
    int idx = sprite_id - 1;
    if (idx < 0 || idx >= MAX_SPRITES || !_sprites[idx].in_use) {
        return nullptr;
    }
    return &_sprites[idx];
}

int SpriteLoader::findCacheEntry(uint32_t key) {
    for (int i = 0; i < SPRITE_CACHE_MAX_FRAMES; i++) {
        if (_cache[i].valid && _cache[i].key == key) {
            return i;
        }
    }
    return -1;
}

void SpriteLoader::evictOldest() {
    uint32_t oldest_time = UINT32_MAX;
    int oldest_idx = -1;
    
    for (int i = 0; i < SPRITE_CACHE_MAX_FRAMES; i++) {
        if (_cache[i].valid && _cache[i].last_used < oldest_time) {
            oldest_time = _cache[i].last_used;
            oldest_idx = i;
        }
    }
    
    if (oldest_idx >= 0) {
        if (_cache[oldest_idx].pixel_data) {
            free(_cache[oldest_idx].pixel_data);
        }
        _cache[oldest_idx].valid = false;
        _cache_count--;
    }
}

int SpriteLoader::decodeFrame(SpriteSlot *slot, uint8_t frame_index,
                               uint16_t *buffer, size_t buffer_size) {
    if (!slot || !slot->file) return SPRITE_ERR_NOT_FOUND;
    
    uint16_t w = slot->header.width;
    uint16_t h = slot->header.height;
    size_t pixel_count = (size_t)w * h;
    size_t frame_bytes = pixel_count * 2;
    
    if (buffer_size < frame_bytes) {
        strncpy(_last_error, "Buffer too small for frame", sizeof(_last_error));
        return SPRITE_ERR_OOM;
    }
    
    // Seek to frame offset
    uint32_t frame_offset = slot->frame_offsets[frame_index];
    if (!slot->file.seek(frame_offset)) {
        snprintf(_last_error, sizeof(_last_error), "Seek to 0x%lx failed", frame_offset);
        return SPRITE_ERR_CORRUPT;
    }
    
    // Read compressed size
    uint8_t size_buf[4];
    if (slot->file.read(size_buf, 4) != 4) {
        strncpy(_last_error, "Cannot read frame size", sizeof(_last_error));
        return SPRITE_ERR_CORRUPT;
    }
    
    uint32_t compressed_size = size_buf[0] | (size_buf[1] << 8) |
                               (size_buf[2] << 16) | (size_buf[3] << 24);
    
    if (compressed_size == 0 || compressed_size > pixel_count * 2) {
        snprintf(_last_error, sizeof(_last_error), "Bad compressed size: %lu", compressed_size);
        return SPRITE_ERR_CORRUPT;
    }
    
    // Read compressed data
    uint8_t *compressed = (uint8_t*)malloc(compressed_size);
    if (!compressed) {
        strncpy(_last_error, "OOM for compressed data", sizeof(_last_error));
        return SPRITE_ERR_OOM;
    }
    
    if (slot->file.read(compressed, compressed_size) != compressed_size) {
        free(compressed);
        strncpy(_last_error, "Cannot read compressed data", sizeof(_last_error));
        return SPRITE_ERR_CORRUPT;
    }
    
    // Decompress RLE into color indices
    uint8_t *indices = (uint8_t*)malloc(pixel_count);
    if (!indices) {
        free(compressed);
        strncpy(_last_error, "OOM for index buffer", sizeof(_last_error));
        return SPRITE_ERR_OOM;
    }
    
    size_t src_pos = 0;
    size_t dst_pos = 0;
    
    while (src_pos < compressed_size && dst_pos < pixel_count) {
        uint8_t token = compressed[src_pos++];
        
        if (token & 0x80) {
            // Repeat: count = (token & 0x7F) + 1, next byte = color index
            uint8_t count = (token & 0x7F) + 1;
            uint8_t color = compressed[src_pos++];
            size_t fill = count;
            if (dst_pos + fill > pixel_count) fill = pixel_count - dst_pos;
            memset(indices + dst_pos, color, fill);
            dst_pos += fill;
        } else {
            // Literal: count = token + 1, next count bytes = color indices
            uint8_t count = token + 1;
            size_t copy = count;
            if (src_pos + copy > compressed_size) copy = compressed_size - src_pos;
            if (dst_pos + copy > pixel_count) copy = pixel_count - dst_pos;
            memcpy(indices + dst_pos, compressed + src_pos, copy);
            src_pos += count;  // Advance by full count (may read past, but copy is clamped)
            dst_pos += copy;
        }
    }
    
    free(compressed);
    
    // Convert color indices to RGB565 using palette
    for (size_t i = 0; i < pixel_count; i++) {
        uint8_t idx = indices[i] & 0x0F;  // Clamp to 16 colors
        buffer[i] = slot->palette[idx];
    }
    
    free(indices);
    
    return pixel_count;
}

bool SpriteLoader::readFileBytes(File &file, uint8_t *buf, size_t len) {
    return file.read(buf, len) == len;
}
