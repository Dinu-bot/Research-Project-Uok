/**
 * @file wifi_link.h
 * @brief Wi-Fi Link Implementation (ESP-NOW + AP for laptop)
 */

#ifndef WIFI_LINK_H
#define WIFI_LINK_H

#include "link_manager.h"
#include <WiFi.h>
#include <esp_now.h>

namespace links {

class WiFiLink : public LinkInterface {
public:
    WiFiLink();

    bool begin() override;
    void end() override;
    bool isReady() const override { return ready && apActive && peerRegistered; }

    bool send(const hlink::Packet& pkt) override;
    void update() override;

    int8_t getRSSI() const override;
    float getSignalQuality() const override;
    fsm::LinkType getType() const override { return fsm::LINK_WIFI; }
    const char* getName() const override { return "WiFi"; }
    const Stats& getStats() const override { return stats; }
    void resetStats() override { stats = {}; }

    // ESP-NOW peer management
    bool setPeerMAC(const uint8_t* mac);

    // Laptop UDP
    bool hasLaptopConnection() const { return laptopConnected; }

private:
    bool apActive;
    bool peerRegistered;
    bool laptopConnected;
    uint8_t peerMAC[6];
    WiFiUDP udp;

    // Receive buffer
    uint8_t rxBuffer[HLINK_MAX_TOTAL_SIZE];
    size_t rxIndex;

    static void onESPNOWRecv(const uint8_t* mac, const uint8_t* data, int len);
    static void onESPNOWSent(const uint8_t* mac, esp_now_send_status_t status);

    void processUDPPacket();
    void processESPNOWPacket(const uint8_t* data, int len);
};

} // namespace links

#endif // WIFI_LINK_H
