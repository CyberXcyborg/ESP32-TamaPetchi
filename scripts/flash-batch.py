#!/usr/bin/env python3
"""
Batch flash script for ESP32-TamaPetchi.
Flashes multiple ESP32 devices sequentially using PlatformIO.

Usage:
    python3 scripts/flash-batch.py [--port PORT] [--all] [--test]

Requirements:
    - PlatformIO CLI installed
    - ESP32 devices connected via USB

Examples:
    # Flash all detected ESP32 devices
    python3 scripts/flash-batch.py --all

    # Flash specific port
    python3 scripts/flash-batch.py --port /dev/ttyUSB0

    # Flash and run tests
    python3 scripts/flash-batch.py --all --test
"""

import argparse
import glob
import os
import subprocess
import sys
import time

# Default ports to try (Linux/Mac)
DEFAULT_PORT_GLOBS = [
    "/dev/ttyUSB*",
    "/dev/ttyACM*",
    "/dev/cu.usbserial*",
    "/dev/cu.SLAB_USBtoUART*",
]

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def detect_ports():
    """Detect available serial ports."""
    ports = []
    for pattern in DEFAULT_PORT_GLOBS:
        ports.extend(glob.glob(pattern))
    return sorted(set(ports))


def flash_device(port=None, upload_fs=True):
    """Flash a single ESP32 device."""
    env_flag = ""
    if port:
        env_flag = f"--environment esp32dev"

    print(f"\n{'='*60}")
    print(f"Flashing {'on ' + port if port else 'default port'}...")
    print(f"{'='*60}\n")

    os.chdir(REPO_ROOT)

    # Build and upload firmware
    cmd = ["pio", "run", "-e", "esp32dev", "--target", "upload"]
    if port:
        cmd.extend(["--upload-port", port])

    result = subprocess.run(cmd, cwd=REPO_ROOT)
    if result.returncode != 0:
        print(f"ERROR: Firmware upload failed (exit code {result.returncode})")
        return False

    # Upload SPIFFS filesystem
    if upload_fs:
        print("\nUploading SPIFFS filesystem...")
        cmd_fs = ["pio", "run", "-e", "esp32dev", "--target", "uploadfs"]
        if port:
            cmd_fs.extend(["--upload-port", port])
        result = subprocess.run(cmd_fs, cwd=REPO_ROOT)
        if result.returncode != 0:
            print(f"ERROR: SPIFFS upload failed (exit code {result.returncode})")
            return False

    print(f"\nSUCCESS: Device flashed successfully!")
    return True


def run_tests():
    """Run native unit tests."""
    print(f"\n{'='*60}")
    print("Running native unit tests...")
    print(f"{'='*60}\n")

    os.chdir(REPO_ROOT)
    result = subprocess.run(["pio", "test", "-e", "native"], cwd=REPO_ROOT)
    return result.returncode == 0


def main():
    parser = argparse.ArgumentParser(description="Batch flash ESP32-TamaPetchi")
    parser.add_argument("--port", help="Specific serial port to flash")
    parser.add_argument("--all", action="store_true", help="Flash all detected ports")
    parser.add_argument("--test", action="store_true", help="Run tests after flashing")
    parser.add_argument("--no-fs", action="store_true", help="Skip SPIFFS upload")
    args = parser.parse_args()

    success_count = 0
    fail_count = 0

    if args.port:
        # Flash specific port
        if flash_device(args.port, upload_fs=not args.no_fs):
            success_count += 1
        else:
            fail_count += 1
    elif args.all:
        ports = detect_ports()
        if not ports:
            print("No serial ports detected. Connect an ESP32 and try again.")
            print(f"Searched: {', '.join(DEFAULT_PORT_GLOBS)}")
            sys.exit(1)

        print(f"Detected {len(ports)} port(s): {', '.join(ports)}")
        for i, port in enumerate(ports):
            print(f"\n>>> Device {i+1}/{len(ports)}: {port}")
            if flash_device(port, upload_fs=not args.no_fs):
                success_count += 1
            else:
                fail_count += 1
            if i < len(ports) - 1:
                print("\nWaiting 3 seconds before next device...")
                time.sleep(3)
    else:
        # Flash default (no port specified)
        if flash_device(upload_fs=not args.no_fs):
            success_count += 1
        else:
            fail_count += 1

    # Run tests if requested
    if args.test:
        if run_tests():
            print("\nAll tests PASSED!")
        else:
            print("\nWARNING: Some tests failed!")
            fail_count += 1

    # Summary
    print(f"\n{'='*60}")
    print(f"SUMMARY: {success_count} succeeded, {fail_count} failed")
    print(f"{'='*60}")

    sys.exit(0 if fail_count == 0 else 1)


if __name__ == "__main__":
    main()
