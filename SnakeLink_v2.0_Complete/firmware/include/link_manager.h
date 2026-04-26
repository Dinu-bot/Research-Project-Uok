/**
 * @file link_manager.h
 * @brief Abstract Link Interface & Routing Manager
 * @description Polymorphic link drivers with Make-Before-Break support.
 */

#ifndef LINK_MANAGER_H
#define LINK_MANAGER_H

#include "config.h"
#include "hlink_protocol.h"
#include "fsm_manager.h"
#include <functional>

namespace links {

using PacketHandler = std::function<void(const hlink::Packet&)>;
using LinkStatusCallback = std::function<void(fsm::LinkType, bool)>;

// ============================================================================
// ABSTRACT LINK INTERFACE
// ============================================================================
class LinkInterface {
public:
    virtual ~LinkInterface() = default;

    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual bool isReady() const = 0;

    // Send a packet (returns true if queued/transmitted)
    virtual bool send(const hlink::Packet& pkt) = 0;

    // Receive processing (call frequently)
    virtual void update() = 0;

    // Signal quality
    virtual int8_t getRSSI() const = 0;
    virtual float getSignalQuality() const = 0;  // 0.0 - 1.0

    // Link type
    virtual fsm::LinkType getType() const = 0;
    virtual const char* getName() const = 0;

    // Statistics
    struct Stats {
        uint32_t txPackets;
        uint32_t rxPackets;
        uint32_t txBytes;
        uint32_t rxBytes;
        uint32_t errors;
        uint32_t dropped;
        uint32_t retransmits;
        uint32_t lastActivity;
    };
    virtual const Stats& getStats() const = 0;
    virtual void resetStats() = 0;

    // Set callback for received packets
    void onPacketReceived(PacketHandler handler) { packetHandler = handler; }
    void onStatusChanged(LinkStatusCallback handler) { statusHandler = handler; }

protected:
    PacketHandler packetHandler;
    LinkStatusCallback statusHandler;
    Stats stats = {};
    bool ready = false;
};

// ============================================================================
// LINK MANAGER (routes packets to active link, handles duplication)
// ============================================================================
class LinkManager {
public:
    LinkManager();

    bool begin();
    void end();

    // Register links
    void registerLink(LinkInterface* link);

    // Set FSM reference for routing decisions
    void setFSM(fsm::FSMManager* fsm) { fsmMgr = fsm; }

    // Send packet via active link (or broadcast)
    bool send(const hlink::Packet& pkt);
    bool sendTo(fsm::LinkType link, const hlink::Packet& pkt);
    bool broadcast(const hlink::Packet& pkt);  // Send on ALL links

    // Make-Before-Break duplication
    bool sendDuplicate(const hlink::Packet& pkt);  // Send on active + backup

    // Update all links
    void update();

    // Get link by type
    LinkInterface* getLink(fsm::LinkType type);
    LinkInterface* getActiveLink();
    LinkInterface* getBackupLink();

    // Packet handler (from links → laptop/FSM)
    void onPacketReceived(const hlink::Packet& pkt);
    void setPacketHandler(PacketHandler handler) { packetHandler = handler; }

    // Emergency broadcast (bypasses FSM, sends on all links)
    bool sendEmergency(const hlink::Packet& pkt);

private:
    LinkInterface* links[4];  // Indexed by LinkType
    fsm::FSMManager* fsmMgr;
    PacketHandler packetHandler;

    // Deduplication cache (for MBB)
    struct DedupEntry {
        uint16_t seq;
        uint32_t timestamp;
    };
    static constexpr size_t DEDUP_SIZE = 16;
    DedupEntry dedupCache[DEDUP_SIZE];
    size_t dedupIndex;

    bool isDuplicate(uint16_t seq);
    void addDedup(uint16_t seq);
};

} // namespace links

#endif // LINK_MANAGER_H
