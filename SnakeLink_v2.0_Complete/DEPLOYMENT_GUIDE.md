# SnakeLink v2.0 — Deployment Guide

## Pre-Deployment Checklist

### Hardware Requirements (Per Station)
| Item | Qty | Spec | Source |
|------|-----|------|--------|
| ESP32 DevKit v1 | 1 | 30-pin, WROOM-32 | Any electronics supplier |
| Ra-02 LoRa module | 1 | SX1278, 433MHz | AI-Thinker |
| BPW34 photodiode | 1 | PIN photodiode | Vishay |
| Laser diode | 1 | 650nm, 5mW, 3V | 5V TTL module or discrete |
| CA3140E op-amp | 1 | BiMOS | Intersil |
| LM358 op-amp | 1 | Dual | TI/ST |
| LM393 comparator | 1 | Dual | TI/ST |
| 2N7000 MOSFET | 1 | N-channel | Fairchild |
| SSD1306 OLED | 1 | 128x64, I2C | Generic |
| MT3608 boost module | 1 | 2A, adjustable | Generic |
| TP4056 charger module | 1 | 1A, with protection | Generic |
| 18650 cells | 2 | 3500mAh, protected | Samsung/LG |
| 18650 holder | 1 | 2x parallel | Generic |
| Resistors | Assorted | 100Ω, 1kΩ, 2.2kΩ, 100kΩ, 10kΩ | 1/4W |
| Capacitors | Assorted | 22pF, 100nF, 10µF | Ceramic/Electrolytic |
| Breadboard + jumpers | 1 | 830 tie-point | Generic |
| Laptop | 1 | Windows 10/11 or Linux | Any |

### Software Requirements
- PlatformIO Core (CLI) or PlatformIO IDE (VS Code)
- Python 3.10+
- pip package manager

---

## Step 1: Hardware Assembly

### 1.1 Power Section (Build First)
```
[18650+] ──► [TP4056 B+] ──► [MT3608 IN+] ──► [5.0V Rail]
[18650−] ──► [TP4056 B−] ──► [MT3608 IN−] ──► [GND Rail]
```

**CRITICAL RULE**: Master switch OFF before charging via USB-C.

**Voltage Check**: Use multimeter to verify 5.0V ± 0.05V at MT3608 output before connecting ESP32.

### 1.2 ESP32 Power Connections
```
MT3608 OUT+ ──► ESP32 VIN
MT3608 OUT− ──► ESP32 GND
ESP32 3V3   ──► LoRa VCC, OLED VCC, pull-ups
ESP32 5V    ──► Laser anode, Op-amp VCC
```

### 1.3 FSO Receiver Chain
```
BPW34 cathode ──► +5V
BPW34 anode   ──► CA3140E IN− (Pin 2)
CA3140E OUT   ──► AC coupling cap ──► LM393 IN+ (Pin 3)
LM393 IN−     ──► 100kΩ trimpot wiper (threshold)
LM393 OUT     ──► 2.2kΩ pull-up to 3.3V ──► ESP32 GPIO 16
Separate tap  ──► 10kΩ/10kΩ divider ──► ESP32 GPIO 34 (ADC)
```

### 1.4 FSO Transmitter
```
ESP32 GPIO 17 ──► 1kΩ ──► 2N7000 Gate
2N7000 Drain  ──► Laser cathode ──► 100Ω ──► +5V
2N7000 Source ──► GND
100kΩ         ──► Gate to GND (pull-down)
2.2kΩ         ──► Drain to +5V (simmer bypass)
```

### 1.5 LoRa Module
```
ESP32 GPIO 18 ──► LoRa SCK
ESP32 GPIO 19 ──► LoRa MISO
ESP32 GPIO 23 ──► LoRa MOSI
ESP32 GPIO 5  ──► LoRa NSS
ESP32 GPIO 4  ──► LoRa DIO0
ESP32 GPIO 14 ──► LoRa RST
ESP32 3V3     ──► LoRa VCC
ESP32 GND     ──► LoRa GND
```

**WARNING**: Ra-02 is 3.3V ONLY. Connecting to 5V will destroy the module.

### 1.6 OLED Display
```
ESP32 GPIO 21 ──► OLED SDA
ESP32 GPIO 22 ──► OLED SCL
ESP32 3V3     ──► OLED VCC
ESP32 GND     ──► OLED GND
```

### 1.7 Buttons & Feedback
```
ESP32 GPIO 32 ──► Button 1 (Align) ──► GND (internal pull-up)
ESP32 GPIO 13 ──► Button 2 (Mode) ──► GND (internal pull-up)
ESP32 GPIO 25 ──► Buzzer (+)
ESP32 GPIO 26 ──► Green LED (PWM)
ESP32 GPIO 27 ──► Blue LED (PWM)
ESP32 GPIO 33 ──► Red LED (PWM)
```

