# SnakeLink v2.0 — Validation & Test Checklist

## Pre-Test Setup

- [ ] Both ESP32s flashed with v2.0 firmware
- [ ] Both laptops have v2.0 software installed
- [ ] Encryption keys exchanged and verified (fingerprint match)
- [ ] Batteries charged > 80%
- [ ] All wiring verified against WIRING_REFERENCE.md
- [ ] Serial monitors connected (115200 baud) for both ESP32s
- [ ] Antennas attached to both Ra-02 modules

---

## Phase 1: Power & Boot Tests

| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 1.1 | Measure MT3608 output | 5.0V ± 0.05V | ☐ | ☐ |
| 1.2 | Measure 3.3V rail | 3.3V ± 0.1V | ☐ | ☐ |
| 1.3 | Power on Device A | Boot sequence on serial, OLED shows "Boot" | ☐ | ☐ |
| 1.4 | Power on Device B | Same as 1.3 | ☐ | ☐ |
| 1.5 | Check NVS init | Serial: "Config loaded from NVS" | ☐ | ☐ |
| 1.6 | Check device ID | Unique 8-byte ID displayed | ☐ | ☐ |
| 1.7 | Check key fingerprint | 2-char hex shown on OLED | ☐ | ☐ |
| 1.8 | Verify fingerprints match | Both devices show same fingerprint | ☐ | ☐ |
| 1.9 | Battery reading | OLED shows 80-100% | ☐ | ☐ |
| 1.10 | Boot completion | OLED shows "READY" with signal bars | ☐ | ☐ |

---

## Phase 2: Individual Link Tests

### 2.1 Wi-Fi Link
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 2.1.1 | Laptop connects to AP | SSID "SnakeLink-XXXX" visible | ☐ | ☐ |
| 2.1.2 | Software shows "CONNECTED" | Status bar green | ☐ | ☐ |
| 2.1.3 | Send test packet | Serial shows TX/RX on both sides | ☐ | ☐ |
| 2.1.4 | RSSI reading | Dashboard shows -30 to -75 dBm | ☐ | ☐ |
| 2.1.5 | ESP-NOW peer comm | Packets received without laptop | ☐ | ☐ |
| 2.1.6 | 10 packets, 0% loss | All ACKs received | ☐ | ☐ |
| 2.1.7 | Throughput test | >100 kbps sustained | ☐ | ☐ |

### 2.2 FSO Link
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 2.2.1 | Enter alignment mode | Long-press Button 1 (1.5s) | ☐ | ☐ |
| 2.2.2 | Buzzer pitch changes | Higher pitch = stronger signal | ☐ | ☐ |
| 2.2.3 | OLED alignment % | Shows 0-100% | ☐ | ☐ |
| 2.2.4 | ADC voltage | 1.65V idle, >1.8V with signal | ☐ | ☐ |
| 2.2.5 | Send test packet via FSO | Serial shows FSO TX/RX | ☐ | ☐ |
| 2.2.6 | 10 packets, 0% loss | All ACKs received | ☐ | ☐ |
| 2.2.7 | Throughput test | >50 kbps at 115200 baud | ☐ | ☐ |
| 2.2.8 | Laser modulation | Oscilloscope shows UART pattern | ☐ | ☐ |

### 2.3 LoRa Link
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 2.3.1 | Module init | Serial: "LoRa initialized" | ☐ | ☐ |
| 2.3.2 | Heartbeat received | Every 5 seconds on both sides | ☐ | ☐ |
| 2.3.3 | RSSI reading | Dashboard shows -80 to -120 dBm | ☐ | ☐ |
| 2.3.4 | GPS broadcast | Every 30 seconds | ☐ | ☐ |
| 2.3.5 | Battery broadcast | Every 60 seconds | ☐ | ☐ |
| 2.3.6 | 10 packets, <10% loss | Acceptable for LoRa | ☐ | ☐ |
| 2.3.7 | Range test | 500m line-of-sight | ☐ | ☐ |

---

## Phase 3: FSM & Switching Tests

| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 3.1 | Default state | Both devices on Wi-Fi | ☐ | ☐ |
| 3.2 | Degrade Wi-Fi signal | Move laptop away / shield antenna | ☐ | ☐ |
| 3.3 | FSM enters WARNING | OLED shows "SWITCHING", LEDs flash | ☐ | ☐ |
| 3.4 | Packets duplicated | Serial shows TX on both Wi-Fi and FSO | ☐ | ☐ |
| 3.5 | Switch to FSO | OLED shows "FSO", blue LED slow blink | ☐ | ☐ |
| 3.6 | No packet loss | All 10 test packets received | ☐ | ☐ |
| 3.7 | Recover Wi-Fi | Move laptop closer | ☐ | ☐ |
| 3.8 | FSM switches back | After 5s stability, returns to Wi-Fi | ☐ | ☐ |
| 3.9 | Degrade both Wi-Fi and FSO | Shield both / increase distance | ☐ | ☐ |
| 3.10 | Fallback to LoRa | OLED shows "LORA ONLY" | ☐ | ☐ |
| 3.11 | Recover Wi-Fi | Remove shielding | ☐ | ☐ |
| 3.12 | Return to Wi-Fi | After stability window | ☐ | ☐ |
| 3.13 | Manual override | Press Button 2 to cycle links | ☐ | ☐ |
| 3.14 | Release override | Long-press Button 2 | ☐ | ☐ |

