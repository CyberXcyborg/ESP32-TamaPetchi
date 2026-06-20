#!/bin/bash
# ESP32 TamaPetchi — Batch Flash Script
# Phase 13.4: Manufacturing & Provisioning Tools
#
# Usage:
#   ./batch_flash.sh firmware.bin /dev/ttyUSB0
#   ./batch_flash.sh firmware.bin /dev/ttyUSB0 count=5
#   ./batch_flash.sh firmware.bin /dev/ttyUSB0 delay=3
#
# This script flashes multiple ESP32 devices sequentially.
# After each flash, it waits for the device to boot and prints
# the device ID (from serial output) for labeling.

set -euo pipefail

FIRMWARE="${1:-}"
PORT="${2:-}"
COUNT="${3:-1}"
DELAY="${4:-5}"

if [ -z "$FIRMWARE" ]; then
    echo "Usage: $0 <firmware.bin> <serial_port> [count=1] [delay_seconds=5]"
    echo ""
    echo "Example:"
    echo "  $0 .pio/build/esp32dev/firmware.bin /dev/ttyUSB0"
    echo "  $0 .pio/build/esp32dev/firmware.bin /dev/ttyUSB0 5 3"
    exit 1
fi

if [ ! -f "$FIRMWARE" ]; then
    echo "ERROR: Firmware file not found: $FIRMWARE"
    exit 1
fi

# Parse named args
for arg in "$@"; do
    case "$arg" in
        count=*) COUNT="${arg#*=}" ;;
        delay=*) DELAY="${arg#*=}" ;;
    esac
done

echo "========================================="
echo "  TamaPetchi Batch Flash"
echo "========================================="
echo "Firmware: $FIRMWARE"
echo "Port:     $PORT"
echo "Count:    $COUNT"
echo "Delay:    ${DELAY}s between devices"
echo "========================================="

SUCCESS=0
FAILED=0

for i in $(seq 1 "$COUNT"); do
    echo ""
    echo "--- Device $i/$COUNT ---"
    echo "Connect device and press Enter..."
    read -r

    echo "Flashing device $i..."

    if [ -n "$PORT" ]; then
        esptool.py --port "$PORT" write_flash 0x10000 "$FIRMWARE"
    else
        esptool.py write_flash 0x10000 "$FIRMWARE"
    fi

    if [ $? -eq 0 ]; then
        echo "  ✓ Device $i flashed successfully"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "  ✗ Device $i flash FAILED"
        FAILED=$((FAILED + 1))
    fi

    # Wait for device to boot
    echo "  Waiting ${DELAY}s for device to boot..."
    sleep "$DELAY"

    # Try to read device ID from serial
    if [ -n "$PORT" ] && command -v esptool.py &>/dev/null; then
        echo "  Reading device info..."
        timeout 3 esptool.py --port "$PORT" read_flash 0x3ff0000 0x1000 2>/dev/null || true
    fi

    if [ $i -lt "$COUNT" ]; then
        echo "  Ready for next device..."
    fi
done

echo ""
echo "========================================="
echo "  Batch Flash Complete"
echo "========================================="
echo "  Success: $SUCCESS"
echo "  Failed:  $FAILED"
echo "  Total:   $COUNT"
echo "========================================="

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
