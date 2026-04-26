/**
 * @file display_manager.cpp
 * @brief OLED Display Implementation
 */

#include "display_manager.h"
#include <Arduino.h>

namespace display {

DisplayManager::DisplayManager() 
    : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1),
      currentState(DISP_BOOT), lastUpdate(0), stateEntryTime(0),
      activeLink(1), sigWifi(0), sigFSO(0), sigLoRa(0),
      batteryPct(100), batteryV(4.2f), txRate(0), rxRate(0),
      alignPct(0), bootStep(0) {
    memset(message, 0, sizeof(message));
}

bool DisplayManager::begin() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        Serial.println("[Display] SSD1306 init failed");
        return false;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    stateEntryTime = millis();
    Serial.println("[Display] SSD1306 initialized");
    return true;
}

void DisplayManager::update() {
    uint32_t now = millis();
    if (now - lastUpdate < DISPLAY_UPDATE_MS) return;
    lastUpdate = now;

    display.clearDisplay();

    // Always draw status bar and signal meters
    drawStatusBar();
    drawSignalMeters();

    // State-specific content
    switch (currentState) {
        case DISP_BOOT: drawBoot(); break;
        case DISP_IDLE: drawIdle(); break;
        case DISP_TRANSMIT: drawTransmit(); break;
        case DISP_RECEIVE: drawReceive(); break;
        case DISP_SWITCHING: drawSwitching(); break;
        case DISP_ALIGN: drawAlign(); break;
        case DISP_LORA_EMERGENCY: drawLoRaEmergency(); break;
        case DISP_EMERGENCY_RX: drawEmergencyRX(); break;
        case DISP_BATTERY_WARN: drawBatteryWarn(); break;
        default: drawIdle(); break;
    }

    drawDataTicker();
    display.display();
}

void DisplayManager::setState(DisplayState state) {
    if (state != currentState) {
        currentState = state;
        stateEntryTime = millis();
    }
}

void DisplayManager::drawStatusBar() {
    // Zone 1: 0-10px
    display.setCursor(0, 0);

    // Heartbeat dot (blink every 500ms)
    if ((millis() / 500) % 2 == 0) {
        display.print("*");
    } else {
        display.print(" ");
    }

    // Active link badge
    const char* linkName = "---";
    switch (activeLink) {
        case 1: linkName = "WIFI"; break;
        case 2: linkName = "FSO "; break;
        case 3: linkName = "LORA"; break;
    }
    display.print(linkName);
    display.print(" ");

    // Battery icon
    if (batteryPct > 75) display.print("[|||]");
    else if (batteryPct > 50) display.print("[|| ]");
    else if (batteryPct > 25) display.print("[|  ]");
    else display.print("[!  ]");

    display.print(" ");
    display.print(batteryPct);
    display.print("%");
}

void DisplayManager::drawSignalMeters() {
    // Zone 3: 39-52px (drawn at bottom of main area)
    int y = 52;
    int barW = 20;
    int gap = 8;
    int startX = 10;

    // Wi-Fi
    display.setCursor(startX, y);
    display.print("W");
    display.drawRect(startX, y+8, barW, 4, SSD1306_WHITE);
    display.fillRect(startX, y+8, (int)(sigWifi * barW), 4, SSD1306_WHITE);

    // FSO
    display.setCursor(startX + barW + gap, y);
    display.print("F");
    display.drawRect(startX + barW + gap, y+8, barW, 4, SSD1306_WHITE);
    display.fillRect(startX + barW + gap, y+8, (int)(sigFSO * barW), 4, SSD1306_WHITE);

    // LoRa
    display.setCursor(startX + 2*(barW + gap), y);
    display.print("L");
    display.drawRect(startX + 2*(barW + gap), y+8, barW, 4, SSD1306_WHITE);
    display.fillRect(startX + 2*(barW + gap), y+8, (int)(sigLoRa * barW), 4, SSD1306_WHITE);

    // Active indicator
    int activeX = startX;
    if (activeLink == 2) activeX += barW + gap;
    else if (activeLink == 3) activeX += 2*(barW + gap);
    display.drawLine(activeX - 2, y+14, activeX + barW + 2, y+14, SSD1306_WHITE);
}

