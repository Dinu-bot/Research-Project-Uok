# SnakeLink v2.0 — Quick Start Guide

## 5-Minute Setup

### 1. Flash Firmware (2 minutes)
```bash
cd firmware
pio run --target upload
```

### 2. Install Software (2 minutes)
```bash
cd software
pip install -r requirements.txt
```

### 3. Connect & Run (1 minute)
```bash
# Connect laptop to Wi-Fi AP: SnakeLink-XXXX
# Password: Tactical2026
python main.py
```

## First Use Checklist
- [ ] Both ESP32s powered on and showing "READY" on OLED
- [ ] Key fingerprints match on both OLEDs
- [ ] Laptop connected to transceiver Wi-Fi
- [ ] Software shows "TRANSCEIVER CONNECTED"
- [ ] Send test message in Chat tab
- [ ] Verify delivery ticks (✓ → ✓✓)

## Common Commands

### Serial Monitor (ESP32)
```bash
pio device monitor --baud 115200
```

### Reset to Factory Defaults
Hold both buttons (GPIO 32 + GPIO 13) during boot.

### Emergency Broadcast
- **Software**: Click red "EMERGENCY" button
- **Hardware**: Long-press Button 2 (GPIO 13) for 1.5 seconds

## Next Steps
- Read [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md) for field setup
- Read [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for issues
- Read [WIRING_REFERENCE.md](WIRING_REFERENCE.md) for hardware
- Run through [TEST_CHECKLIST.md](TEST_CHECKLIST.md) for validation

---
*Quick Start v2.0 — April 2026*
