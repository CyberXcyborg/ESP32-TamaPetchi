# ESP32-TamaPetchi Hardware Wiring Diagram

## Components Required

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32 DevKit V1 | 1 | 30-pin dev board |
| SSD1306 OLED 128x64 | 1 | I2C, 0.96" (optional) |
| Passive Buzzer | 1 | 3.3V compatible |
| RGB LED (common cathode) | 1 | Or RGB LED module |
| 220Ω Resistor | 3 | For RGB LED channels |
| 10kΩ Resistor | 1 | For battery voltage divider |
| Push Button | 1 | Or use GPIO 0 BOOT button |
| LiPo Battery (3.7V) | 1 | With JST connector (optional) |
| 10kΩ Resistor | 1 | For voltage divider (battery) |

## Wiring Table

| ESP32 Pin | Component | Wire Color | Notes |
|------------|-----------|------------|-------|
| 3V3 | OLED VCC | Red | 3.3V power |
| GND | OLED GND | Black | Common ground |
| GPIO 21 (SDA) | OLED SDA | Blue | I2C data |
| GPIO 22 (SCL) | OLED SCL | Yellow | I2C clock |
| GPIO 25 | Buzzer (+) | Orange | PWM buzzer signal |
| GND | Buzzer (-) | Black | |
| GPIO 14 | RGB LED R | Red | Via 220Ω resistor |
| GPIO 12 | RGB LED G | Green | Via 220Ω resistor |
| GPIO 13 | RGB LED B | Blue | Via 220Ω resistor |
| GND | RGB LED COM | Black | Common cathode |
| GPIO 0 | BOOT Button | White | Internal pull-up, active low |
| GPIO 35 | Battery ADC | Purple | Via voltage divider (10kΩ/10kΩ) |
| 3V3 | Button pull-up | Red | Internal pull-up used |

## ASCII Wiring Diagram

```
                    ESP32 DevKit V1
                   ┌──────────────────┐
                   │                  │
    OLED SDA ─────┤ GPIO21      3V3  ├──── OLED VCC
    OLED SCL ─────┤ GPIO22      GND  ├──── OLED GND
                   │                  │
    Buzzer (+) ───┤ GPIO25      GND  ├──── Buzzer (-)
                   │                  │
    RGB LED R ────┤ GPIO14      3V3  ├──── 3V3 Rail
    RGB LED G ────┤ GPIO12      GND  ├──── GND Rail
    RGB LED B ────┤ GPIO13      GPIO0 ├─── BOOT Button ─── GND
                   │                  │
    Battery ADC ──┤ GPIO35      VIN  ├──── 5V (USB)
                   │                  │
                   └──────────────────┘

    Battery Voltage Divider (for GPIO 35 ADC):
    ┌─────────┐     10kΩ      ┌─────────┐     10kΩ      ┌─────────┐
    │ Battery ├──────/\/\/────┤ GPIO35  ├──────/\/\/────┤  GND    │
    │ 4.2V max│               │  ADC    │               │         │
    └─────────┘               └─────────┘               └─────────┘
    (Voltage at GPIO35 = Battery_V / 2, max 2.1V → safe for 3.3V ADC)

    RGB LED (Common Cathode):
    ┌──────────────────────────────────────┐
    │  RGB LED Module                      │
    │  R ──── 220Ω ──── GPIO14            │
    │  G ──── 220Ω ──── GPIO12            │
    │  B ──── 220Ω ──── GPIO13            │
    │  COM ──────────── GND               │
    └──────────────────────────────────────┘

    Buzzer:
    ┌──────────────────────────────────────┐
    │  Passive Buzzer                      │
    │  (+) ──── GPIO25                    │
    │  (-) ──── GND                      │
    └──────────────────────────────────────┘
```

## Notes

1. **OLED is optional** — enable with `-DENABLE_OLED` in build flags
2. **RGB LED is optional** — can be disabled with `#define DISABLE_RGB_LED` in config.h
3. **Buzzer is optional** — can be disabled with `#define DISABLE_MUSIC` in config.h
4. **Buttons** — GPIO 0 has a built-in BOOT button on most ESP32 dev boards
5. **Battery monitoring** — GPIO 35 is ADC1_CH7, suitable for battery voltage reading
6. **Deep sleep wake** — GPIO 0 can wake the ESP32 from deep sleep (ext0 wakeup)
