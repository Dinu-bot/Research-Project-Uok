/**
 * @file fsm_manager.h
 * @brief Finite State Machine for Tri-Mode Link Switching
 * @description Threshold-based FSM with hysteresis, EMA filtering, and
 *              Make-Before-Break warning state.
 */

#ifndef FSM_MANAGER_H
#define FSM_MANAGER_H

#include "config.h"
#include <cstdint>

namespace fsm {

// ============================================================================
// FSM STATES
// ============================================================================
enum State : uint8_t {
    STATE_IDLE = 0,         // Boot / no link active
    STATE_WIFI_ACTIVE,      // Wi-Fi is primary link
    STATE_FSO_ACTIVE,       // FSO is primary link
    STATE_LORA_EMERGENCY,   // LoRa only (both Wi-Fi and FSO failed)
    STATE_WARNING,          // Make-Before-Break: duplicating on both links
    STATE_COUNT
};

enum LinkType : uint8_t {
    LINK_NONE = 0,
    LINK_WIFI = 1,
    LINK_FSO  = 2,
    LINK_LORA = 3
};

// ============================================================================
// SIGNAL METRICS (filtered)
// ============================================================================
struct SignalMetrics {
    float fsoVoltage;       // ADC reading (0-3.3V, biased at 1.65V)
    float fsoVoltageEMA;    // Exponentially moving average
    int8_t wifiRSSI;        // dBm
    float wifiRSSIEMA;      // EMA of RSSI
    int8_t loraRSSI;        // dBm
    float loraRSSIEMA;
    uint8_t crcErrorRate;   // Percentage (0-100)
    uint8_t ackTimeoutCount;// Consecutive ACK timeouts
    uint32_t lastUpdate;    // millis()

    SignalMetrics() : fsoVoltage(0), fsoVoltageEMA(1.65f), wifiRSSI(-100),
                      wifiRSSIEMA(-100), loraRSSI(-120), loraRSSIEMA(-120),
                      crcErrorRate(0), ackTimeoutCount(0), lastUpdate(0) {}
};

// ============================================================================
// FSM MANAGER
// ============================================================================
class FSMManager {
public:
    FSMManager();

    // Initialize with thresholds from NVS (or defaults)
    void begin();

    // Update signal metrics (called from data task)
    void updateMetrics(float fsoV, int8_t wifiRssi, int8_t loraRssi);
    void onCRCError();
    void onACKTimeout();
    void onACKSuccess();

    // Main FSM evaluation (call every tick)
    void evaluate();

    // State queries
    State getState() const { return currentState; }
    LinkType getActiveLink() const;
    LinkType getBackupLink() const;
    bool isWarning() const { return currentState == STATE_WARNING; }
    bool shouldDuplicate() const { return currentState == STATE_WARNING; }

    // Manual override (from laptop CMD packet)
    void forceLink(LinkType link);
    void releaseOverride();
    bool isOverridden() const { return manualOverride; }

    // Hysteresis status
    bool isStable() const { return stabilityTimer == 0; }
    uint32_t getStabilityRemaining() const { return stabilityTimer; }

    // Metrics access
    const SignalMetrics& getMetrics() const { return metrics; }

    // String helpers
    static const char* stateToString(State s);
    static const char* linkToString(LinkType l);

private:
    State currentState;
    State previousState;
    SignalMetrics metrics;

    // Hysteresis timers
    uint32_t stabilityTimer;    // ms remaining before switch allowed
    uint32_t warningTimer;      // ms remaining in warning state
    uint32_t lastEvalTime;

    // CRC error tracking
    uint8_t crcWindow[CRC_ERROR_WINDOW];
    uint8_t crcWindowIndex;
    uint8_t crcErrorCount;

    // Manual override
    bool manualOverride;
    LinkType forcedLink;

    // EMA alpha coefficient (0.0-1.0), higher = more responsive
    static constexpr float EMA_ALPHA = 0.3f;

    // Thresholds (loaded from NVS or defaults)
    float threshFsoDegrade;
    float threshFsoRecover;
    int8_t threshWifiDegrade;
    int8_t threshWifiRecover;
    uint8_t threshCRCDegrade;
    uint8_t threshCRCRecover;
    uint32_t threshStableMs;
    uint32_t threshWarningMs;

    void updateEMA();
    void updateCRCWindow(bool error);
    uint8_t calculateCRCRate();
    void transitionTo(State newState);
    bool canSwitchTo(State target);
    void enterWarning(State from, State to);
    State evaluateTransitions();
};

} // namespace fsm

#endif // FSM_MANAGER_H
