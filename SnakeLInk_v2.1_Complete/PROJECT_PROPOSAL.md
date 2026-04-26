# SnakeLink v2.0
## Tri-Mode Hybrid Tactical Communication System for Disaster Response & Defense

**Research Project — University of Kelaniya, Sri Lanka**  
**Date:** April 2026

---

## 1. Executive Summary

SnakeLink is a **self-contained, off-grid tactical communication system** that creates an instant bridge between two operator stations without requiring cellular towers, internet, or grid power. The system uses a **novel free-space optical (FSO) laser link as its primary medium**, with Wi-Fi and LoRa as automatic fallbacks. When environmental conditions disrupt the laser, the system seamlessly switches to Wi-Fi. If both fail, LoRa maintains a lifeline. All switching happens with **zero packet loss** through Make-Before-Break duplication.

---

## 2. Problem Statement

Sri Lanka faces recurring natural disasters — floods, landslides, cyclones — that destroy conventional communication infrastructure within minutes. First responders, military units, and disaster management teams are left isolated precisely when coordination is most critical. Existing solutions are either:

- **Dependent on infrastructure** (cellular, fiber) that fails during disasters
- **Expensive military-grade radios** unaffordable for civilian agencies
- **Single-medium systems** with no redundancy when conditions change

There is no portable, affordable system that provides **multiple independent physical layers** with automatic failover.

---

## 3. Proposed Solution

SnakeLink integrates **three communication links** into one portable transceiver pair:

| Priority | Link | Technology | Speed | Role |
|----------|------|-----------|-------|------|
| **1st** | **FSO (Laser)** | 650nm laser diode + photodiode | **115 kbps** | **PRIMARY** — default, stealth, no RF signature |
| **2nd** | **Wi-Fi** | ESP-NOW peer-to-peer | **~500 kbps** | **BACKUP** — activates when fog/rain disrupts laser |
| **3rd** | **LoRa** | SX1278 @ 433 MHz | **0.3–5 kbps** | **EMERGENCY** — lifeline when both above fail |

### Why FSO as Primary?

The **laser link is our core innovation** and default medium because:

1. **Speed & Bandwidth**: 115,200 baud UART provides reliable throughput for chat, compressed voice, and file chunks
2. **Stealth**: No radio frequency emissions — undetectable by conventional SIGINT or jamming [^6^]
3. **No spectrum licensing**: Unlike RF systems, FSO requires no frequency allocation
4. **Deterministic latency**: UART timing is predictable, unlike contended Wi-Fi channels
5. **No interference**: Immune to RF congestion, electromagnetic interference, or jamming

Wi-Fi serves as the **high-bandwidth backup** when environmental conditions (fog, heavy rain, dust, misalignment) attenuate the laser beam. LoRa provides the **ultimate resilience** — it penetrates obstacles and operates over hundreds of meters even when both optical and Wi-Fi links are completely blocked.

---

## 4. System Architecture

```
[Laptop A] ←──UDP/Wi-Fi AP──→ [ESP32 Transceiver A] 
                                    │
                                    ├──► FSO Laser ──┐
                                    ├──► Wi-Fi ESP-NOW│──→ [ESP32 Transceiver B]
                                    └──► LoRa 433MHz─┘
[Laptop B] ←──UDP/Wi-Fi AP──→ [ESP32 Transceiver B]
```

### Hardware Components (Per Station)

| Component | Function |
|-----------|----------|
| **ESP32 DevKit v1** | Dual-core processor, protocol stack, FSM, encryption |
| **650nm Laser Diode + 2N7000 Driver** | FSO transmitter with 100Ω current limiting |
| **BPW34 Photodiode + CA3140E TIA** | FSO receiver with transimpedance amplification |
| **LM393 Comparator** | Digital signal recovery from analog envelope |
| **Ra-02 (SX1278)** | LoRa transceiver for emergency fallback |
| **SSD1306 OLED** | Tactical HUD displaying link status, battery, signal |
| **2× 18650 Li-ion + MT3608** | Self-powered operation, 12–18 hours runtime |

### Software Stack

