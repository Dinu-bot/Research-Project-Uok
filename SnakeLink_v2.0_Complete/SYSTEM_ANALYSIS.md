# SnakeLink — Complete System Analysis & Transformation Report
**Version:** 2.0 (Industry-Grade Refinement)  
**Date:** April 2026  
**Classification:** Technical Reference / Production Blueprint

---

## PHASE 1: DEEP UNDERSTANDING

### 1.1 System Architecture Overview
SnakeLink is a **tri-mode hybrid tactical communication system** designed for disaster response and defense operations in Sri Lanka. It creates a self-contained, off-grid communication bridge between two operator stations.

**Physical Topology:**
```
[Laptop A] <--UDP/Wi-Fi AP--> [Transceiver A] <--FSO/Wi-Fi ESP-NOW/LoRa--> [Transceiver B] <--UDP/Wi-Fi AP--> [Laptop B]
```

**Core Components:**
- **2× ESP32 Transceivers** (DevKit v1, 30-pin): Physical-layer processing, protocol handling, FSM, telemetry
- **2× Laptops**: Application-layer processing (PyQt6 desktop app), encryption, compression, UI
- **3× Communication Links**: FSO (laser, 9600–115200 bps), Wi-Fi (ESP-NOW, ~500 kbps), LoRa (433 MHz, 0.3–5 kbps)

### 1.2 Data Flow Architecture

| Layer | Responsibility | Hardware |
|-------|---------------|----------|
| Application | Encryption, compression, chunking, UI | Laptop (Python) |
| Transport | H-Link protocol, ARQ, deduplication | ESP32 (C++) |
| Network | Link selection, FSM switching | ESP32 (C++) |
| Data Link | UART (FSO), ESP-NOW (Wi-Fi), SPI (LoRa) | ESP32 + peripherals |
| Physical | Laser diode, photodiode, RF transceivers | Analog front-end + Ra-02 |

### 1.3 Novelty & Objectives
1. **Make-Before-Break Switching**: Zero-packet-loss link transition via simultaneous duplication
2. **Deterministic FSM**: Microsecond-level switching decisions without ML overhead
3. **Hardware-Software Co-design**: ADC envelope monitoring enables predictive switching before packet loss
4. **Tri-mode Redundancy**: FSO (stealth), Wi-Fi (bandwidth), LoRa (resilience)
5. **End-to-End Security**: AES-256-GCM with optional laptop-level encryption

---

## PHASE 2: GAP ANALYSIS

### 2.1 Critical Issues (Must Fix)

| ID | Issue | Impact | Resolution |
|----|-------|--------|------------|
| C1 | **Pin Mismatch**: Core Report assigns GPIO 33 to Mode Button; Corrected Guide assigns GPIO 33 to Red LED and GPIO 32/13 to buttons | Firmware will fail to read button inputs; LED conflicts | Adopt Corrected Guide v2 pinout exclusively |
| C2 | **Missing LoRa RST**: Core Report omits LoRa RST pin; Corrected Guide adds GPIO 14 | LoRa module may not reset properly on boot | Add GPIO 14 RST control to firmware |
| C3 | **No GPS Hardware**: Software expects GPS coordinates every 30s, but no GPS module in hardware guide | Telemetry GPS packets cannot be sourced | Add software GPS fallback (manual entry + laptop GPS dongle) |
| C4 | **Power Brownout Risk**: Original v1 fed 3.7V directly to VIN; v2 fixes with MT3608→5V | System instability during Wi-Fi bursts | Validate v2 power architecture in firmware |
| C5 | **Missing Watchdog**: No watchdog timer implementation mentioned | System freeze in field conditions unrecoverable | Add hardware + task watchdogs |
| C6 | **No Persistent Storage**: ESP32 has no SD card or SPIFFS usage for config | Device ID, keys, thresholds lost on reboot | Add SPIFFS/NVS config storage |
| C7 | **Buffer Overflow Risk**: 240-byte payload with multi-byte UTF-8 (Sinhala) can exceed buffer | Memory corruption, crashes | Add explicit bounds checking |

