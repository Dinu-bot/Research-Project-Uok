# SnakeLink v2.0 — Troubleshooting Guide

## Diagnostic Flowchart

```
Device won't boot?
    ├── Check power: Multimeter on VIN = 5.0V?
    │   ├── NO → Check MT3608, battery, TP4056
    │   └── YES → Check serial output at 115200 baud
    │       ├── No serial output → Flash firmware again
    │       └── Boot messages visible → Continue below
    │
OLED shows "Boot" but hangs?
    ├── Check I2C: Scan with `Wire.scan()`
    │   └── No device at 0x3C → Check SDA/SCL wiring
    │
Wi-Fi AP not visible?
    ├── Check ESP32 Wi-Fi radio
    │   └── Serial shows "AP creation failed" → Hardware fault
    │
LoRa not initializing?
    ├── Check 3.3V rail at module
    │   ├── <3.2V → Power issue
    │   └── OK → Check SPI wiring, RST pin (GPIO 14)
    │       └── Still failing → Try manual reset pulse
    │
Laser not modulating?
    ├── Check GPIO 17 with oscilloscope/LED
    │   ├── No signal → Firmware issue
    │   └── Signal present → Check 2N7000, laser diode
    │
No FSO reception?
    ├── Check CA3140E output (should be ~2.5V idle)
    │   ├── Wrong voltage → Check BPW34 bias, feedback resistor
    │   └── OK → Check LM393 threshold, output pull-up
    │       └── OK → Check UART2 config (GPIO 16/17)
```

---

## Common Issues

### Issue 1: ESP32 Brownout During Wi-Fi Operation
**Symptoms**: Reboot loop, "Brownout detector triggered" in serial output

**Root Cause**: Power supply cannot deliver peak current during Wi-Fi transmission

**Solutions**:
1. Verify MT3608 output = 5.0V under load
2. Add 1000µF electrolytic capacitor across MT3608 output
3. Ensure battery can deliver >2A peak (use high-drain 18650s)
4. Check breadboard contacts — high resistance causes voltage drop

**Prevention**: Always use MT3608 boost to 5V, never feed 3.7V directly to VIN

---

### Issue 2: FSO False Link Switches
**Symptoms**: System rapidly switches between Wi-Fi and FSO, OLED shows "SWITCHING" frequently

**Root Cause**: Transient noise on ADC or threshold set too aggressively

**Solutions**:
1. Increase EMA alpha in `config.h`: `#define EMA_ALPHA 0.1f` (slower response)
2. Increase stability window: `#define WIFI_RSSI_STABLE_MS 10000`
3. Check FSO ADC wiring — ensure shielded cable or short traces
4. Add 10µF capacitor across BPW34 bias for noise filtering
5. Verify LM393 threshold trimpot is not at extreme position

---

### Issue 3: LoRa Module Not Responding
**Symptoms**: "Module init failed" in serial output, no LoRa packets

**Root Cause**: SPI communication failure or module not resetting

**Solutions**:
1. Verify 3.3V at Ra-02 VCC pin (NOT 5V)
2. Check SPI wiring: SCK=18, MISO=19, MOSI=23, NSS=5
3. Verify GPIO 14 RST pulse in `lora_link.cpp`:
   ```cpp
   digitalWrite(PIN_LORA_RST, LOW); delay(10);
   digitalWrite(PIN_LORA_RST, HIGH); delay(10);
   ```
4. Try manual reset: Ground RST pin for 100ms, then release
5. Check antenna is connected (transmitting without antenna damages PA)
6. Verify module frequency matches region (433E6 for Sri Lanka)

---

### Issue 4: Buttons Not Registering
**Symptoms**: No response to button presses, no mode changes

**Root Cause**: Wrong GPIO assignment or missing pull-up

**Solutions**:
1. Verify Corrected Guide v2 pinout:
   - Button 1 (Align): GPIO 32
   - Button 2 (Mode): GPIO 13
2. Check `INPUT_PULLUP` is configured in `button_manager.cpp`
3. Test with multimeter: Pin should read 3.3V when open, 0V when pressed
4. Check for short to ground (damaged button or wiring)
5. Verify debounce time is not too long: 50ms standard

