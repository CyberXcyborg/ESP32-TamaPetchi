#!/usr/bin/env python3
"""
png2spr.py — Convert PNG images to TamaPetchi v2.0 .spr sprite format.

Supports:
  - Single-frame: one PNG → one .spr file
  - Multi-frame: directory of PNGs (sorted) → one .spr with multiple frames
  - Auto-quantization: PNGs with >16 colors are reduced via median-cut
  - RLE compression for compact storage

Usage:
  python3 png2spr.py input.png output.spr [--width 64] [--height 64]
  python3 png2spr.py frames_dir/ output.spr [--width 64] [--height 64]
"""

import struct
import sys
import os
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow required. Install: pip3 install Pillow", file=sys.stderr)
    sys.exit(1)

# Constants
SPR_MAGIC = b'SPR\0'
MAX_COLORS = 16
MAX_DIM = 256
MAX_FRAMES = 256


def quantize_image(img: Image.Image, max_colors: int = MAX_COLORS) -> Image.Image:
    """Quantize image to max_colors using median-cut algorithm."""
    if img.mode == 'RGBA':
        # Convert RGBA to RGB with white background, then quantize
        bg = Image.new('RGB', img.size, (255, 255, 255))
        bg.paste(img, mask=img.split()[3])
        img = bg
    elif img.mode != 'RGB':
        img = img.convert('RGB')

    # PIL's quantize uses median-cut by default
    quantized = img.quantize(colors=max_colors, method=Image.Quantize.MEDIANCUT)
    return quantized.convert('P')  # Ensure palette mode


def rgb_to_rgb565(r: int, g: int, b: int) -> int:
    """Convert 8-bit RGB to 16-bit RGB565."""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def rgb565_to_rgb(val: int) -> tuple:
    """Convert 16-bit RGB565 to 8-bit RGB tuple."""
    r = ((val >> 11) & 0x1F) << 3
    g = ((val >> 5) & 0x3F) << 2
    b = (val & 0x1F) << 3
    return (r, g, b)