### 2.2 Moderate Issues (Should Improve)

| ID | Issue | Impact | Resolution |
|----|-------|--------|------------|
| M1 | **No OTA Updates**: Field firmware updates require USB cable | Maintenance burden in deployed units | Add OTA via Wi-Fi hotspot |
| M2 | **Single-threaded Python UI**: PyQt6 app may block on UDP I/O | UI freezing during file transfers | Add threaded UDP handler |
| M3 | **No Rate Limiting**: Emergency broadcast floods all links | Denial-of-service on own network | Add 5-second cooldown + confirmation |
| M4 | **Missing ACK Batching**: Stop-and-wait ARQ is inefficient on high-latency LoRa | Throughput collapse on LoRa | Add selective repeat for LoRa |
| M5 | **No Link Quality History**: FSM uses instantaneous values only | False switches due to transient noise | Add exponential moving average (EMA) filtering |
| M6 | **Codec2 Dependency**: Core Report mentions Codec2 but Python bindings are complex | Voice on FSO may not work | Add fallback to Opus at low bitrate |

### 2.3 Minor Issues (Optional Optimization)

| ID | Issue | Resolution |
|----|-------|------------|
| N1 | No sleep modes for power saving | Add light sleep during idle periods |
| N2 | No compression on ESP32 side | Add LZ4 or heatshrink for file chunks |
| N3 | OLED uses blocking I2C | Switch to async display updates |
| N4 | No automated testing framework | Add unit tests for H-Link protocol |

---

## PHASE 3: HARDWARE VALIDATION

### 3.1 Power Architecture Validation ✅
**v2 Correction**: MT3608 boosted to **5.0V** (not 12V), single-rail architecture.

**Analysis:**
- **Input**: 2× 18650 in parallel → 3.7V nominal, 4.2V max, ~7000 mAh
- **Charger**: TP4056 with master switch isolation rule (switch OFF during charging)
- **Boost**: MT3608 → 5.0V ± 0.05V
- **ESP32 VIN**: 5.0V → AMS1117 onboard → 3.3V (stable, no brownout)
- **3.3V Rail**: From ESP32 3V3 pin → LoRa, OLED, pull-ups
- **5V Rail**: MT3608 OUT+ → Laser anode, Op-Amps (CA3140E, LM358, LM393)

**Verdict**: CORRECT. The 5V single-rail eliminates brownout issues. The 100Ω + 100Ω battery divider on GPIO 35 gives 2.1V max (safe for 3.3V ADC).

### 3.2 FSO Receiver Chain Validation ✅
**Path**: BPW34 → CA3140E TIA → AC coupling → LM393 comparator → GPIO 16 (digital) + GPIO 34 (ADC envelope)

**Analysis:**
- **BPW34**: Reverse-biased, cathode to +5V, anode to CA3140E IN−. Correct for photoconductive mode.
- **CA3140E**: TIA configuration with 100kΩ feedback + 22pF stability cap. Quiescent point at 2.5V (VGND). Output swings toward 0V when illuminated.
- **LM393**: AC-coupled input (Pin 3), threshold via 100kΩ trimpot (Pin 2). Open-collector output pulled to 3.3V via 2.2kΩ → GPIO 16. **CRITICAL**: Pull-up to 3.3V (not 5V) protects ESP32.
- **ADC Path**: Separate AC coupling + 10kΩ/10kΩ bias divider → 1.65V idle at GPIO 34.

**Verdict**: CORRECT. The corrected LM393 pinout (IN1−=Pin2, IN1+=Pin3) is properly implemented. The 2.5V virtual ground from LM358 is stable.

### 3.3 Laser Transmitter Validation ✅
**Path**: GPIO 17 → 1kΩ series → 2N7000 Gate → Drain → Laser cathode → 100Ω → +5V

