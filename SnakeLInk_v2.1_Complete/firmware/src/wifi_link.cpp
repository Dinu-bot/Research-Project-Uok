/**
 * @file wifi_link.cpp
 * @brief Wi-Fi Link Implementation
 */

#include "wifi_link.h"
#include <Arduino.h>

namespace links {

static WiFiLink* instance = nullptr;

WiFiLink::WiFiLink() : apActive(false), peerRegistered(false), laptopConnected(false), rxIndex(0) {
    memset(peerMAC, 0, sizeof(peerMAC));
    memset(rxBuffer, 0, sizeof(rxBuffer));
}

bool WiFiLink::begin() {
    instance = this;

    // Initialize Wi-Fi in AP+STA mode
    WiFi.mode(WIFI_AP_STA);

    // Create AP for laptop connection
    char ssid[32];
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(ssid, sizeof(ssid), "%s-%02X%02X", WIFI_AP_SSID_PREFIX, mac[4], mac[5]);

    if (!WiFi.softAP(ssid, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN)) {
        Serial.println("[WiFi] AP creation failed");
        return false;
    }

    IPAddress IP = WiFi.softAPIP();
    Serial.printf("[WiFi] AP '%s' started at %s\n", ssid, IP.toString().c_str());

    // Initialize ESP-NOW for inter-device
    if (esp_now_init() != ESP_OK) {
        Serial.println("[WiFi] ESP-NOW init failed");
        return false;
    }

    esp_now_register_recv_cb(onESPNOWRecv);
    esp_now_register_send_cb(onESPNOWSent);

    // Add broadcast peer initially (will be replaced with actual peer)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMAC, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    esp_now_add_peer(&peerInfo);

    // Start UDP server for laptop
    udp.begin(UDP_LOCAL_PORT);
    Serial.printf("[WiFi] UDP server on port %d\n", UDP_LOCAL_PORT);

    apActive = true;
    ready = true;
    return true;
}

void WiFiLink::end() {
    esp_now_deinit();
    WiFi.softAPdisconnect(true);
    udp.stop();
    apActive = false;
    ready = false;
}

bool WiFiLink::setPeerMAC(const uint8_t* mac) {
    memcpy(peerMAC, mac, 6);

    // Remove old peer if exists
    esp_now_del_peer(peerMAC);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMAC, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        peerRegistered = true;
        Serial.printf("[WiFi] Peer registered: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return true;
    }
    return false;
}

bool WiFiLink::send(const hlink::Packet& pkt) {
    if (!isReady()) return false;

    uint8_t buffer[HLINK_MAX_TOTAL_SIZE];
    int len = pkt.serialize(buffer, sizeof(buffer));
    if (len <= 0) return false;

    // Send via ESP-NOW to peer transceiver
    esp_err_t err = esp_now_send(peerMAC, buffer, len);
    if (err == ESP_OK) {
        stats.txPackets++;
        stats.txBytes += len;
        stats.lastActivity = millis();
        return true;
    }
    stats.errors++;
    return false;
}

void WiFiLink::update() {
    // Check for UDP packets from laptop
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        laptopConnected = true;
        uint8_t buffer[UDP_BUFFER_SIZE];
        int len = udp.read(buffer, sizeof(buffer));
        if (len > 0) {
            // Laptop sends raw H-Link packets via UDP
            processUDPPacket();
        }
    }

    // Process accumulated ESP-NOW data in rxBuffer
    // (handled by callback)
}

void WiFiLink::processUDPPacket() {
    // Forward laptop packets to active inter-device link
    // This is handled by the main data task
}

void WiFiLink::processESPNOWPacket(const uint8_t* data, int len) {
    hlink::Packet pkt;
    int consumed = pkt.deserialize(data, len);
    if (consumed > 0 && pkt.isValid()) {
        stats.rxPackets++;
        stats.rxBytes += consumed;
        stats.lastActivity = millis();
        if (packetHandler) packetHandler(pkt);
    } else {
        stats.errors++;
    }
}

int8_t WiFiLink::getRSSI() const {
    if (!apActive) return -100;
    // For ESP-NOW, we can get RSSI from the last received packet
    // This is a simplified version
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return -80;  // Default assumption
}

float WiFiLink::getSignalQuality() const {
    int8_t rssi = getRSSI();
    if (rssi >= -50) return 1.0f;
    if (rssi <= -90) return 0.0f;
    return (rssi + 90) / 40.0f;
}

void WiFiLink::onESPNOWRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (instance) instance->processESPNOWPacket(data, len);
}

void WiFiLink::onESPNOWSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS && instance) {
        instance->stats.errors++;
    }
}

} // namespace links