### 1.8 Battery Monitor
```
18650+ ──► 100kΩ ──► ESP32 GPIO 35 ──► 100kΩ ──► GND
```

---

## Step 2: Firmware Flashing

### 2.1 Install PlatformIO
```bash
# Linux/macOS
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py

# Windows
# Download and install PlatformIO IDE for VS Code
```

### 2.2 Build & Upload
```bash
cd firmware
pio run --target upload
pio device monitor --baud 115200
```

### 2.3 First Boot Sequence
On first boot, the device will:
1. Initialize NVS (Non-Volatile Storage)
2. Generate random device ID and encryption key
3. Display boot sequence on OLED
4. Create Wi-Fi AP: `SnakeLink-XXXX`
5. Show key fingerprint on OLED

### 2.4 Pair Both Devices
```bash
# On Device A (after boot)
# Note the displayed key fingerprint (2 hex chars)

# On Device B (after boot)
# Verify fingerprint matches Device A
# If mismatch, reset config and re-pair
```

---

## Step 3: Software Installation

### 3.1 Python Environment
```bash
cd software
python -m venv venv

# Linux/macOS
source venv/bin/activate

# Windows
venv\Scripts\activate

pip install -r requirements.txt
```

### 3.2 Run Application
```bash
python main.py
```

### 3.3 Initial Configuration
1. Connect laptop to ESP32 Wi-Fi AP (`SnakeLink-XXXX`)
2. Open SnakeLink software
3. Enter transceiver IP: `192.168.4.1`
4. Verify connection status shows "TRANSCEIVER CONNECTED"
5. In Settings, enable High Security Mode if needed
6. Generate and exchange encryption keys
7. Verify key fingerprint matches OLED display

### 3.4 Build Standalone Executable
```bash
pip install pyinstaller
pyinstaller --onefile --windowed --add-data "assets;assets" main.py
```

---

## Step 4: Field Deployment

### 4.1 Pre-Flight Checklist
- [ ] Battery voltage > 3.7V (OLED shows >50%)
- [ ] All three links show signal on dashboard
- [ ] Wi-Fi RSSI > -75 dBm
- [ ] FSO alignment tone responds when button pressed
- [ ] LoRa heartbeat received every 5 seconds
- [ ] Encryption key fingerprint matches on both devices
- [ ] Master switch ON, USB disconnected

### 4.2 Alignment Procedure
1. Press and hold Button 1 (GPIO 32) for 1.5s
2. Enter alignment mode (buzzer pitch = signal strength)
3. Adjust laser aim until buzzer reaches highest pitch
4. OLED shows alignment percentage
5. Press Button 1 briefly to exit alignment mode

### 4.3 Normal Operation
1. System defaults to Wi-Fi link (highest bandwidth)
2. FSM automatically monitors signal quality
3. If Wi-Fi degrades, system enters WARNING state
4. Packets duplicated on Wi-Fi + FSO for 3 seconds
5. Switch completes to FSO
6. If both fail, falls back to LoRa (emergency mode)

### 4.4 Emergency Broadcast
1. Long-press Button 2 (GPIO 13) for 1.5s
2. Alarm sounds on both devices
3. GPS coordinates broadcast on ALL links
4. "EMERGENCY" message displayed on OLED and UI
5. 5-second cooldown prevents accidental re-trigger

---

## Step 5: Maintenance

### 5.1 Battery Care
- Recharge when OLED shows <25%
- Master switch OFF before charging
- Use only protected 18650 cells
- Expected runtime: 12-18 hours continuous

### 5.2 Firmware Updates
```bash
# Via USB (local)
pio run --target upload

# Via Wi-Fi OTA (prepared, not yet implemented)
# Future: Upload .bin via web interface
```

### 5.3 Log Retrieval
```bash
# Read SPIFFS logs from ESP32
pio run --target uploadfs
# Or via serial: send CMD packet type 0x0E with subcommand
```

---

## Step 6: Troubleshooting Quick Reference

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No OLED display | I2C wiring | Check GPIO 21/22, pull-ups |
| Laser always on | GPIO 17 stuck high | Check 100kΩ pull-down |
| No LoRa TX | Missing antenna | Attach SMA antenna |
| Brownout on Wi-Fi | Power rail < 5V | Check MT3608 output |
| False link switches | Noisy ADC | Increase EMA alpha or stability window |
| Buzzer silent | PWM channel conflict | Check LEDC setup |
| Button not working | Wrong GPIO | Verify Corrected Guide v2 pinout |

---

*Deployment Guide v2.0 — April 2026*
