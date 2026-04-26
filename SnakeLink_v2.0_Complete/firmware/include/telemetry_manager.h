/**
 * @file telemetry_manager.h
 * @brief Automated Telemetry (LoRa Background Channel)
 * @description Periodic GPS, battery, heartbeat, link status broadcasts.
 */

#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include "config.h"
#include "hlink_protocol.h"
#include "link_manager.h"

namespace telemetry {

struct TelemetryData {
    float latitude;
    float longitude;
    char mgrs[16];
    float batteryVoltage;
    uint8_t batteryPercent;
    uint16_t estRuntimeMinutes;
    float fsoVoltage;
    int8_t wifiRSSI;
    int8_t loraRSSI;
    uint8_t activeLink;
    uint32_t uptime;
    uint32_t timestamp;
};

class TelemetryManager {
public:
    TelemetryManager();

    void begin();
    void update();  // Call from background task

    // Set data sources
    void setGPS(float lat, float lon, const char* mgrs = nullptr);
    void setBattery(float voltage);
    void setLinkStatus(float fsoV, int8_t wifiRssi, int8_t loraRssi, uint8_t active);
    void setLinkManager(links::LinkManager* mgr) { linkMgr = mgr; }

    // Get current telemetry
    const TelemetryData& getData() const { return data; }

    // Enable/disable specific telemetry types
    void enableHeartbeat(bool en) { heartbeatEnabled = en; }
    void enableGPS(bool en) { gpsEnabled = en; }
    void enableBattery(bool en) { batteryEnabled = en; }
    void enableLinkStatus(bool en) { linkStatusEnabled = en; }

private:
    TelemetryData data;
    links::LinkManager* linkMgr;

    bool heartbeatEnabled;
    bool gpsEnabled;
    bool batteryEnabled;
    bool linkStatusEnabled;

    uint32_t lastHeartbeat;
    uint32_t lastGPS;
    uint32_t lastBattery;
    uint32_t lastLinkStatus;

    void sendHeartbeat();
    void sendGPS();
    void sendBattery();
    void sendLinkStatus();
};

} // namespace telemetry

#endif // TELEMETRY_MANAGER_H