**Analysis:**
- **Current limit**: R = (5.0 − 2.2 − 0.1) / 0.028 ≈ 100Ω. Correct.
- **Power dissipation**: P = I²R = 0.028² × 100 = 78 mW. 1/4W resistor sufficient.
- **Simmer bypass**: 2.2kΩ across Drain-Source gives ~1.3mA pre-bias. Good for fast switching.
- **Gate protection**: 1kΩ series limits inrush, 100kΩ pull-down ensures OFF at boot.

**Verdict**: CORRECT. The 100Ω current limit is non-negotiable and properly calculated for the real 5V rail.

### 3.4 LoRa Module Validation ✅
**Ra-02 (SX1278)**: 3.3V ONLY. SPI to GPIO 18/19/23/5, DIO0 to GPIO 4, RST to GPIO 14.

**Analysis:**
- **Voltage**: Powered from +3V3 rail. Correct.
- **Antenna**: SMA connector required. Transmitting without antenna damages PA. Documented.
- **Decoupling**: 100nF cap on 3.3V/GND near module. Correct.

**Verdict**: CORRECT with the addition of GPIO 14 RST.

### 3.5 Pin Assignment Validation ⚠️
**Conflict Resolution** (adopting Corrected Guide v2 as master):

| GPIO | Core Report | Corrected Guide v2 | Adopted |
|------|-------------|-------------------|---------|
| 16 | FSO RX | FSO RX (UART2) | ✅ v2 |
| 17 | FSO TX | FSO TX (UART2) | ✅ v2 |
| 34 | FSO ADC | FSO ADC | ✅ v2 |
| 35 | Battery ADC | Battery ADC | ✅ v2 |
| 18/19/23/5/4 | LoRa SPI/DIO0 | LoRa SPI/DIO0 | ✅ v2 |
| 14 | — | LoRa RST | ✅ v2 |
| 21/22 | OLED I2C | OLED I2C | ✅ v2 |
| 25 | Green LED | Buzzer | ✅ v2 |
| 26 | Blue LED | Green LED | ✅ v2 |
| 27 | Red LED | Blue LED | ✅ v2 |
| 33 | Mode Button | Red LED | ✅ v2 |
| 32 | — | Button 1 (Align) | ✅ v2 |
| 13 | — | Button 2 (Mode/Emergency) | ✅ v2 |

**Note**: The Core Report had LEDs on 25/26/27 and button on 33. The Corrected Guide moves Buzzer to 25, LEDs to 26/27/33, and buttons to 32/13. **We adopt the Corrected Guide exclusively** as it is the reviewed, corrected version.

---

## PHASE 4: ARCHITECTURE REFINEMENT

### 4.1 Refined System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           SNAKELINK v2.0 ARCHITECTURE                       │
├─────────────────────────────────────────────────────────────────────────────┤
│  LAPTOP A                              │  LAPTOP B                          │
│  ┌─────────────────────────────────┐   │  ┌─────────────────────────────────┐│
│  │ PyQt6 Tactical UI               │   │  │ PyQt6 Tactical UI               ││
│  │ ├── Chat (AES-256-GCM E2E)      │   │  │ ├── Chat (AES-256-GCM E2E)      ││
│  │ ├── Voice PTT (Opus/Codec2)     │   │  │ ├── Voice PTT (Opus/Codec2)     ││
│  │ ├── File Transfer (chunk+resume)│   │  │ ├── File Transfer (chunk+resume)││
│  │ ├── Offline Map (folium)        │   │  │ ├── Offline Map (folium)        ││
│  │ └── Dashboard (telemetry)       │   │  │ └── Dashboard (telemetry)       ││
│  └────────────┬────────────────────┘   │  └────────────┬────────────────────┘│
│               │ UDP (port 4210)        │               │ UDP (port 4210)       │
│               ▼                        │               ▼                       │
│  ┌─────────────────────────────────┐   │  ┌─────────────────────────────────┐│
│  │ UDP Client Thread               │   │  │ UDP Client Thread               ││
│  │ ├── Packetizer (H-Link)         │   │  │ ├── Packetizer (H-Link)         ││
│  │ ├── Crypto Engine               │   │  │ ├── Crypto Engine               ││
│  │ └── File Chunker/Compressor     │   │  │ └── File Chunker/Compressor     ││
│  └────────────┬────────────────────┘   │  └────────────┬────────────────────┘│
│               │ Wi-Fi AP               │               │ Wi-Fi AP              │
└───────────────┼────────────────────────┘───────────────┼─────────────────────┘
                │                                        │