| Layer | Technology | Responsibility |
|-------|-----------|--------------|
| **Application** | PyQt6 (Python) | Chat, voice PTT, file transfer, map, dashboard |
| **Transport** | H-Link v2.0 | Packet framing, ARQ, deduplication, encryption |
| **Network** | FSM Manager | Link selection, Make-Before-Break switching |
| **Physical** | FSO / Wi-Fi / LoRa | Three independent radio/optical layers |

---

## 5. Key Innovations

### 5.1 Make-Before-Break Zero-Loss Switching

When the laser beam is disrupted by fog or rain:

1. **FSM detects degradation** via ADC envelope monitoring (GPIO 34)
2. **Enters WARNING state** for 3 seconds
3. **Duplicates packets** on both FSO and Wi-Fi simultaneously
4. **Switches to Wi-Fi** only after stability is confirmed
5. **Zero packets lost** during the transition

### 5.2 Hardware-Software Co-Design

A dedicated ADC channel monitors the **FSO signal envelope voltage** in real-time. This enables **predictive switching** — the system reacts to signal attenuation *before* packets are lost, rather than waiting for CRC errors or ACK timeouts.

### 5.3 Continuous LoRa Telemetry

Even while chat and files flow over FSO or Wi-Fi, LoRa broadcasts **every 5–60 seconds**:
- Heartbeat (device alive)
- GPS coordinates
- Battery voltage & runtime estimate
- Active link status

This creates a **persistent situational awareness channel** independent of the primary data link.

### 5.4 End-to-End Security

- **AES-256-GCM** on both ESP32 (mbedTLS hardware acceleration) and laptop
- **Anti-replay protection** via sequence numbers + timestamps
- **Visual key fingerprint** (2 characters) on OLED and UI for MITM detection

---

## 6. Operational Scenario

### Normal Operation (Clear Weather)
```
FSO:     ████████████████████ ACTIVE — Chat, Voice, Files
Wi-Fi:   ░░░░░░░░░░░░░░░░░░░░ Standby (monitored)
LoRa:    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ Telemetry (heartbeat every 5s)
```

### Fog/Rain Disrupts Laser
```
FSO:     ░░░░░░░░░░░░░░░░░░░░ Degraded (ADC < 1.0V)
Wi-Fi:   ████████████████████ ACTIVE — Seamless takeover
LoRa:    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ Telemetry continues
```

### Both Optical and Wi-Fi Blocked
```
FSO:     ░░░░░░░░░░░░░░░░░░░░ Blocked
Wi-Fi:   ░░░░░░░░░░░░░░░░░░░░ Blocked
LoRa:    ████████████████████ EMERGENCY — Text + GPS only
```

---

## 7. Deliverables

| # | Deliverable | Description |
|---|-------------|-------------|
| 1 | **2× ESP32 Transceivers** | Assembled, flashed, tested with tri-mode links |
| 2 | **2× Laptop Stations** | Running PyQt6 tactical software |
| 3 | **FSO Link Validation** | Verified 50m+ range, alignment mode, ADC monitoring |
| 4 | **FSM Demonstration** | Live switching between FSO→Wi-Fi→LoRa with zero loss |
| 5 | **Security Verification** | AES-256-GCM end-to-end, key fingerprint matching |
| 6 | **Field Test Report** | Range, battery life, environmental resilience |

---

## 8. Significance

### For Disaster Response
Sri Lanka's disaster management suffers from **poor inter-agency coordination** and **infrastructure dependency** [^2^]. SnakeLink provides instant communication when towers are down — no setup, no subscriptions, no external power.

### For Defense
The FSO laser provides **LPI/LPD (Low Probability of Intercept/Detection)** communication with **zero RF signature** [^6^]. In contested environments, this is superior to radio-based tactical systems that can be jammed or triangulated.

### For Developing Nations
Built from **<$50 commodity components** per transceiver, the system is **affordable and replicable** — unlike military radios costing thousands of dollars.

---

## 9. Conclusion

SnakeLink v2.0 delivers a **novel, practical, and robust** tactical communication solution. By making the **FSO laser the primary link** — leveraging its speed, stealth, and immunity to RF interference — and backing it with automatic Wi-Fi and LoRa fallbacks, the system ensures continuous communication across the widest possible range of environmental and threat conditions.

**No infrastructure. No internet. No RF emissions. Just light, air, and guaranteed delivery.**

---

*SnakeLink v2.0 — University of Kelaniya Research Project*  
*April 2026*
