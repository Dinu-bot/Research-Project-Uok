/**
 * @file telemetry_manager.cpp
 * @brief Telemetry Implementation
 */

#include "telemetry_manager.h"
#include <Arduino.h>

namespace telemetry {

TelemetryManager::TelemetryManager() 
    : linkMgr(nullptr), heartbeatEnabled(true), gpsEnabled(true),
      batteryEnabled(true), linkStatusEnabled(true),
      lastHeartbeat(0), lastGPS(0), lastBattery(0), lastLinkStatus(0) {
    memset(&data, 0, sizeof(data));
}

void TelemetryManager::begin() {
    lastHeartbeat = millis();
    lastGPS = millis();
    lastBattery = millis();
    lastLinkStatus = millis();
    Serial.println("[Telemetry] Manager initialized");
}

void TelemetryManager::update() {
    uint32_t now = millis();

    if (heartbeatEnabled && (now - lastHeartbeat >= TELEM_HEARTBEAT_MS)) {
        sendHeartbeat();
        lastHeartbeat = now;
    }

    if (gpsEnabled && (now - lastGPS >= TELEM_GPS_MS)) {
        sendGPS();
        lastGPS = now;
    }

    if (batteryEnabled && (now - lastBattery >= TELEM_BATTERY_MS)) {
        sendBattery();
        lastBattery = now;
    }

    if (linkStatusEnabled && (now - lastLinkStatus >= TELEM_LINK_STATUS_MS)) {
        sendLinkStatus();
        lastLinkStatus = now;
    }
}

void TelemetryManager::setGPS(float lat, float lon, const char* mgrs) {
    data.latitude = lat;
    data.longitude = lon;
    if (mgrs) {
        strncpy(data.mgrs, mgrs, sizeof(data.mgrs) - 1);
        data.mgrs[sizeof(data.mgrs) - 1] = '\0';
    }
    data.timestamp = now();
}

void TelemetryManager::setBattery(float voltage) {
    data.batteryVoltage = voltage;
    data.batteryPercent = voltageToPercent(voltage);
    // Estimate runtime: rough heuristic
    // 7000mAh pack, assume 200mA average draw
    float hours = (data.batteryPercent / 100.0f) * (7000.0f / 200.0f);
    data.estRuntimeMinutes = static_cast<uint16_t>(hours * 60.0f);
}

void TelemetryManager::setLinkStatus(float fsoV, int8_t wifiRssi, int8_t loraRssi, uint8_t active) {
    data.fsoVoltage = fsoV;
    data.wifiRSSI = wifiRssi;
    data.loraRSSI = loraRssi;
    data.activeLink = active;
    data.uptime = millis() / 1000;
}

void TelemetryManager::sendHeartbeat() {
    if (!linkMgr) return;

    uint8_t payload[32];
    size_t idx = 0;
    // Device ID (simplified)
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    memcpy(&payload[idx], mac, 6); idx += 6;
    // Sequence counter
    static uint16_t seq = 0;
    payload[idx++] = (seq >> 8) & 0xFF;
    payload[idx++] = seq & 0xFF;
    seq++;
    // Status byte
    payload[idx++] = 0x01;  // OK

    hlink::Packet pkt(hlink::TYPE_HEARTBEAT, hlink::Packet::nextSequence(), payload, idx);
    linkMgr->sendTo(fsm::LINK_LORA, pkt);
}

void TelemetryManager::sendGPS() {
    if (!linkMgr) return;

    uint8_t payload[32];
    size_t idx = 0;
    memcpy(&payload[idx], &data.latitude, 4); idx += 4;
    memcpy(&payload[idx], &data.longitude, 4); idx += 4;
    memcpy(&payload[idx], data.mgrs, 15); idx += 15;
    uint32_t ts = data.timestamp;
    memcpy(&payload[idx], &ts, 4); idx += 4;

    hlink::Packet pkt(hlink::TYPE_GPS, hlink::Packet::nextSequence(), payload, idx);
    linkMgr->sendTo(fsm::LINK_LORA, pkt);
}

void TelemetryManager::sendBattery() {
    if (!linkMgr) return;

    uint8_t payload[16];
    size_t idx = 0;
    memcpy(&payload[idx], &data.batteryVoltage, 4); idx += 4;
    payload[idx++] = data.batteryPercent;
    payload[idx++] = (data.estRuntimeMinutes >> 8) & 0xFF;
    payload[idx++] = data.estRuntimeMinutes & 0xFF;

    hlink::Packet pkt(hlink::TYPE_BATTERY, hlink::Packet::nextSequence(), payload, idx);
    linkMgr->sendTo(fsm::LINK_LORA, pkt);
}

void TelemetryManager::sendLinkStatus() {
    if (!linkMgr) return;

    uint8_t payload[32];
    size_t idx = 0;
    memcpy(&payload[idx], &data.fsoVoltage, 4); idx += 4;
    payload[idx++] = static_cast<uint8_t>(data.wifiRSSI + 128);  // Offset to unsigned
    payload[idx++] = static_cast<uint8_t>(data.loraRSSI + 128);
    payload[idx++] = data.activeLink;
    memcpy(&payload[idx], &data.uptime, 4); idx += 4;

    hlink::Packet pkt(hlink::TYPE_LINK_STATUS, hlink::Packet::nextSequence(), payload, idx);
    linkMgr->sendTo(fsm::LINK_LORA, pkt);
}

} // namespace telemetry