┌───────────────▼────────────────────────────────────────▼─────────────────────┐
│                           INTER-DEVICE LINK LAYER                            │
│  ┌─────────────────┐    ┌─────────────────┐    ┌──────────────────────────┐  │
│  │ FSO Link        │    │ Wi-Fi Link      │    │ LoRa Link                │  │
│  │ (UART2, 115200) │    │ (ESP-NOW, 500k) │    │ (SPI, 433MHz, SF7-12)    │  │
│  │ ├── Laser TX/RX │    │ ├── RSSI Monitor│    │ ├── Background Telemetry │  │
│  │ └── ADC Envelope│    │ └── MAC Filter  │    │ └── Emergency Fallback   │  │
│  └────────┬────────┘    └────────┬────────┘    └────────────┬─────────────┘  │
│           │                      │                          │                │
│           └──────────────────────┼──────────────────────────┘                │
│                                  ▼                                           │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                    LINK MANAGER + FSM CORE                             │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────────┐ │  │
│  │  │ Signal EMA  │→ │ Threshold   │→ │ Make-Before-Break Duplication   │ │  │
│  │  │ Filter      │  │ FSM         │  │ (Warning State, 1-3s window)    │ │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────────────┘ │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────────┐ │  │
│  │  │ H-Link TX/RX│  │ Stop-and-Wait│  │ Packet Deduplication (SEQ)      │ │  │
│  │  │ Protocol    │  │ ARQ (3 retries)│ │ Hysteresis (5s stability)       │ │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────────────┘ │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│                                  ▼                                           │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                    HARDWARE ABSTRACTION LAYER                          │  │
│  │  Core 0 (Background): LoRa, OLED, LEDs, Buzzer, Battery, Buttons      │  │
│  │  Core 1 (Real-Time):  FSO UART, Wi-Fi ESP-NOW, FSM, ACK handling      │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Communication Protocol Stack

**H-Link Protocol v2.0 Enhancements:**
- Frame: `| SYNC (2B) | TYPE (1B) | SEQ (2B) | LEN (2B) | PAYLOAD (N) | CRC-8 (1B) |`
- Added `TIMESTAMP (4B)` field for anti-replay (inserted after SEQ in encrypted mode)
- Added `FLAGS (1B)` field for priority, encryption status, fragmentation
- CRC-8 polynomial: 0x07 (ITU) — verified lightweight for ESP32

**APIs:**
- `LinkManager::send(Packet)` — route to active link
- `LinkManager::broadcast(Packet)` — send on ALL links (emergency)
- `FSM::update(signalMetrics)` — evaluate thresholds
- `Telemetry::schedule(type, interval)` — background transmissions

### 4.3 Security Layers

| Layer | Mechanism | Location |
|-------|-----------|----------|
| L1 | Device Authentication (pre-shared Device ID) | ESP32 NVS |
| L2 | Transport Encryption (AES-256-GCM, pre-shared key) | ESP32 firmware |
| L3 | Application E2E Encryption (AES-256-GCM, per-session) | Laptop software |
| L4 | Anti-Replay (SEQ + 32-bit timestamp, 5s window) | Both |
| L5 | Key Fingerprint (2-char display on OLED + UI) | Both |

### 4.4 Data Models

**File Transfer State:**
```cpp
struct FileTransferState {
    uint32_t fileId;
    char filename[64];
    uint32_t totalSize;
    uint16_t totalChunks;
    uint16_t chunksAcked;
    uint32_t timestamp;
    bool encrypted;
    uint8_t hash[32]; // SHA-256
};
```

