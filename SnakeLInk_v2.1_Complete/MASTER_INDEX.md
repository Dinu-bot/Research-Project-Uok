# SnakeLink v2.0 — Master Index

## Welcome
This is the complete production-ready transformation of the SnakeLink tactical communication system. All files are organized into three categories: **Documentation**, **Firmware**, and **Software**.

---

## 📋 Documentation (9 files)

| File | Purpose | Read First? |
|------|---------|-------------|
| [QUICK_START.md](QUICK_START.md) | 5-minute setup guide | ✅ YES |
| [README.md](README.md) | Project overview & features | ✅ YES |
| [SYSTEM_ANALYSIS.md](SYSTEM_ANALYSIS.md) | Deep technical analysis, gap report, architecture | For developers |
| [TRANSFORMATION_SUMMARY.md](TRANSFORMATION_SUMMARY.md) | Catalog of all changes from v1 to v2 | For developers |
| [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md) | Step-by-step field deployment | For operators |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Diagnostic flowchart & common issues | For field support |
| [WIRING_REFERENCE.md](WIRING_REFERENCE.md) | Complete pinout, schematics, power budget | For hardware builders |
| [TEST_CHECKLIST.md](TEST_CHECKLIST.md) | 83-point validation checklist | For QA/testing |
| [system_architecture.png](system_architecture.png) | Visual architecture diagram | Reference |
| [fsm_diagram.png](fsm_diagram.png) | FSM state machine diagram | Reference |

---

## 🔧 Firmware (31 files)

### Build Configuration
| File | Purpose |
|------|---------|
| [platformio.ini](firmware/platformio.ini) | PlatformIO build config, libraries, partitions |
| [partitions.csv](firmware/partitions.csv) | Flash layout (factory app + SPIFFS) |
| [pre_script.py](firmware/pre_script.py) | Pre-build version generation |

### Headers (14 files in `firmware/include/`)
| File | Module | Description |
|------|--------|-------------|
| [config.h](firmware/include/config.h) | Global | Master pinout, thresholds, constants |
| [hlink_protocol.h](firmware/include/hlink_protocol.h) | Protocol | H-Link v2.0 packet structure, ARQ, CRC-8 |
| [fsm_manager.h](firmware/include/fsm_manager.h) | FSM | 5-state finite state machine |
| [link_manager.h](firmware/include/link_manager.h) | Routing | Abstract link interface, Make-Before-Break |
| [wifi_link.h](firmware/include/wifi_link.h) | Wi-Fi | ESP-NOW + AP + UDP |
| [fso_link.h](firmware/include/fso_link.h) | FSO | Laser UART + ADC envelope |
| [lora_link.h](firmware/include/lora_link.h) | LoRa | SX1278 SPI driver |
| [telemetry_manager.h](firmware/include/telemetry_manager.h) | Telemetry | Scheduled LoRa broadcasts |
| [display_manager.h](firmware/include/display_manager.h) | UI | SSD1306 async HUD |
| [feedback_manager.h](firmware/include/feedback_manager.h) | Feedback | LEDs, buzzer, patterns |
| [battery_manager.h](firmware/include/battery_manager.h) | Power | ADC monitoring, level detection |
| [button_manager.h](firmware/include/button_manager.h) | Input | Debounce, long-press, emergency |
| [config_manager.h](firmware/include/config_manager.h) | Storage | NVS persistent config |
| [crypto_manager.h](firmware/include/crypto_manager.h) | Security | AES-256-GCM via mbedTLS |

### Source (14 files in `firmware/src/`)
| File | Module | Description |
|------|--------|-------------|
| [main.cpp](firmware/src/main.cpp) | Entry | FreeRTOS task creation, boot sequence |
| [hlink_protocol.cpp](firmware/src/hlink_protocol.cpp) | Protocol | Packet serialize/deserialize, ARQ manager |
| [fsm_manager.cpp](firmware/src/fsm_manager.cpp) | FSM | EMA filtering, hysteresis, transitions |
| [link_manager.cpp](firmware/src/link_manager.cpp) | Routing | Dedup cache, broadcast, duplication |
| [wifi_link.cpp](firmware/src/wifi_link.cpp) | Wi-Fi | ESP-NOW callbacks, UDP server |
| [fso_link.cpp](firmware/src/fso_link.cpp) | FSO | UART2 driver, ADC reading |
| [lora_link.cpp](firmware/src/lora_link.cpp) | LoRa | SPI initialization, packet polling |
| [telemetry_manager.cpp](firmware/src/telemetry_manager.cpp) | Telemetry | Heartbeat, GPS, battery, link status |
| [display_manager.cpp](firmware/src/display_manager.cpp) | UI | Zone-based OLED rendering |
| [feedback_manager.cpp](firmware/src/feedback_manager.cpp) | Feedback | PWM patterns, tone generation |
| [battery_manager.cpp](firmware/src/battery_manager.cpp) | Power | Voltage reading, percentage mapping |
| [button_manager.cpp](firmware/src/button_manager.cpp) | Input | State machine, callbacks |
| [config_manager.cpp](firmware/src/config_manager.cpp) | Storage | NVS read/write/reset |
| [crypto_manager.cpp](firmware/src/crypto_manager.cpp) | Security | GCM encrypt/decrypt, IV generation |

