/**
 * @file fso_link.h
 * @brief Free-Space Optical Link (Laser UART)
 * @description UART2-based laser communication with ADC envelope monitoring.
 */

#ifndef FSO_LINK_H
#define FSO_LINK_H

#include "link_manager.h"

namespace links {

class FSOLink : public LinkInterface {
public:
    FSOLink();

    bool begin() override;
    void end() override;
    bool isReady() const override { return ready; }

    bool send(const hlink::Packet& pkt) override;
    void update() override;

    int8_t getRSSI() const override;
    float getSignalQuality() const override;
    fsm::LinkType getType() const override { return fsm::LINK_FSO; }
    const char* getName() const override { return "FSO"; }
    const Stats& getStats() const override { return stats; }
    void resetStats() override { stats = {}; }

    // FSO-specific
    float readADC() const;              // Read envelope voltage
    bool isAligned() const;             // Check if signal detected
    void setLaser(bool on);             // Direct laser control (for alignment)
    void sendRawByte(uint8_t b);        // Direct UART byte

private:
    uint8_t txBuffer[HLINK_MAX_TOTAL_SIZE];
    uint8_t rxBuffer[HLINK_MAX_TOTAL_SIZE];
    size_t rxIndex;

    void processRX();
};

} // namespace links

#endif // FSO_LINK_H