**Telemetry Packet:**
```cpp
struct TelemetryPacket {
    float latitude;
    float longitude;
    float batteryVoltage;
    uint8_t batteryPercent;
    uint16_t estRuntimeMinutes;
    float fsoVoltage;
    int8_t wifiRSSI;
    int8_t loraRSSI;
    uint8_t activeLink;
    uint32_t uptime;
};
```

---

## PHASE 5: FULL CODE TRANSFORMATION

### 5.1 Firmware Transformation Summary

| Module | Before | After |
|--------|--------|-------|
| Build System | Basic Arduino IDE | PlatformIO with partition scheme |
| Config | Hardcoded values | `config.h` with compile-time + NVS runtime |
| Protocol | Basic struct | Full H-Link v2 with bounds checking |
| FSM | 3 states | 5 states (Idle, WiFi, FSO, LoRa, Warning) |
| Links | Inline code | Abstract `LinkInterface` with polymorphic drivers |
| Display | Blocking updates | Double-buffered async OLED |
| Storage | None | NVS for config, SPIFFS for logs |
| Watchdog | None | Task watchdog + hardware watchdog |
| Logging | Serial only | Structured log levels + SPIFFS persistence |

### 5.2 Software Transformation Summary

| Module | Before | After |
|--------|--------|-------|
| Architecture | Single-threaded | Multi-threaded (UI, UDP, Audio, File) |
| UDP | Blocking socket | Async threaded with queue |
| Crypto | Basic AES | AES-256-GCM with authenticated encryption |
| Audio | Basic capture | Threaded with ring buffer, auto-codec selection |
| Files | Basic send | Chunked, compressed, resumable, encrypted |
| Maps | Online | Offline tile cache + pre-downloaded Sri Lanka tiles |
| Packaging | Manual | PyInstaller spec with assets bundled |

---

## PHASE 6: EXPLANATION OF MAJOR CHANGES

### Change 1: Pinout Alignment to Corrected Guide v2
**What**: Adopted GPIO 32/13 for buttons, GPIO 26/27/33 for LEDs, GPIO 25 for buzzer, GPIO 14 for LoRa RST.
**Why**: The Core Report contained uncorrected pin assignments that conflict with the reviewed v2 hardware.
**Impact**: Firmware now matches the physical breadboard exactly.

### Change 2: Abstract Link Interface
**What**: Created `LinkInterface` base class with `FSOLink`, `WiFiLink`, `LoRaLink` implementations.
**Why**: Enables clean Make-Before-Break duplication (send on multiple links simultaneously) and simplifies FSM logic.
**Impact**: Adding a 4th link (e.g., ultrasonic) requires only a new subclass.

### Change 3: EMA Signal Filtering
**What**: Added exponential moving average (α=0.3) to FSO ADC and Wi-Fi RSSI readings.
**Why**: Prevents FSM chattering from transient noise spikes.
**Impact**: More stable switching decisions in fog/rain conditions.

### Change 4: Threaded Python Architecture
**What**: Separated UI thread, UDP I/O thread, audio thread, and file transfer thread.
**Why**: Prevents UI freezing during large file transfers or high-latency LoRa operations.
**Impact**: Responsive interface even under heavy load.

### Change 5: Persistent Configuration
**What**: ESP32 uses NVS (Non-Volatile Storage) for device ID, encryption keys, thresholds.
**Why**: Operators can configure once in the field without recompiling firmware.
**Impact**: Deployment-ready for non-technical users.

---

## PHASE 7: FINAL OUTPUT

### 7.1 Corrected System Architecture
See Phase 4 diagram and the generated code structure below.

### 7.2 Hardware Validation Report
**Status: VALIDATED WITH CORRECTIONS**
- Power: ✅ Single 5V rail, no brownout
- FSO RX: ✅ CA3140E + LM393 with correct pinout
- FSO TX: ✅ 100Ω limit, 28mA, simmer bypass
- LoRa: ✅ 3.3V only, RST added
- Battery: ✅ 2:1 divider, 2.1V max at ADC
- Wiring: ✅ Star ground, analog/digital island separation

### 7.3 Issue List (Categorized)
See Phase 2 tables (5 Critical, 6 Moderate, 4 Minor).

