# Firmware Libraries

These libraries are automatically fetched by PlatformIO via `lib_deps` in `platformio.ini`:

| Library | Version | Purpose |
|---------|---------|---------|
| LoRa (sandeepmistry) | ^0.8.0 | SX1278 LoRa driver |
| Adafruit SSD1306 | ^2.5.9 | OLED display driver |
| Adafruit GFX | ^1.11.9 | Graphics primitives |
| Adafruit BusIO | ^1.14.5 | I2C/SPI abstraction |
| ArduinoJson | ^6.21.3 | JSON serialization |

Built-in (no install needed):
- mbedTLS (AES-256-GCM hardware acceleration)
- ESP-NOW (Wi-Fi peer-to-peer)
- FreeRTOS (dual-core tasking)
- NVS (non-volatile storage)
- SPIFFS (filesystem)

No manual library installation required — PlatformIO handles everything.