---

## Phase 4: Application Tests

### 4.1 Chat
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 4.1.1 | Send text message | Bubble appears, single ✓ | ☐ | ☐ |
| 4.1.2 | Receive text message | Bubble appears on other side, ✓✓ | ☐ | ☐ |
| 4.1.3 | Send Sinhala text | UTF-8 renders correctly | ☐ | ☐ |
| 4.1.4 | Send 240-byte message | Max payload, no truncation | ☐ | ☐ |
| 4.1.5 | High security mode | Messages encrypted, key verified | ☐ | ☐ |

### 4.2 File Transfer
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 4.2.1 | Send small file (<10KB) | Progress bar completes | ☐ | ☐ |
| 4.2.2 | Send large file (>1MB) | Chunked transfer, resume capable | ☐ | ☐ |
| 4.2.3 | Compression enabled | Smaller transfer size | ☐ | ☐ |
| 4.2.4 | Encrypted transfer | File hash matches | ☐ | ☐ |
| 4.2.5 | Cancel mid-transfer | Clean abort, no corruption | ☐ | ☐ |

### 4.3 Voice
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 4.3.1 | PTT press | Status shows "TRANSMITTING" | ☐ | ☐ |
| 4.3.2 | PTT release | Returns to "Ready" | ☐ | ☐ |
| 4.3.3 | Spacebar PTT | Same as button press | ☐ | ☐ |
| 4.3.4 | Receive voice | Level meter animates | ☐ | ☐ |

### 4.4 Emergency
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 4.4.1 | Trigger emergency | Confirmation dialog | ☐ | ☐ |
| 4.4.2 | Confirm emergency | Alarm on both devices | ☐ | ☐ |
| 4.4.3 | GPS coordinates sent | Other device shows coordinates | ☐ | ☐ |
| 4.4.4 | Cooldown active | Cannot re-trigger for 5s | ☐ | ☐ |
| 4.4.5 | Hardware trigger | Long-press Button 2 (1.5s) | ☐ | ☐ |

### 4.5 Dashboard
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 4.5.1 | Signal bars update | Real-time Wi-Fi/FSO/LoRa | ☐ | ☐ |
| 4.5.2 | Battery display | Shows own + other device | ☐ | ☐ |
| 4.5.3 | Packet counters | TX/RX increment correctly | ☐ | ☐ |
| 4.5.4 | Uptime counter | Increments every second | ☐ | ☐ |

### 4.6 Map
| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 4.6.1 | GPS received | Map shows other device position | ☐ | ☐ |
| 4.6.2 | Coordinates correct | Within 10m accuracy | ☐ | ☐ |

---

## Phase 5: Stress & Edge Cases

| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 5.1 | Rapid link switching | FSM stable, no crashes | ☐ | ☐ |
| 5.2 | High packet rate | >100 packets/sec, no drops | ☐ | ☐ |
| 5.3 | Low battery operation | System functional down to 3.3V | ☐ | ☐ |
| 5.4 | Battery critical | Warning displayed, no shutdown | ☐ | ☐ |
| 5.5 | CRC error injection | FSM switches link correctly | ☐ | ☐ |
| 5.6 | ACK timeout | Retransmit up to 3 times | ☐ | ☐ |
| 5.7 | Duplicate packets | Deduplication cache works | ☐ | ☐ |
| 5.8 | Buffer overflow | 241-byte payload rejected | ☐ | ☐ |
| 5.9 | Wrong encryption key | Decrypt fails, error logged | ☐ | ☐ |
| 5.10 | Reboot mid-transfer | Resume from last ACK | ☐ | ☐ |

---

## Phase 6: Field Simulation

| # | Test | Expected Result | Pass | Fail |
|---|------|-----------------|------|------|
| 6.1 | 100m Wi-Fi range | Stable connection | ☐ | ☐ |
| 6.2 | 50m FSO range | Alignment maintained | ☐ | ☐ |
| 6.3 | 500m LoRa range | Heartbeat received | ☐ | ☐ |
| 6.4 | Fog simulation | FSO degrades, FSM switches to Wi-Fi | ☐ | ☐ |
| 6.5 | Obstruction | Wi-Fi degrades, FSM switches to FSO | ☐ | ☐ |
| 6.6 | Both links blocked | Falls back to LoRa | ☐ | ☐ |
| 6.7 | 4-hour continuous | No crashes, battery >25% | ☐ | ☐ |
| 6.8 | Temperature test | Functional at 30-40°C | ☐ | ☐ |

---

## Sign-Off

**Test Date**: _______________  
**Tester Name**: _______________  
**Device A ID**: _______________  
**Device B ID**: _______________  

| Phase | Tests | Passed | Failed | Notes |
|-------|-------|--------|--------|-------|
| 1. Power & Boot | 10 | ___ | ___ | |
| 2. Individual Links | 21 | ___ | ___ | |
| 3. FSM & Switching | 14 | ___ | ___ | |
| 4. Application | 20 | ___ | ___ | |
| 5. Stress & Edge | 10 | ___ | ___ | |
| 6. Field Simulation | 8 | ___ | ___ | |
| **TOTAL** | **83** | **___** | **___** | |

**Overall Result**: ☐ PASS  ☐ FAIL (requires: >75 tests passed)

**Tester Signature**: _______________  
**Date**: _______________

---

*Test Checklist v2.0 — April 2026*