---

### Issue 5: OLED Display Corruption
**Symptoms**: Garbled pixels, partial display, flickering

**Root Cause**: I2C bus noise or insufficient pull-ups

**Solutions**:
1. Add 4.7kΩ pull-up resistors on SDA (GPIO 21) and SCL (GPIO 22)
2. Check I2C address — some modules use 0x3D instead of 0x3C
3. Reduce I2C clock speed: `Wire.setClock(100000)`
4. Check for loose connections on breadboard
5. Verify 3.3V supply is stable (add 100nF decoupling near OLED)

---

### Issue 6: Encryption Key Mismatch
**Symptoms**: "Decrypt/auth failed" in serial output, messages not readable

**Root Cause**: Devices have different encryption keys

**Solutions**:
1. Check key fingerprint on both OLEDs (first 2 bytes of key)
2. If mismatch, reset NVS on both devices:
   ```cpp
   // In serial console
   cfgMgr.reset();
   ```
3. Re-pair devices by generating new shared key
4. In software Settings, verify key matches transceiver

---

### Issue 7: File Transfer Fails Midway
**Symptoms**: Progress bar stops, "Cancelled" or timeout message

**Root Cause**: Link degradation during transfer, ACK timeout

**Solutions**:
1. Check active link signal quality on dashboard
2. If on LoRa: reduce chunk size or increase timeout
3. Enable compression to reduce transfer time
4. Check for duplicate SEQ numbers (deduplication cache full)
5. Verify receiver has sufficient disk space

---

### Issue 8: Emergency Broadcast Not Working
**Symptoms**: Long-press button 2 but no alarm on receiver

**Root Cause**: Packet not reaching receiver on any link

**Solutions**:
1. Verify button long-press detection (1.5s hold)
2. Check all three links are initialized
3. Verify `FLAG_BROADCAST` is set in packet
4. Check receiver is not in sleep mode
5. Test individual links first (send test packet on each)

---

### Issue 9: High CRC Error Rate
**Symptoms**: Dashboard shows >30% CRC errors, frequent retransmissions

**Root Cause**: Noisy environment, weak signal, or timing issues

**Solutions**:
1. For FSO: Check alignment, clean lenses, reduce distance
2. For Wi-Fi: Reduce distance, clear line of sight
3. For LoRa: Increase spreading factor (SF7→SF12)
4. Check for interference sources (microwave ovens, other 433MHz devices)
5. Verify baud rate matches on both ends (FSO UART)

---

### Issue 10: Laptop Cannot Connect to ESP32 AP
**Symptoms**: "DISCONNECTED" in software, AP not in Wi-Fi list

**Root Cause**: Wi-Fi channel conflict or authentication issue

**Solutions**:
1. Check laptop Wi-Fi is enabled and not connected to another network
2. Verify AP SSID: `SnakeLink-XXXX` (last 4 digits of MAC)
3. Try manual connection with password: `Tactical2026`
4. Change Wi-Fi channel in `config.h` if congested (default: 6)
5. Check firewall is not blocking UDP port 4210
6. Verify ESP32 Wi-Fi radio is not damaged (test with phone hotspot)

---

## Serial Debug Commands

Connect to ESP32 serial monitor (115200 baud) and type:

```
status      → Show current FSM state, link metrics, battery
links       → Show all link statistics (TX/RX/errors)
fsm         → Show FSM thresholds and hysteresis timers
crypto      → Show encryption status and key fingerprint
config      → Show NVS configuration
reset       → Reset to factory defaults
reboot      → Soft reboot
```

*Note: Debug commands require `LOG_LEVEL >= LOG_LEVEL_INFO`*

---

## Emergency Recovery

If device is completely unresponsive:

1. **Power cycle**: Turn master switch OFF, wait 5s, turn ON
2. **Hard reset**: Press ESP32 EN button for 1s
3. **Factory reset**: Hold both buttons during boot (GPIO 32 + GPIO 13)
4. **Reflash firmware**: Connect USB, hold BOOT button, press EN, release BOOT
5. **Erase flash**: `pio run --target erase` then reflash

---

*Troubleshooting Guide v2.0 — April 2026*
