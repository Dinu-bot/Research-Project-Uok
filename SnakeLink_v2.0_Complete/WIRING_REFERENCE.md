# SnakeLink v2.0 — Quick Wiring Reference

## Breadboard Layout (Top View)

```
    ┌─────────────────────────────────────────────────────────────────┐
    │  POWER RAIL (+5V)          │  POWER RAIL (GND)                 │
    │  ━━━━━━━━━━━━━━━━━━━━━━━   │  ━━━━━━━━━━━━━━━━━━━━━━━          │
    │  │ MT3608 OUT+ ─────────┼──┼──► ESP32 VIN                      │
    │  │ MT3608 OUT+ ─────────┼──┼──► Laser anode (via 100Ω)         │
    │  │ MT3608 OUT+ ─────────┼──┼──► CA3140E V+                     │
    │  │ MT3608 OUT+ ─────────┼──┼──► LM358 V+                       │
    │  │ MT3608 OUT+ ─────────┼──┼──► LM393 V+                       │
    │  │                        │  │                                  │
    │  │ TP4056 OUT+ ─────────┼──┼──► MT3608 IN+                     │
    │  │ TP4056 OUT− ─────────┼──┼──► MT3608 IN−                     │
    │  │                        │  │                                  │
    │  │ 18650+ ──────────────┼──┼──► TP4056 B+                      │
    │  │ 18650− ──────────────┼──┼──► TP4056 B−                      │
    │  │                        │  │                                  │
    │  │ 100kΩ ───────────────┼──┼──► ESP32 GPIO 35 ── 100kΩ ── GND │
    │  │                        │  │    (Battery divider)             │
    └─────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────┐
    │  ESP32 DEVKIT v1 (30-pin)                                       │
    │                                                                 │
    │  3V3 ──► LoRa VCC, OLED VCC, pull-ups                          │
    │  GND ──► Common ground (star point)                             │
    │  VIN ──► MT3608 OUT+ (5.0V)                                     │
    │                                                                 │
    │  GPIO 16 ──► FSO RX (from LM393 OUT, via 2.2kΩ pull-up 3.3V)   │
    │  GPIO 17 ──► FSO TX (to 2N7000 Gate, via 1kΩ)                   │
    │  GPIO 34 ──► FSO ADC (envelope, via 10kΩ/10kΩ divider)          │
    │                                                                 │
    │  GPIO 18 ──► LoRa SCK                                           │
    │  GPIO 19 ──► LoRa MISO                                          │
    │  GPIO 23 ──► LoRa MOSI                                          │
    │  GPIO 5  ──► LoRa NSS                                           │
    │  GPIO 4  ──► LoRa DIO0                                          │
    │  GPIO 14 ──► LoRa RST                                           │
    │                                                                 │
    │  GPIO 21 ──► OLED SDA (with 4.7kΩ pull-up 3.3V)                │
    │  GPIO 22 ──► OLED SCL (with 4.7kΩ pull-up 3.3V)                │
    │                                                                 │
    │  GPIO 25 ──► Buzzer (+)                                         │
    │  GPIO 26 ──► Green LED (PWM)                                    │
    │  GPIO 27 ──► Blue LED (PWM)                                     │
    │  GPIO 33 ──► Red LED (PWM)                                      │
    │                                                                 │
    │  GPIO 32 ──► Button 1 (Align) ──► GND (internal pull-up)       │
    │  GPIO 13 ──► Button 2 (Mode) ──► GND (internal pull-up)        │
    │                                                                 │
    │  GPIO 35 ──► Battery ADC (via 100kΩ/100kΩ divider)              │
    └─────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────┐
    │  FSO RECEIVER CHAIN                                             │
    │                                                                 │
    │  +5V ──► BPW34 cathode                                          │
    │  BPW34 anode ──► CA3140E IN− (Pin 2)                            │
    │  CA3140E OUT (Pin 6) ──► 100kΩ feedback ──► IN− (Pin 2)        │
    │  CA3140E OUT ──► 22pF ──► GND (stability)                       │
    │  CA3140E OUT ──► 10µF AC coupling ──► LM393 IN+ (Pin 3)         │
    │  LM393 IN− (Pin 2) ──► 100kΩ trimpot wiper                      │
    │  LM393 OUT (Pin 1) ──► 2.2kΩ pull-up ──► 3.3V ──► GPIO 16      │
    │                                                                 │
    │  Separate tap from CA3140E OUT:                                 │
    │  ├──► 10µF AC coupling ──► 10kΩ ──► GPIO 34                     │
    │  └──► 10kΩ ──► GND (bias to 1.65V)                              │
    │                                                                 │
    │  LM358:                                                         │
    │  Pin 3 (+IN) ──► 10kΩ ──► +5V                                   │
    │  Pin 3 (+IN) ──► 10kΩ ──► GND  (2.5V VGND)                     │
    │  Pin 2 (−IN) ──► 10kΩ ──► VGND (buffer config)                  │
    │  Pin 1 (OUT) ──► 10kΩ ──► Pin 2 (feedback)                      │
    │  Pin 1 (OUT) ──► CA3140E V− (Pin 4)                             │
    │  Pin 7 (OUT2) ──► LM393 V− (Pin 4)                              │
    └─────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────┐
    │  FSO TRANSMITTER                                                │
    │                                                                 │
    │  GPIO 17 ──► 1kΩ ──► 2N7000 Gate (Pin 2)                        │
    │  2N7000 Gate ──► 100kΩ ──► GND (pull-down)                      │
    │  2N7000 Drain (Pin 3) ──► Laser cathode                         │
    │  2N7000 Source (Pin 1) ──► GND                                  │
    │  +5V ──► 100Ω ──► Laser anode                                   │
    │  +5V ──► 2.2kΩ ──► 2N7000 Drain (simmer bypass)                 │
    └─────────────────────────────────────────────────────────────────┘
```