void DisplayManager::drawDataTicker() {
    // Zone 4: 53-63px
    int y = 56;
    static uint8_t tick = 0;
    tick = (tick + 1) % 4;

    display.setCursor(0, y);
    switch (tick) {
        case 0: display.printf("TX:%lu RX:%lu", txRate, rxRate); break;
        case 1: display.printf("BAT:%.2fV %u%%", batteryV, batteryPct); break;
        case 2: display.printf("UP:%lus", millis()/1000); break;
        case 3: display.printf("LINK:%s", activeLink==1?"W":(activeLink==2?"F":"L")); break;
    }
}

void DisplayManager::drawBoot() {
    display.setCursor(20, 20);
    display.setTextSize(1);
    display.print("SnakeLink v2.0");
    display.setCursor(10, 32);
    display.print("Booting...");

    // Progress dots
    int dots = (millis() - stateEntryTime) / 300;
    for (int i = 0; i < dots % 4; i++) {
        display.print(".");
    }
}

void DisplayManager::drawIdle() {
    display.setCursor(20, 20);
    display.print("READY");
    if (message[0]) {
        display.setCursor(0, 32);
        display.print(message);
    }
}

void DisplayManager::drawTransmit() {
    display.setCursor(10, 20);
    display.print(">>> TX <<<");
    display.setCursor(0, 32);
    display.print(message);
}

void DisplayManager::drawReceive() {
    display.setCursor(10, 20);
    display.print("<<< RX >>>");
    display.setCursor(0, 32);
    display.print(message);
}

void DisplayManager::drawSwitching() {
    display.setCursor(5, 20);
    display.print("SWITCHING...");
    // Animated bar
    int progress = ((millis() - stateEntryTime) * 128) / 1000;
    if (progress > 128) progress = 128;
    display.drawRect(0, 35, 128, 6, SSD1306_WHITE);
    display.fillRect(2, 37, progress - 4, 2, SSD1306_WHITE);
}

void DisplayManager::drawAlign() {
    display.setCursor(10, 12);
    display.print("ALIGN MODE");
    display.setCursor(40, 28);
    display.setTextSize(2);
    display.printf("%u%%", alignPct);
    display.setTextSize(1);

    // Bar
    display.drawRect(10, 48, 108, 6, SSD1306_WHITE);
    display.fillRect(12, 50, (alignPct * 104) / 100, 2, SSD1306_WHITE);
}

void DisplayManager::drawLoRaEmergency() {
    display.setCursor(5, 18);
    display.print("LORA ONLY");
    display.setCursor(10, 30);
    display.print("LIMITED MODE");

    // Warning flash
    if ((millis() / 500) % 2 == 0) {
        display.fillRect(0, 0, 128, 64, SSD1306_INVERSE);
    }
}

void DisplayManager::drawEmergencyRX() {
    display.setCursor(5, 12);
    display.print("EMERGENCY!");
    display.setCursor(0, 28);
    display.print(message);

    // Inverted flash
    if ((millis() / 300) % 2 == 0) {
        display.invertDisplay(true);
    } else {
        display.invertDisplay(false);
    }
}

void DisplayManager::drawBatteryWarn() {
    display.setCursor(10, 20);
    display.print("LOW BATTERY");
    display.setCursor(30, 32);
    display.printf("%u%%", batteryPct);
}

void DisplayManager::showBootStep(const char* step, bool ok) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Boot:");
    display.setCursor(0, 12);
    display.print(step);
    display.setCursor(0, 24);
    display.print(ok ? "OK" : "FAIL");
    display.display();
    delay(200);
}

void DisplayManager::setActiveLink(uint8_t link) { activeLink = link; }
void DisplayManager::setSignalBars(float wifi, float fso, float lora) {
    sigWifi = wifi; sigFSO = fso; sigLoRa = lora;
}
void DisplayManager::setBattery(uint8_t pct, float voltage) {
    batteryPct = pct; batteryV = voltage;
}
void DisplayManager::setDataRate(uint32_t tx, uint32_t rx) {
    txRate = tx; rxRate = rx;
}
void DisplayManager::setMessage(const char* msg) {
    strncpy(message, msg, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
}
void DisplayManager::setAlignmentPercent(uint8_t pct) {
    alignPct = pct;
}

} // namespace display
