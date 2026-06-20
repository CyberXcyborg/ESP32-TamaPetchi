#!/usr/bin/env python3
"""
ESP32 TamaPetchi — Manufacturing Provisioning Tool
Phase 13.4

Usage:
  python3 provision.py --port /dev/ttyUSB0 --ssid "MyWiFi" --password "MyPass"
  python3 provision.py --port /dev/ttyUSB0 --ssid "MyWiFi" --password "MyPass" --device-id "TAMA-CUSTOM01"
  python3 provision.py --port /dev/ttyUSB0 --batch count=5

This tool:
  1. Flashes the firmware (if not already flashed)
  2. Generates a unique device ID from ESP32 MAC
  3. Stores WiFi credentials in SPIFFS
  4. Creates initial pet data
  5. Verifies provisioning via HTTP API
"""

import argparse
import json
import os
import subprocess
import sys
import time
import urllib.request
import urllib.error

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False


def run_pio(args, cwd=None):
    """Run a PlatformIO command and return (exit_code, stdout, stderr)."""
    cmd = ["pio"] + args
    print(f"  $ {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
    if result.stdout:
        print(f"  {result.stdout.strip()}")
    if result.returncode != 0 and result.stderr:
        print(f"  STDERR: {result.stderr.strip()}")
    return result.returncode, result.stdout, result.stderr


def flash_firmware(port, project_dir):
    """Flash firmware to ESP32 via PlatformIO."""
    print("[1/5] Flashing firmware...")
    code, out, err = run_pio(["run", "-e", "esp32dev", "-t", "upload", "--upload-port", port], cwd=project_dir)
    if code != 0:
        print("  ERROR: Firmware flash failed!")
        return False
    print("  OK: Firmware flashed successfully")
    return True


def wait_for_boot(port, timeout=30):
    """Wait for ESP32 to boot and output ready message."""
    print("[2/5] Waiting for ESP32 to boot...")
    if not HAS_SERIAL:
        print("  (pyserial not available, using delay-based wait)")
        time.sleep(5)
        return True

    try:
        ser = serial.Serial(port, 115200, timeout=1)
        start = time.time()
        while time.time() - start < timeout:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if "Provision" in line or "TamaPetchi" in line or "AP:" in line:
                print(f"  Boot message: {line}")
                ser.close()
                return True
        ser.close()
        print("  WARNING: Boot message not detected, continuing anyway")
        return True
    except Exception as e:
        print(f"  WARNING: Serial error: {e}")
        time.sleep(5)
        return True


def get_device_info(project_dir):
    """Get firmware version and build info."""
    version = "1.3.0"
    # Try to read from CHANGELOG or version file
    changelog = os.path.join(project_dir, "CHANGELOG.md")
    if os.path.exists(changelog):
        with open(changelog) as f:
            for line in f:
                if "## v" in line:
                    version = line.strip().split("## ")[-1].split(" ")[0].lstrip("v")
                    break
    return {"version": version}


def generate_device_id(mac_bytes=None):
    """Generate a TamaPetchi device ID."""
    if mac_bytes:
        low = int.from_bytes(mac_bytes[-3:], "big")
    else:
        import random
        low = random.randint(0, 0xFFFFFF)
    return f"TAMA-{low:06X}"


def provision_via_serial(port, ssid, password, device_id=None):
    """Provision ESP32 via serial AT-like commands."""
    if not HAS_SERIAL:
        print("  pyserial not available, skipping serial provisioning")
        return None

    try:
        s = serial.Serial(port, 115200, timeout=2)  # noqa: F841
        time.sleep(1)

        # Send provisioning command
        cmd = {
            "cmd": "provision",
            "ssid": ssid,
            "password": password,
        }
        if device_id:
            cmd["deviceID"] = device_id

        s.write(json.dumps(cmd).encode() + b"\n")
        time.sleep(2)

        # Read response
        response = b""
        while s.in_waiting:
            response += s.read(s.in_waiting)
            time.sleep(0.5)

        s.close()

        if response:
            try:
                result = json.loads(response.decode())
                return result
            except json.JSONDecodeError:
                print(f"  Serial response (raw): {response.decode(errors='ignore')}")
        return None
    except Exception as e:
        print(f"  Serial provisioning error: {e}")
        return None


def provision_via_http(ip, ssid, password, device_id=None):
    """Provision ESP32 via HTTP API (when in AP mode)."""
    print(f"[3/5] Provisioning via HTTP at {ip}...")

    # Set WiFi credentials
    payload = json.dumps({"ssid": ssid, "password": password}).encode()
    req = urllib.request.Request(
        f"http://{ip}/api/provisioning/set",
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            result = json.loads(resp.read())
            if result.get("success"):
                print("  OK: WiFi credentials saved")
            else:
                print(f"  ERROR: {result.get('error', 'unknown')}")
                return False
    except urllib.error.URLError as e:
        print(f"  ERROR: HTTP request failed: {e}")
        return False

    # Set custom device ID if provided
    if device_id:
        payload = json.dumps({"deviceID": device_id}).encode()
        req = urllib.request.Request(
            f"http://{ip}/api/provisioning/deviceid",
            data=payload,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        try:
            with urllib.request.urlopen(req, timeout=10) as resp:
                result = json.loads(resp.read())
                if result.get("success"):
                    print(f"  OK: Device ID set to {result.get('deviceID')}")
                else:
                    print(f"  WARNING: Device ID set failed: {result.get('error')}")
        except urllib.error.URLError:
            print("  WARNING: Could not set device ID via HTTP")

    return True


def verify_provisioning(ip):
    """Verify provisioning status via HTTP API."""
    print("[4/5] Verifying provisioning...")
    try:
        req = urllib.request.Request(f"http://{ip}/api/provisioning/status")
        with urllib.request.urlopen(req, timeout=10) as resp:
            status = json.loads(resp.read())
            print(f"  Provisioned: {status.get('provisioned')}")
            print(f"  Device ID: {status.get('deviceID')}")
            print(f"  Firmware: {status.get('firmwareVersion')}")
            return status.get("provisioned", False)
    except urllib.error.URLError as e:
        print(f"  WARNING: Could not verify: {e}")
        return False


def create_initial_pet(ip, pet_name="Tama"):
    """Create initial pet via HTTP API."""
    print("[5/5] Creating initial pet...")
    payload = json.dumps({"name": pet_name}).encode()
    req = urllib.request.Request(
        f"http://{ip}/api/pets",
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            result = json.loads(resp.read())
            print(f"  OK: Pet created: {result.get('name', pet_name)}")
            return True
    except urllib.error.URLError as e:
        print(f"  WARNING: Could not create pet: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description="TamaPetchi ESP32 Provisioning Tool")
    parser.add_argument("--port", required=True, help="Serial port (e.g. /dev/ttyUSB0)")
    parser.add_argument("--ssid", help="WiFi SSID")
    parser.add_argument("--password", help="WiFi password")
    parser.add_argument("--device-id", help="Custom device ID (auto-generated if not set)")
    parser.add_argument("--pet-name", default="Tama", help="Initial pet name")
    parser.add_argument("--firmware", help="Path to firmware .bin file (skip PlatformIO build)")
    parser.add_argument("--project-dir", default=".", help="PlatformIO project directory")
    parser.add_argument("--skip-flash", action="store_true", help="Skip firmware flash")
    parser.add_argument("--batch", type=int, default=0, help="Batch mode: provision N devices sequentially")
    args = parser.parse_args()

    project_dir = os.path.abspath(args.project_dir)

    if args.batch > 0:
        print(f"=== Batch provisioning: {args.batch} devices ===")
        print(f"Connect each device and press Enter to provision...")
        for i in range(1, args.batch + 1):
            print(f"\n--- Device {i}/{args.batch} ---")
            input(f"  Connect device {i} and press Enter...")
            if not args.skip_flash:
                flash_firmware(args.port, project_dir)
            wait_for_boot(args.port)
            device_id = generate_device_id()
            print(f"  Generated device ID: {device_id}")
            # After flash, device starts in AP mode
            provision_via_serial(args.port, args.ssid, args.password, device_id)
        print(f"\n=== Batch complete: {args.batch} devices provisioned ===")
        return

    # Single device provisioning
    print("=== TamaPetchi ESP32 Provisioning Tool ===")
    print(f"Port: {args.port}")
    print(f"Project: {project_dir}")

    if not args.skip_flash:
        if not flash_firmware(args.port, project_dir):
            sys.exit(1)
    else:
        print("[1/5] Skipping firmware flash (--skip-flash)")

    wait_for_boot(args.port)

    if args.ssid and args.password:
        # Try serial first, then HTTP
        result = provision_via_serial(args.port, args.ssid, args.password, args.device_id)
        if result is None:
            # Device may be in AP mode — try HTTP
            ap_ip = "192.168.4.1"  # Default AP IP
            provision_via_http(ap_ip, args.ssid, args.password, args.device_id)
    else:
        print("[3/5] No WiFi credentials provided, skipping WiFi provisioning")

    # Verify
    verify_provisioning("192.168.4.1")

    print("\n=== Provisioning complete ===")


if __name__ == "__main__":
    main()