## Pin Quick Reference Table

| GPIO | Function | Direction | Voltage | Notes |
|------|----------|-----------|---------|-------|
| 16 | FSO RX | Input | 3.3V | From LM393, 2.2kΩ pull-up |
| 17 | FSO TX | Output | 3.3V | To 2N7000 Gate |
| 34 | FSO ADC | Input | 0-3.3V | 1.65V idle, AC coupled |
| 35 | Battery ADC | Input | 0-2.1V | 2:1 divider |
| 32 | Button 1 | Input | 3.3V | Active-low, internal pull-up |
| 13 | Button 2 | Input | 3.3V | Active-low, internal pull-up |
| 25 | Buzzer | Output | 3.3V PWM | Active buzzer |
| 26 | Green LED | Output | 3.3V PWM | Power/health |
| 27 | Blue LED | Output | 3.3V PWM | Active link |
| 33 | Red LED | Output | 3.3V PWM | Alert/data |
| 18 | LoRa SCK | Output | 3.3V | SPI clock |
| 19 | LoRa MISO | Input | 3.3V | SPI data in |
| 23 | LoRa MOSI | Output | 3.3V | SPI data out |
| 5 | LoRa NSS | Output | 3.3V | SPI chip select |
| 4 | LoRa DIO0 | Input | 3.3V | Packet ready interrupt |
| 14 | LoRa RST | Output | 3.3V | Module reset |
| 21 | OLED SDA | I/O | 3.3V | I2C data, 4.7kΩ pull-up |
| 22 | OLED SCL | Output | 3.3V | I2C clock, 4.7kΩ pull-up |

## Critical Component Values

| Component | Value | Tolerance | Purpose |
|-----------|-------|-----------|---------|
| Laser current limit | 100Ω | 5% | 28mA @ 5V |
| Gate series resistor | 1kΩ | 5% | Current limit |
| Gate pull-down | 100kΩ | 5% | Ensure OFF at boot |
| Simmer bypass | 2.2kΩ | 5% | 1.3mA pre-bias |
| TIA feedback | 100kΩ | 1% | Transimpedance gain |
| TIA stability | 22pF | 10% | Prevent oscillation |
| Comparator pull-up | 2.2kΩ | 5% | Open-collector output |
| ADC bias divider | 10kΩ/10kΩ | 1% | 1.65V bias |
| Battery divider | 100kΩ/100kΩ | 1% | 2:1 ratio |
| I2C pull-ups | 4.7kΩ | 5% | SDA/SCL |

## Power Budget (Estimated)

| Component | Current | Notes |
|-----------|---------|-------|
| ESP32 (idle) | 80mA | Wi-Fi sleep |
| ESP32 (Wi-Fi TX) | 250mA | Peak burst |
| LoRa (RX) | 10mA | Receive mode |
| LoRa (TX @ 20dBm) | 120mA | Peak |
| OLED | 15mA | All pixels on |
| Laser (modulated) | 28mA | Average |
| Op-amps | 5mA | Total |
| LEDs (all on) | 30mA | PWM max |
| Buzzer | 20mA | Active |
| **Total average** | **~200mA** | Normal operation |
| **Total peak** | **~500mA** | Wi-Fi + LoRa TX + Laser |

**Runtime**: 7000mAh / 200mA = **~35 hours** (theoretical)  
**Realistic**: **12-18 hours** (accounting for peaks, conversion losses)

---

*Wiring Reference v2.0 — April 2026*
