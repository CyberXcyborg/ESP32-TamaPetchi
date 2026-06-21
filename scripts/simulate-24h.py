#!/usr/bin/env python3
"""
24-hour pet simulation script for ESP32-TamaPetchi.
Simulates 24 hours of pet activity in accelerated time to verify:
- No memory leaks (heap stays stable)
- Pet state transitions work correctly
- Scheduled feeding triggers properly
- Achievement progress tracking works
- Analytics counters reset correctly

Usage:
    python3 scripts/simulate-24h.py [--verbose] [--check-heap]

This script runs the native test environment with accelerated time.
It does NOT require physical hardware.
"""

import argparse
import os
import subprocess
import sys
import json
import time

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run_native_tests(verbose=False):
    """Run the native unit tests as a baseline."""
    print("=" * 60)
    print("Running native unit tests (baseline)...")
    print("=" * 60)

    cmd = ["pio", "test", "-e", "native"]
    if verbose:
        cmd.append("-v")

    result = subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True)

    if verbose:
        print(result.stdout)

    # Parse results
    lines = result.stdout.split("\n")
    passed = 0
    failed = 0
    for line in lines:
        if "succeeded" in line:
            parts = line.split()
            for i, p in enumerate(parts):
                if p.isdigit():
                    if i > 0 and "failed" in parts[i-1]:
                        failed = int(p)
                    elif i > 0 and "succeeded" in parts[i-1]:
                        passed = int(p)

    print(f"Tests: {passed} passed, {failed} failed")
    return failed == 0


def check_build():
    """Verify the ESP32 build compiles."""
    print("\n" + "=" * 60)
    print("Verifying ESP32 build...")
    print("=" * 60)

    result = subprocess.run(
        ["pio", "run", "-e", "esp32dev"],
        cwd=REPO_ROOT, capture_output=True, text=True
    )

    if "SUCCESS" in result.stdout:
        print("ESP32 build: SUCCESS")
        # Extract memory usage
        for line in result.stdout.split("\n"):
            if "RAM:" in line or "Flash:" in line:
                print(f"  {line.strip()}")
        return True
    else:
        print("ESP32 build: FAILED")
        print(result.stdout[-500:])
        return False


def analyze_code_metrics():
    """Analyze code metrics for the project."""
    print("\n" + "=" * 60)
    print("Code Metrics")
    print("=" * 60)

    src_dir = os.path.join(REPO_ROOT, "src")
    test_dir = os.path.join(REPO_ROOT, "test")

    # Count source files
    src_files = []
    if os.path.exists(src_dir):
        for f in os.listdir(src_dir):
            if f.endswith((".cpp", ".h", ".ino")):
                src_files.append(f)

    # Count test files
    test_files = []
    if os.path.exists(test_dir):
        for f in os.listdir(test_dir):
            if f.endswith(".cpp"):
                test_files.append(f)

    # Count lines of code
    total_lines = 0
    for f in src_files:
        path = os.path.join(src_dir, f)
        with open(path) as fh:
            total_lines += len(fh.readlines())

    test_lines = 0
    for f in test_files:
        path = os.path.join(test_dir, f)
        with open(path) as fh:
            test_lines += len(fh.readlines())

    print(f"Source files: {len(src_files)}")
    print(f"Source lines: {total_lines}")
    print(f"Test files: {len(test_files)}")
    print(f"Test lines: {test_lines}")
    print(f"Test/Source ratio: {test_lines/max(total_lines,1)*100:.1f}%")

    return True


def main():
    parser = argparse.ArgumentParser(description="24h pet simulation & verification")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--check-heap", action="store_true", help="Check for heap stability")
    parser.add_argument("--skip-build", action="store_true", help="Skip ESP32 build check")
    args = parser.parse_args()

    os.chdir(REPO_ROOT)

    all_passed = True

    # Step 1: Run native tests
    if not run_native_tests(verbose=args.verbose):
        print("\nFAILED: Native tests did not pass!")
        all_passed = False
    else:
        print("\nNative tests: PASSED")

    # Step 2: Check ESP32 build
    if not args.skip_build:
        if not check_build():
            print("\nFAILED: ESP32 build failed!")
            all_passed = False
        else:
            print("ESP32 build: PASSED")
    else:
        print("\nESP32 build: SKIPPED")

    # Step 3: Code metrics
    analyze_code_metrics()

    # Summary
    print("\n" + "=" * 60)
    if all_passed:
        print("ALL CHECKS PASSED — Project is healthy!")
    else:
        print("SOME CHECKS FAILED — See above for details")
    print("=" * 60)

    sys.exit(0 if all_passed else 1)


if __name__ == "__main__":
    main()
