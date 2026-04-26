# SnakeLink v2.0 — Tri-Mode Tactical Communication System

## Overview
SnakeLink is a tri-mode hybrid communication system integrating Free-Space Optical (FSO), Wi-Fi (ESP-NOW), and LoRa technologies for disaster response and defense operations in Sri Lanka.

## Architecture
- **Primary Link**: FSO (Free-Space Optical) — 650nm laser, 115200 baud UART
- **Backup Link**: Wi-Fi (ESP-NOW peer-to-peer) — ~500 kbps
- **Emergency Link**: LoRa (433 MHz) — 0.3-5 kbps, constant telemetry
- **Firmware**: ESP32 dual-core FreeRTOS (C++) with FSM-First switching
- **Software**: PyQt6 desktop application (Python) with tactical dark theme
- **Protocol**: Custom H-Link v2.0 with AES-256-GCM encryption

## Hardware (Corrected Guide v2)
- ESP32 DevKit v1 (30-pin)
- Ra-02 LoRa module (SX1278, 433MHz)
- BPW34 photodiode + CA3140E TIA
- LM393 comparator (corrected pinout)
- 2N7000 MOSFET laser driver
- SSD1306 OLED (128x64)
- 2x 18650 Li-ion cells (parallel)
- MT3608 boost converter (5.0V)

## Firmware Build
```bash
cd firmware
pio run --target upload
pio device monitor
```

## Software Build
```bash
cd software
pip install -r requirements.txt
python main.py
```

## Link Priority (FSO-First)
| Priority | Link | Role | Trigger |
|----------|------|------|---------|
| **1st** | **FSO (Laser)** | **PRIMARY** | Default — highest speed, stealth, no RF signature |
| **2nd** | **Wi-Fi** | **BACKUP** | When fog/rain/misalignment disrupts laser |
| **3rd** | **LoRa** | **EMERGENCY** | When both FSO and Wi-Fi fail |

## Features
- ✅ Make-Before-Break zero-packet-loss switching
- ✅ Deterministic FSM with hysteresis
- ✅ Hardware-software co-design (ADC envelope monitoring)
- ✅ AES-256-GCM end-to-end encryption
- ✅ Chunked, compressed, resumable file transfer
- ✅ Push-to-Talk voice (Opus/Codec2)
- ✅ Offline OpenStreetMap support
- ✅ Emergency broadcast on all links
- ✅ Real-time telemetry dashboard

## File Structure
```
RESEARCH-PROJECT-UOK/
├── firmware/
│   ├── platformio.ini
│   ├── partitions.csv
│   ├── include/
│   │   ├── config.h
│   │   ├── hlink_protocol.h
│   │   ├── fsm_manager.h
│   │   ├── link_manager.h
│   │   ├── wifi_link.h
│   │   ├── fso_link.h
│   │   ├── lora_link.h
│   │   ├── telemetry_manager.h
│   │   ├── display_manager.h
│   │   ├── feedback_manager.h
│   │   ├── battery_manager.h
│   │   ├── button_manager.h
│   │   ├── config_manager.h
│   │   └── crypto_manager.h
│   └── src/
│       ├── main.cpp
│       ├── hlink_protocol.cpp
│       ├── fsm_manager.cpp
│       ├── link_manager.cpp
│       ├── wifi_link.cpp
│       ├── fso_link.cpp
│       ├── lora_link.cpp
│       ├── telemetry_manager.cpp
│       ├── display_manager.cpp
│       ├── feedback_manager.cpp
│       ├── battery_manager.cpp
│       ├── button_manager.cpp
│       ├── config_manager.cpp
│       └── crypto_manager.cpp
├── software/
│   ├── main.py
│   ├── config.py
│   ├── udp_client.py
│   ├── hlink_protocol.py
│   ├── crypto_manager.py
│   ├── requirements.txt
│   └── ui/
│       ├── main_window.py
│       ├── chat_widget.py
│       ├── voice_widget.py
│       ├── files_widget.py
│       ├── map_widget.py
│       ├── dashboard_widget.py
│       └── settings_widget.py
└── SYSTEM_ANALYSIS.md
```

## Safety Notes
- Master switch OFF before charging via USB-C
- Never look directly into laser (5mW retinal hazard)
- Ra-02 is 3.3V ONLY — never connect to 5V
- 100Ω current limit resistor is mandatory for laser
- Star ground rule: all GND returns meet at battery negative

## License
Research Project — University of Kelaniya, Sri Lanka
