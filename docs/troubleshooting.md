# Troubleshooting Guide — ESP32 TamaPetchi

## Common Issues

### Device Won't Boot
- **Symptom**: No serial output, OLED blank
- **Check**: USB cable supports data (not charge-only)
- **Fix**: Hold BOOT button while connecting USB, reflash firmware

### WiFi Won't Connect
- **Symptom**: AP mode starts automatically
- **Check**: WiFi credentials in config.h or WiFi Manager portal
- **Fix**: Connect to "TamaPetchi-Setup" AP, navigate to 192.168.4.1

### Web UI Not Loading
- **Symptom**: Browser shows "connection refused"
- **Check**: Device IP in serial monitor
- **Fix**: Ensure phone/computer on same WiFi network

### Pet Stats Not Updating
- **Symptom**: Stats frozen on web UI
- **Check**: WebSocket connection status (green dot in header)
- **Fix**: Refresh page, check device serial output for errors

### OTA Update Fails
- **Symptom**: "Update failed" message
- **Check**: Flash space (must have headroom for 2x firmware)
- **Fix**: Disable unused features, use delta OTA

### Buzzer Not Working
- **Symptom**: No sound from device
- **Check**: Buzzer pin (GPIO 25), soundEnabled flag
- **Fix**: Toggle sound in web UI or send `POST /mute`

### OLED Display Garbled
- **Symptom**: Random pixels, wrong text
- **Check**: I2C wiring (SDA=GPIO21, SCL=GPIO22)
- **Fix**: Add 4.7kΩ pull-up resistors on SDA/SCL

## FAQ

**Q: How do I factory reset?**
A: Hold BOOT button for 10 seconds while device is powered on.

**Q: Can I run without OLED?**
A: Yes, OLED is optional. Add `-DDISABLE_OLED` to build flags.

**Q: How many pets can I have?**
A: Up to 3 pets stored in SPIFFS. Switch via web UI or IR remote.

**Q: Does it work with 5GHz WiFi?**
A: No, ESP32 only supports 2.4GHz WiFi.

**Q: How much power does it use?**
A: ~80mA active, ~10μA deep sleep. Runs ~24h on 1000mAh battery.

## Performance Benchmarks

| Metric | Value |
|--------|-------|
| Boot time | ~2.1s |
| API response time (local) | ~15ms |
| WebSocket latency | ~5ms |
| Memory usage (24h) | Stable at 18.9% RAM |
| Flash usage | 83.6% |
| Test coverage | 162/162 pass |
