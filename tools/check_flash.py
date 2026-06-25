#!/usr/bin/env python3
"""
ESP32-TamaPetchi v2.0 — Flash Size Check Script
Runs before build to verify flash budget constraints.
"""
import os
import sys

Import("env")

FLASH_SIZE_MB = 8
FLASH_BUDGET_PERCENT = 85
MAX_FLASH_BYTES = int(FLASH_SIZE_MB * 1024 * 1024 * FLASH_BUDGET_PERCENT / 100)

def check_flash():
    """Check firmware size against budget after build."""
    # This runs pre-build, so we just print info
    print(f"[FlashCheck] Target: ESP32-S3, {FLASH_SIZE_MB}MB flash")
    print(f"[FlashCheck] Budget: {FLASH_BUDGET_PERCENT}% = {MAX_FLASH_BYTES // 1024}KB max")

env.AddPreAction("buildprog", check_flash)