def extract_palette(img: Image.Image) -> list:
    """Extract palette from PIL palette-mode image as list of RGB565 values."""
    raw_palette = img.getpalette()  # Flat [R,G,B, R,G,B, ...]
    colors = []
    num_colors = min(len(raw_palette) // 3, MAX_COLORS)
    for i in range(num_colors):
        r, g, b = raw_palette[i * 3], raw_palette[i * 3 + 1], raw_palette[i * 3 + 2]
        colors.append(rgb_to_rgb565(r, g, b))
    # Pad to 16 entries
    while len(colors) < MAX_COLORS:
        colors.append(0)
    return colors


def rle_compress(indices: list) -> bytes:
    """
    RLE compress a list of color indices.
    
    Token format:
    - Bit 7 set: repeat. Lower 7 bits = count-1, next byte = color index.
    - Bit 7 clear: literal. Lower 7 bits = count-1, next count bytes = indices.
    """
    result = bytearray()
    n = len(indices)
    i = 0

    while i < n:
        # Check for repeat run
        if i + 1 < n and indices[i] == indices[i + 1]:
            color = indices[i]
            count = 1
            while i + count < n and indices[i + count] == color and count < 128:
                count += 1
            # Emit repeat token: count-1 with bit 7 set, then color
            result.append(0x80 | (count - 1))
            result.append(color)
            i += count
        else:
            # Literal run
            count = 1
            while i + count < n:
                # Stop if we see a repeat of 3+ (worth switching to repeat)
                if (count >= 3 and
                    i + count + 2 < n and
                    indices[i + count] == indices[i + count + 1] == indices[i + count + 2]):
                    break
                if count >= 128:
                    break
                count += 1
            result.append(count - 1)
            result.extend(indices[i:i + count])
            i += count

    return bytes(result)


def process_frame(img: Image.Image, width: int, height: int) -> tuple:
    """
    Process a single image into sprite frame data.
    Returns (palette_list, compressed_bytes, pixel_indices).
    """
    # Resize if needed
    if img.size != (width, height):
        img = img.resize((width, height), Image.Resampling.NEAREST)

    # Quantize to 16 colors
    img = quantize_image(img, MAX_COLORS)

    # Extract palette
    palette = extract_palette(img)

    # Get pixel indices
    pixels = list(img.get_flattened_data()) if hasattr(img, 'get_flattened_data') else list(img.getdata())

    # Compress
    compressed = rle_compress(pixels)

    return palette, compressed, pixels


def build_spr_file(frames_data: list, width: int, height: int) -> bytes:
    """
    Build complete .spr file from frame data.
    frames_data: list of (palette, compressed_bytes) tuples
    """
    num_frames = len(frames_data)

    # Use the first frame's palette as the global palette
    # (all frames share the same palette for simplicity)
    global_palette = frames_data[0][0]

    # Calculate offsets
    header_size = 30
    palette_size = MAX_COLORS * 2  # 32 bytes
    offset_table_size = num_frames * 4
    data_start = header_size + palette_size + offset_table_size

    # Build frame data and offset table
    offset_table = []
    frame_data_blocks = bytearray()
    current_offset = data_start

    for palette, compressed in frames_data:
        offset_table.append(current_offset)
        frame_data_blocks.extend(struct.pack('<I', len(compressed)))
        frame_data_blocks.extend(compressed)
        current_offset += 4 + len(compressed)

    # Build header (30 bytes)
    header = bytearray()
    header += SPR_MAGIC                              # 4 bytes: magic "SPR\0"
    header += struct.pack('<H', width)               # 2 bytes: width
    header += struct.pack('<H', height)              # 2 bytes: height
    header += struct.pack('<H', num_frames)          # 2 bytes: frame count
    header += struct.pack('<B', 4)                   # 1 byte: bpp (4)
    header += struct.pack('<B', 0)                   # 1 byte: flags
    header += struct.pack('<h', -1)                  # 2 bytes: transparency index
    header += b'\x00' * 16                           # 16 bytes: reserved
    assert len(header) == 30, f"Header size {len(header)} != 30"

    # Build palette
    palette_bytes = b''
    for color in global_palette:
        palette_bytes += struct.pack('<H', color)

    # Build offset table
    offset_bytes = b''
    for offset in offset_table:
        offset_bytes += struct.pack('<I', offset)

    return bytes(header) + palette_bytes + offset_bytes + frame_data_blocks


def verify_spr(spr_data: bytes, expected_pixels: list, width: int, height: int) -> bool:
    """Verify .spr file by decompressing first frame and comparing pixels."""
    HEADER_SIZE = 30
    PALETTE_SIZE = MAX_COLORS * 2  # 32

    # Parse header
    magic = spr_data[0:4]
    if magic != SPR_MAGIC:
        print("  VERIFY FAIL: bad magic", file=sys.stderr)
        return False

    w, h, nframes = struct.unpack_from('<HHH', spr_data, 4)
    if w != width or h != height:
        print(f"  VERIFY FAIL: size mismatch {w}x{h} vs {width}x{height}", file=sys.stderr)
        return False

    # Read first frame offset from offset table
    offset_table_offset = HEADER_SIZE + PALETTE_SIZE
    frame0_offset = struct.unpack_from('<I', spr_data, offset_table_offset)[0]

    # Read frame compressed size and data
    frame_compressed_size = struct.unpack_from('<I', spr_data, frame0_offset)[0]
    frame_data = spr_data[frame0_offset + 4:frame0_offset + 4 + frame_compressed_size]

    # Decompress RLE
    pixels = []
    i = 0
    while i < len(frame_data) and len(pixels) < width * height:
        token = frame_data[i]
        i += 1
        if token & 0x80:
            # Repeat
            count = (token & 0x7F) + 1
            color = frame_data[i]
            i += 1
            pixels.extend([color] * count)
        else:
            # Literal
            count = token + 1
            pixels.extend(frame_data[i:i + count])
            i += count

    pixels = pixels[:width * height]

    # Compare
    if pixels == expected_pixels[:width * height]:
        print(f"  VERIFY OK: {len(pixels)} pixels match")
        return True
    else:
        mismatches = sum(1 for a, b in zip(pixels, expected_pixels) if a != b)
        print(f"  VERIFY FAIL: {mismatches} mismatches out of {len(pixels)}", file=sys.stderr)
        return False


def collect_inputs(input_path: str) -> list:
    """Collect and return sorted list of PNG file paths."""
    p = Path(input_path)
    if p.is_file():
        return [str(p)]
    elif p.is_dir():
        files = sorted(p.glob('*.png')) + sorted(p.glob('*.PNG'))
        if not files:
            print(f"ERROR: No PNG files in {input_path}", file=sys.stderr)
            sys.exit(1)
        return [str(f) for f in files]
    else:
        print(f"ERROR: {input_path} not found", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description='Convert PNG to TamaPetchi .spr format')
    parser.add_argument('input', help='Input PNG file or directory of PNGs')
    parser.add_argument('output', help='Output .spr file')
    parser.add_argument('--width', type=int, default=64, help='Sprite width (default: 64)')
    parser.add_argument('--height', type=int, default=64, help='Sprite height (default: 64)')
    parser.add_argument('--no-verify', action='store_true', help='Skip verification')
    args = parser.parse_args()

    if args.width < 1 or args.width > MAX_DIM or args.height < 1 or args.height > MAX_DIM:
        print(f"ERROR: dimensions must be 1-{MAX_DIM}", file=sys.stderr)
        sys.exit(1)

    # Collect input files
    input_files = collect_inputs(args.input)
    print(f"Converting {len(input_files)} frame(s) at {args.width}x{args.height}")

    # Process frames
    frames_data = []
    all_pixels = []
    for f in input_files:
        img = Image.open(f)
        palette, compressed, pixels = process_frame(img, args.width, args.height)
        frames_data.append((palette, compressed))
        all_pixels.append(pixels)
        ratio = len(compressed) / (args.width * args.height) * 100
        print(f"  {os.path.basename(f)}: {len(compressed)} bytes ({ratio:.0f}% of raw)")

    # Build .spr file
    spr_data = build_spr_file(frames_data, args.width, args.height)

    # Verify
    if not args.no_verify and len(all_pixels) > 0:
        print("Verifying...")
        if not verify_spr(spr_data, all_pixels[0], args.width, args.height):
            print("ERROR: Verification failed, not writing output", file=sys.stderr)
            sys.exit(1)

    # Write output
    os.makedirs(os.path.dirname(args.output) or '.', exist_ok=True)
    with open(args.output, 'wb') as f:
        f.write(spr_data)

    print(f"Written: {args.output} ({len(spr_data)} bytes)")


if __name__ == '__main__':
    main()
