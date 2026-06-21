# TamaPetchi Tools (Phase 16.3)

## tamapetchi-cli.py

Python CLI for managing TamaPetchi ESP32 devices.

### Installation

```bash
pip install requests zeroconf
```

### Commands

| Command   | Description                          |
|-----------|--------------------------------------|
| discover  | Find TamaPetchi devices on network   |
| status    | Show current pet status              |
| backup    | Download backup from device          |
| restore   | Upload backup to device              |
| action    | Perform action (feed/play/clean/...) |
| flash     | Build and flash firmware             |
| simulate  | Run native test simulation           |
| export    | Export data as JSON or CSV           |

### Examples

```bash
# Discover devices
python3 tools/tamapetchi-cli.py discover

# Check pet status
python3 tools/tamapetchi-cli.py status --host 192.168.1.50

# Feed the pet
python3 tools/tamapetchi-cli.py action feed --host 192.168.1.50

# Download backup
python3 tools/tamapetchi-cli.py backup --host 192.168.1.50 --file my-pet.json

# Flash firmware
python3 tools/tamapetchi-cli.py flash --port /dev/ttyUSB0

# Export analytics
python3 tools/tamapetchi-cli.py export --format csv --file stats.csv
```
