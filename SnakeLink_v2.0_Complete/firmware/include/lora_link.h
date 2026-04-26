/**
 * @file lora_link.h
 * @brief LoRa Link Implementation (SX1278 via Ra-02)
 * @description Background telemetry + emergency fallback.
 */

#ifndef LORA_LINK_H
#define LORA_LINK_H

#include "link_manager.h"
#include <LoRa.h>

namespace links {

class LoRaLink : public LinkInterface {
public:
    LoRaLink();

    bool begin() override;
    void end() override;
    bool isReady() const override { return ready; }

    bool send(const hlink::Packet& pkt) override;
    void update() override;

    int8_t getRSSI() const override;
    float getSignalQuality() const override;
    fsm::LinkType getType() const override { return fsm::LINK_LORA; }
    const char* getName() const override { return "LoRa"; }
    const Stats& getStats() const override { return stats; }
    void resetStats() override { stats = {}; }

    // LoRa-specific
    float getSNR() const { return lastSNR; }
    bool isTransmitting() const { return txPending; }

private:
    bool txPending;
    float lastRSSI;
    float lastSNR;

    uint8_t txBuffer[HLINK_MAX_TOTAL_SIZE];
    uint8_t rxBuffer[HLINK_MAX_TOTAL_SIZE];
    size_t rxIndex;

    void onReceive(int packetSize);
    void processRX();
};

} // namespace links

#endif // LORA_LINK_H