---

## 💻 Software (14 files)

### Core (5 files)
| File | Purpose |
|------|---------|
| [main.py](software/main.py) | Application entry point |
| [config.py](software/config.py) | Persistent app configuration |
| [udp_client.py](software/udp_client.py) | Threaded UDP communication |
| [hlink_protocol.py](software/hlink_protocol.py) | Python H-Link implementation |
| [crypto_manager.py](software/crypto_manager.py) | Python AES-256-GCM |
| [requirements.txt](software/requirements.txt) | Python dependencies |

### UI Widgets (7 files in `software/ui/`)
| File | Feature | Description |
|------|---------|-------------|
| [main_window.py](software/ui/main_window.py) | Main | Tactical dark theme, navigation, status bar |
| [chat_widget.py](software/ui/chat_widget.py) | Chat | Message bubbles, delivery ticks, encryption |
| [voice_widget.py](software/ui/voice_widget.py) | Voice | PTT button, level meter, spacebar hotkey |
| [files_widget.py](software/ui/files_widget.py) | Files | Chunked transfer, compression, progress |
| [map_widget.py](software/ui/map_widget.py) | Map | GPS coordinates, offline tiles |
| [dashboard_widget.py](software/ui/dashboard_widget.py) | Dashboard | Signal bars, telemetry, statistics |
| [settings_widget.py](software/ui/settings_widget.py) | Settings | Thresholds, keys, audio, theme |

---

## 🗺️ Navigation Guide

### For First-Time Users
1. Read [QUICK_START.md](QUICK_START.md)
2. Follow hardware assembly in [WIRING_REFERENCE.md](WIRING_REFERENCE.md)
3. Flash firmware, install software
4. Run [TEST_CHECKLIST.md](TEST_CHECKLIST.md)

### For Hardware Engineers
1. [WIRING_REFERENCE.md](WIRING_REFERENCE.md) — Complete schematics
2. [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md) — Assembly steps
3. [TROUBLESHOOTING.md](TROUBLESHOOTING.md) — Power & signal issues

### For Firmware Developers
1. [SYSTEM_ANALYSIS.md](SYSTEM_ANALYSIS.md) — Architecture deep-dive
2. [TRANSFORMATION_SUMMARY.md](TRANSFORMATION_SUMMARY.md) — Change catalog
3. [firmware/include/config.h](firmware/include/config.h) — Pinout & thresholds
4. [firmware/src/main.cpp](firmware/src/main.cpp) — Task architecture

### For Software Developers
1. [software/main.py](software/main.py) — Entry point
2. [software/udp_client.py](software/udp_client.py) — Threading model
3. [software/ui/main_window.py](software/ui/main_window.py) — UI architecture

### For Field Operators
1. [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md) — Field setup
2. [TROUBLESHOOTING.md](TROUBLESHOOTING.md) — Quick fixes
3. [TEST_CHECKLIST.md](TEST_CHECKLIST.md) — Pre-flight checks

---

## 📊 Statistics

| Metric | Value |
|--------|-------|
| Total Files | 54 |
| Total Size | ~250 KB |
| Firmware Files | 31 (C/C++) |
| Software Files | 14 (Python) |
| Documentation | 9 (Markdown) |
| Firmware Lines | ~3,500 |
| Software Lines | ~2,200 |
| Test Cases | 83 |

---

## 🔐 Security Notice

All encryption keys are generated randomly on first boot and stored in ESP32 NVS. **Never share your device key fingerprint with unauthorized parties.** The 2-character fingerprint on the OLED is for visual verification only — it does not reveal the full key.

---

## 📞 Support

For issues not covered in the documentation:
1. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) diagnostic flowchart
2. Review serial debug output at 115200 baud
3. Verify wiring against [WIRING_REFERENCE.md](WIRING_REFERENCE.md)
4. Run through [TEST_CHECKLIST.md](TEST_CHECKLIST.md) systematically

---

*Master Index v2.0 — April 2026*  
*SnakeLink — University of Kelaniya Research Project*
