/**
 * @file link_manager.cpp
 * @brief Link Routing & Make-Before-Break Implementation
 */

#include "link_manager.h"
#include <Arduino.h>

namespace links {

// ============================================================================
// LINK MANAGER
// ============================================================================
LinkManager::LinkManager() : fsmMgr(nullptr), dedupIndex(0) {
    memset(links, 0, sizeof(links));
    memset(dedupCache, 0, sizeof(dedupCache));
}

bool LinkManager::begin() {
    Serial.println("[LinkMgr] Initializing...");
    for (int i = 0; i < 4; i++) {
        if (links[i] && !links[i]->begin()) {
            Serial.printf("[LinkMgr] WARNING: %s failed to init\n", links[i]->getName());
        }
    }
    return true;
}

void LinkManager::end() {
    for (int i = 0; i < 4; i++) {
        if (links[i]) links[i]->end();
    }
}

void LinkManager::registerLink(LinkInterface* link) {
    if (!link) return;
    int idx = static_cast<int>(link->getType());
    if (idx >= 0 && idx < 4) {
        links[idx] = link;
        Serial.printf("[LinkMgr] Registered %s\n", link->getName());
    }
}

bool LinkManager::send(const hlink::Packet& pkt) {
    if (!fsmMgr) return false;

    if (pkt.header.flags & hlink::FLAG_BROADCAST) {
        return broadcast(pkt);
    }

    if (fsmMgr->shouldDuplicate()) {
        return sendDuplicate(pkt);
    }

    fsm::LinkType active = fsmMgr->getActiveLink();
    return sendTo(active, pkt);
}

bool LinkManager::sendTo(fsm::LinkType link, const hlink::Packet& pkt) {
    int idx = static_cast<int>(link);
    if (idx < 0 || idx >= 4 || !links[idx]) return false;
    if (!links[idx]->isReady()) return false;

    return links[idx]->send(pkt);
}

bool LinkManager::broadcast(const hlink::Packet& pkt) {
    bool any = false;
    for (int i = 1; i < 4; i++) {  // Skip LINK_NONE
        if (links[i] && links[i]->isReady()) {
            if (links[i]->send(pkt)) any = true;
        }
    }
    return any;
}

bool LinkManager::sendDuplicate(const hlink::Packet& pkt) {
    fsm::LinkType active = fsmMgr->getActiveLink();
    fsm::LinkType backup = fsmMgr->getBackupLink();

    bool ok1 = sendTo(active, pkt);
    bool ok2 = sendTo(backup, pkt);

    if (ok1 || ok2) {
        // Only count as TX on active link for stats
        // Backup TX is "overhead" for zero-loss guarantee
        return true;
    }
    return false;
}

void LinkManager::update() {
    for (int i = 0; i < 4; i++) {
        if (links[i]) links[i]->update();
    }
}

LinkInterface* LinkManager::getLink(fsm::LinkType type) {
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < 4) return links[idx];
    return nullptr;
}

LinkInterface* LinkManager::getActiveLink() {
    if (!fsmMgr) return nullptr;
    return getLink(fsmMgr->getActiveLink());
}

LinkInterface* LinkManager::getBackupLink() {
    if (!fsmMgr) return nullptr;
    return getLink(fsmMgr->getBackupLink());
}

void LinkManager::onPacketReceived(const hlink::Packet& pkt) {
    // Deduplication for Make-Before-Break
    if (isDuplicate(pkt.header.seq)) {
        return;  // Silently drop duplicate
    }
    addDedup(pkt.header.seq);

    if (packetHandler) {
        packetHandler(pkt);
    }
}

bool LinkManager::sendEmergency(const hlink::Packet& pkt) {
    return broadcast(pkt);
}

bool LinkManager::isDuplicate(uint16_t seq) {
    uint32_t now = millis();
    for (size_t i = 0; i < DEDUP_SIZE; i++) {
        if (dedupCache[i].seq == seq && 
            (now - dedupCache[i].timestamp) < MBB_DUPLICATE_MAX_AGE_MS) {
            return true;
        }
    }
    return false;
}

void LinkManager::addDedup(uint16_t seq) {
    dedupCache[dedupIndex].seq = seq;
    dedupCache[dedupIndex].timestamp = millis();
    dedupIndex = (dedupIndex + 1) % DEDUP_SIZE;
}

} // namespace links