### 7.4 Updated File Structure
```
RESEARCH-PROJECT-UOK/
├── firmware/
│   ├── platformio.ini          (Build config, partitions, libraries)
│   ├── src/
│   │   ├── main.cpp            (FreeRTOS init, task creation)
│   │   ├── config.cpp          (NVS-backed configuration)
│   │   ├── fsm_manager.cpp     (5-state FSM with hysteresis)
│   │   ├── link_manager.cpp    (Abstract link routing)
│   │   ├── fso_link.cpp        (UART2 laser driver)
│   │   ├── wifi_link.cpp       (ESP-NOW implementation)
│   │   ├── lora_link.cpp       (SX1278 SPI driver)
│   │   ├── hlink_protocol.cpp  (Packet encode/decode, CRC-8)
│   │   ├── telemetry_manager.cpp (GPS, battery, heartbeat)
│   │   ├── display_manager.cpp (SSD1306 async UI)
│   │   ├── feedback_manager.cpp (LEDs, buzzer patterns)
│   │   ├── crypto_manager.cpp  (AES-256-GCM via mbedTLS)
│   │   ├── battery_manager.cpp (ADC + voltage mapping)
│   │   └── button_manager.cpp  (Debounce, long-press detection)
│   └── include/
│       └── (all headers)
├── software/
│   ├── main.py                 (Entry point)
│   ├── config.py               (App configuration)
│   ├── udp_client.py           (Threaded UDP comms)
│   ├── crypto_manager.py       (AES-256-GCM)
│   ├── audio_manager.py        (Capture/playback/codec)
│   ├── file_transfer.py        (Chunking, compression, resume)
│   ├── hlink_protocol.py       (Python H-Link implementation)
│   └── ui/
│       ├── main_window.py      (Tactical dark theme)
│       ├── chat_widget.py      (Message bubbles, delivery ticks)
│       ├── voice_widget.py     (PTT button, level meter)
│       ├── files_widget.py     (Drag-drop, progress, resume)
│       ├── map_widget.py       (Offline folium map)
│       ├── dashboard_widget.py (Signal bars, telemetry)
│       └── settings_widget.py  (Thresholds, keys, audio)
└── docs/
    └── SYSTEM_ANALYSIS.md      (This document)
```

### 7.5 Integration Flow
1. **Power-on**: MT3608 delivers 5V, ESP32 boots from VIN
2. **Init**: Load config from NVS, initialize all links, display boot sequence
3. **Wi-Fi AP**: ESP32 creates hotspot, laptop connects, UDP socket opens
4. **Link Discovery**: FSM evaluates signals; default to Wi-Fi if RSSI > -75dBm
5. **Normal Operation**: Laptop sends H-Link packets via UDP; ESP32 routes via active link
6. **Degradation**: FSO ADC drops → FSM enters Warning → duplicate packets → switch to Wi-Fi
7. **Emergency**: Long-press Button 2 → broadcast on all links → alarm on receiver

### 7.6 Deployment Recommendations
1. **Pre-deployment**: Flash firmware, write device ID + key to NVS via setup wizard
2. **Field pairing**: Laptops connect to respective ESP32 APs (SSID: SnakeLink-A / SnakeLink-B)
3. **Alignment**: Use Button 1 (GPIO 32) for FSO alignment mode with buzzer pitch feedback
4. **Power**: Master switch OFF before charging; 2× 18650 gives ~12-18 hours continuous
5. **Security**: Enable High Security mode for classified data; verify key fingerprint on OLED

### 7.7 Future Scalability
1. **Mesh Extension**: LoRa can be extended to multi-hop with routing table
2. **Satellite Backhaul**: Add Iridium modem as 4th link for beyond-line-of-sight
3. **Mobile App**: Port PyQt6 UI to Kivy for Android tablets
4. **HSM Integration**: Replace software AES with ATECC608A secure element
5. **ML Enhancement**: Add edge ML for predictive link failure (post-deployment upgrade)

---

*End of System Analysis Report*
