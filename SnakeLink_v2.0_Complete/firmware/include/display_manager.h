/**
 * @file display_manager.h
 * @brief SSD1306 OLED Display Manager
 * @description Async, zone-based display with fighter-jet HUD philosophy.
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "config.h"
#include <Adafruit_SSD1306.h>

namespace display {

enum DisplayState {
    DISP_BOOT = 0,
    DISP_IDLE,
    DISP_TRANSMIT,
    DISP_RECEIVE,
    DISP_SWITCHING,
    DISP_ALIGN,
    DISP_LORA_EMERGENCY,
    DISP_EMERGENCY_RX,
    DISP_BATTERY_WARN,
    DISP_CRC_ERROR
};

class DisplayManager {
public:
    DisplayManager();

    bool begin();
    void update();  // Call frequently, handles async refresh

    // State changes
    void setState(DisplayState state);
    void setActiveLink(uint8_t link);  // 1=WiFi, 2=FSO, 3=LoRa
    void setSignalBars(float wifi, float fso, float lora);  // 0.0-1.0
    void setBattery(uint8_t percent, float voltage);
    void setDataRate(uint32_t tx, uint32_t rx);
    void setMessage(const char* msg);
    void setAlignmentPercent(uint8_t pct);

    // Boot sequence
    void showBootStep(const char* step, bool ok);

private:
    Adafruit_SSD1306 display;
    DisplayState currentState;
    uint32_t lastUpdate;
    uint32_t stateEntryTime;

    // Display data
    uint8_t activeLink;
    float sigWifi, sigFSO, sigLoRa;
    uint8_t batteryPct;
    float batteryV;
    uint32_t txRate, rxRate;
    char message[32];
    uint8_t alignPct;

    // Boot
    uint8_t bootStep;

    void drawBoot();
    void drawIdle();
    void drawTransmit();
    void drawReceive();
    void drawSwitching();
    void drawAlign();
    void drawLoRaEmergency();
    void drawEmergencyRX();
    void drawBatteryWarn();
    void drawStatusBar();
    void drawSignalMeters();
    void drawDataTicker();
};

} // namespace display

#endif // DISPLAY_MANAGER_H
