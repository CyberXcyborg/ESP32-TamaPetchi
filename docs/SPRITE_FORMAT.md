# TamaPetchi v2.0 — Sprite File Format (.spr)

## Overview

The `.spr` sprite format is a compact, palette-based image format designed for
ESP32-S3 + LVGL rendering. It uses 4-bit palette indexing (16 colors) with
RLE compression for efficient storage in LittleFS and fast decompression into
PSRAM framebuffers.

## File Layout

```
┌─────────────────────────────────────┐
│ Header (32 bytes)                   │
├─────────────────────────────────────┤
│ Palette (16 × 2 bytes = 32 bytes)  │
├─────────────────────────────────────┤
│ Frame Offset Table (N × 4 bytes)   │
├─────────────────────────────────────┤
│ Frame 0 Data (RLE compressed)       │
├─────────────────────────────────────┤
│ Frame 1 Data (RLE compressed)       │
├─────────────────────────────────────┤
│ ...                                 │
├─────────────────────────────────────┤
│ Frame N-1 Data (RLE compressed)     │
└─────────────────────────────────────┘
```

## Header (32 bytes)

| Offset | Size | Type   | Value / Description                    |
|--------|------|--------|----------------------------------------|
| 0      | 4    | char[4]| Magic: `SPR\0`                         |
| 4      | 2    | uint16 | Width in pixels (max 256)              |
| 6      | 2    | uint16 | Height in pixels (max 256)             |
| 8      | 2    | uint16 | Frame count (max 256)                  |
| 10     | 1    | uint8  | Bits per pixel (4 for 16-color palette)|
| 11     | 1    | uint8  | Flags (bit 0: has transparency)        |
| 12     | 2    | uint16 | Transparency color index (0-15, or -1) |
| 14     | 18   | —      | Reserved (zero-filled)                 |

## Palette (32 bytes)

16 entries × 2 bytes each, RGB565 format (little-endian).
Color index 0 is the first palette entry.

## Frame Offset Table (N × 4 bytes)

Array of `uint32_t` offsets from the start of the file to each frame's
compressed data. Used for random access to individual frames.

## Frame Data (RLE Compressed)

Each frame is compressed independently using a simple RLE scheme:

### RLE Encoding

Each RLE token is 1 byte:

- **Bit 7 set (0x80–0xFF):** Repeat token. Lower 7 bits = repeat count - 1.
  The next byte is the color index to repeat.
  Example: `0x83 0x05` → color index 5 repeated 5 times (count=4+1).

- **Bit 7 clear (0x00–0x7F):** Literal run. Lower 7 bits = count - 1.
  The next `count` bytes are literal color indices.
  Example: `0x03 0x01 0x02 0x03 0x04` → 4 literal pixels.

### Decompression

Decompress into a flat array of uint8_t color indices (one per pixel).
Total pixels = width × height.

## Memory Budget

For a 64×64 sprite with 8 frames:
- Uncompressed: 64 × 64 × 8 = 32,768 bytes (color indices)
- With RLE (typical ~40% compression): ~19,660 bytes
- Total file: 32 (header) + 32 (palette) + 32 (offset table) + ~19,660 ≈ **20KB**

Decoded frame buffer (16bpp RGB565 for LVGL): 64 × 64 × 2 = 8,192 bytes in PSRAM.

## LVGL Integration

LVGL image decoder registers with `lv_img_decoder_set_*_cb` callbacks.
Source path format: `S:/sprites/baby_idle.spr#3` (frame 3 of baby_idle.spr).

## Conversion Tool

Use `tools/png2spr.py` to convert PNG images to `.spr` format:
- Input: PNG with ≤16 colors (auto-quantized if needed)
- Output: `.spr` binary file
- Usage: `python3 tools/png2spr.py input.png output.spr [--width 64] [--height 64]`
