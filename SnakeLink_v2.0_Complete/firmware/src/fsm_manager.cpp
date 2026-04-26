/**
 * @file fsm_manager.cpp
 * @brief FSM Implementation with Hysteresis and Make-Before-Break
 */

#include "fsm_manager.h"
#include <Arduino.h>

namespace fsm {

// ============================================================================
// FSM MANAGER
// ============================================================================
FSMManager::FSMManager()
    : currentState(STATE_IDLE), previousState(STATE_IDLE),
      stabilityTimer(0), warningTimer(0), lastEvalTime(0),
      crcWindowIndex(0), crcErrorCount(0),
      manualOverride(false), forcedLink(LINK_NONE),
      threshFsoDegrade(FSO_VOLTAGE_DEGRADE),
      threshFsoRecover(FSO_VOLTAGE_RECOVER),
      threshWifiDegrade(WIFI_RSSI_DEGRADE),
      threshWifiRecover(WIFI_RSSI_RECOVER),
      threshCRCDegrade(CRC_ERROR_DEGRADE),
      threshCRCRecover(CRC_ERROR_RECOVER),
      threshStableMs(WIFI_RSSI_STABLE_MS),
      threshWarningMs(MBB_WARNING_DURATION_MS) {
    memset(crcWindow, 0, sizeof(crcWindow));
}

void FSMManager::begin() {
    // TODO: Load thresholds from NVS if available
    currentState = STATE_WIFI_ACTIVE;  // Default to Wi-Fi if available
    lastEvalTime = millis();
    Serial.println("[FSM] Initialized");
}

void FSMManager::updateMetrics(float fsoV, int8_t wifiRssi, int8_t loraRssi) {
    metrics.fsoVoltage = fsoV;
    metrics.wifiRSSI = wifiRssi;
    metrics.loraRSSI = loraRssi;
    metrics.lastUpdate = millis();
    updateEMA();
}

void FSMManager::updateEMA() {
    // FSO voltage EMA (note: idle is ~1.65V, signal rides on top)
    metrics.fsoVoltageEMA = (EMA_ALPHA * metrics.fsoVoltage) + 
                            ((1.0f - EMA_ALPHA) * metrics.fsoVoltageEMA);

    // Wi-Fi RSSI EMA
    metrics.wifiRSSIEMA = (EMA_ALPHA * static_cast<float>(metrics.wifiRSSI)) +
                          ((1.0f - EMA_ALPHA) * metrics.wifiRSSIEMA);

    // LoRa RSSI EMA
    metrics.loraRSSIEMA = (EMA_ALPHA * static_cast<float>(metrics.loraRSSI)) +
                          ((1.0f - EMA_ALPHA) * metrics.loraRSSIEMA);
}

void FSMManager::onCRCError() {
    updateCRCWindow(true);
}

void FSMManager::onACKTimeout() {
    if (metrics.ackTimeoutCount < 255) metrics.ackTimeoutCount++;
}

void FSMManager::onACKSuccess() {
    metrics.ackTimeoutCount = 0;
    updateCRCWindow(false);
}

void FSMManager::updateCRCWindow(bool error) {
    if (crcWindow[crcWindowIndex] && !error) crcErrorCount--;
    if (!crcWindow[crcWindowIndex] && error) crcErrorCount++;
    crcWindow[crcWindowIndex] = error ? 1 : 0;
    crcWindowIndex = (crcWindowIndex + 1) % CRC_ERROR_WINDOW;
    metrics.crcErrorRate = calculateCRCRate();
}

uint8_t FSMManager::calculateCRCRate() {
    return (crcErrorCount * 100) / CRC_ERROR_WINDOW;
}

void FSMManager::evaluate() {
    uint32_t now = millis();
    uint32_t dt = now - lastEvalTime;
    lastEvalTime = now;

    if (manualOverride) {
        State target = STATE_IDLE;
        switch (forcedLink) {
            case LINK_WIFI: target = STATE_WIFI_ACTIVE; break;
            case LINK_FSO:  target = STATE_FSO_ACTIVE; break;
            case LINK_LORA: target = STATE_LORA_EMERGENCY; break;
            default: break;
        }
        if (target != currentState) transitionTo(target);
        return;
    }

    // Update stability timer
    if (stabilityTimer > 0) {
        stabilityTimer = (stabilityTimer > dt) ? (stabilityTimer - dt) : 0;
    }

    // Update warning timer
    if (warningTimer > 0) {
        warningTimer = (warningTimer > dt) ? (warningTimer - dt) : 0;
        if (warningTimer == 0 && currentState == STATE_WARNING) {
            // Warning expired, complete transition to backup
            State backup = evaluateTransitions();
            if (backup != currentState) transitionTo(backup);
        }
    }

    State newState = evaluateTransitions();
    if (newState != currentState) {
        if (currentState == STATE_WARNING && newState != STATE_WARNING) {
            // Transition out of warning completes
            transitionTo(newState);
        } else if (newState == STATE_WARNING) {
            // Enter warning state
            enterWarning(currentState, 
                        (currentState == STATE_WIFI_ACTIVE) ? STATE_FSO_ACTIVE : STATE_WIFI_ACTIVE);
        } else {
            transitionTo(newState);
        }
    }
}

State FSMManager::evaluateTransitions() {
    float fsoEMA = metrics.fsoVoltageEMA;
    float wifiEMA = metrics.wifiRSSIEMA;
    uint8_t crcRate = metrics.crcErrorRate;
    uint8_t ackTimeouts = metrics.ackTimeoutCount;

    // Determine link health
    bool fsoHealthy = (fsoEMA >= threshFsoRecover) && (crcRate < threshCRCRecover);
    bool fsoDegraded = (fsoEMA < threshFsoDegrade) || (crcRate >= threshCRCDegrade);
    bool wifiHealthy = (wifiEMA >= threshWifiRecover) && (crcRate < threshCRCRecover);
    bool wifiDegraded = (wifiEMA < threshWifiDegrade) || (crcRate >= threshCRCDegrade) || 
                        (ackTimeouts >= ACK_TIMEOUT_CONSECUTIVE);

    // State machine logic
    switch (currentState) {
        case STATE_IDLE:
            if (wifiHealthy) return STATE_WIFI_ACTIVE;
            if (fsoHealthy) return STATE_FSO_ACTIVE;
            return STATE_LORA_EMERGENCY;

        case STATE_WIFI_ACTIVE:
            if (wifiDegraded && fsoHealthy) {
                // Wi-Fi degrading, FSO available → warning then switch
                return STATE_WARNING;
            }
            if (wifiDegraded && !fsoHealthy) {
                // Wi-Fi degrading, FSO also bad → LoRa
                return STATE_LORA_EMERGENCY;
            }
            return STATE_WIFI_ACTIVE;

        case STATE_FSO_ACTIVE:
            if (fsoDegraded && wifiHealthy) {
                return STATE_WARNING;
            }
            if (fsoDegraded && !wifiHealthy) {
                return STATE_LORA_EMERGENCY;
            }
            return STATE_FSO_ACTIVE;

        case STATE_LORA_EMERGENCY:
            if (wifiHealthy) return STATE_WIFI_ACTIVE;
            if (fsoHealthy) return STATE_FSO_ACTIVE;
            return STATE_LORA_EMERGENCY;

        case STATE_WARNING:
            // In warning, evaluate which link to land on
            if (previousState == STATE_WIFI_ACTIVE) {
                // Was on Wi-Fi, switching to FSO
                if (fsoDegraded && !wifiHealthy) return STATE_LORA_EMERGENCY;
                if (!fsoDegraded) return STATE_FSO_ACTIVE;
                if (!wifiDegraded) return STATE_WIFI_ACTIVE;
            } else {
                // Was on FSO, switching to Wi-Fi
                if (wifiDegraded && !fsoHealthy) return STATE_LORA_EMERGENCY;
                if (!wifiDegraded) return STATE_WIFI_ACTIVE;
                if (!fsoDegraded) return STATE_FSO_ACTIVE;
            }
            return STATE_WARNING;

        default:
            return STATE_IDLE;
    }
}

void FSMManager::transitionTo(State newState) {
    if (newState == currentState) return;
    if (stabilityTimer > 0 && newState != STATE_WARNING) {
        // Hysteresis active, block transition unless going to warning
        return;
    }

    previousState = currentState;
    currentState = newState;
    stabilityTimer = threshStableMs;  // Start stability window

    Serial.printf("[FSM] %s → %s (FSO:%.2fV WiFi:%.0fdBm CRC:%u%% ACK:%u)\n",
                  stateToString(previousState), stateToString(currentState),
                  metrics.fsoVoltageEMA, metrics.wifiRSSIEMA, 
                  metrics.crcErrorRate, metrics.ackTimeoutCount);
}

void FSMManager::enterWarning(State from, State to) {
    previousState = from;
    currentState = STATE_WARNING;
    warningTimer = threshWarningMs;
    Serial.printf("[FSM] WARNING STATE: duplicating %s → %s for %ums\n",
                  stateToString(from), stateToString(to), threshWarningMs);
}

LinkType FSMManager::getActiveLink() const {
    switch (currentState) {
        case STATE_WIFI_ACTIVE: return LINK_WIFI;
        case STATE_FSO_ACTIVE:  return LINK_FSO;
        case STATE_LORA_EMERGENCY: return LINK_LORA;
        case STATE_WARNING:
            // During warning, return the primary (degrading) link
            return (previousState == STATE_WIFI_ACTIVE) ? LINK_WIFI : LINK_FSO;
        default: return LINK_NONE;
    }
}

LinkType FSMManager::getBackupLink() const {
    switch (currentState) {
        case STATE_WIFI_ACTIVE: return LINK_FSO;
        case STATE_FSO_ACTIVE:  return LINK_WIFI;
        case STATE_WARNING:
            return (previousState == STATE_WIFI_ACTIVE) ? LINK_FSO : LINK_WIFI;
        case STATE_LORA_EMERGENCY:
            if (metrics.wifiRSSIEMA >= threshWifiRecover) return LINK_WIFI;
            if (metrics.fsoVoltageEMA >= threshFsoRecover) return LINK_FSO;
            return LINK_NONE;
        default: return LINK_NONE;
    }
}

void FSMManager::forceLink(LinkType link) {
    manualOverride = true;
    forcedLink = link;
    Serial.printf("[FSM] MANUAL OVERRIDE → %s\n", linkToString(link));
}

void FSMManager::releaseOverride() {
    manualOverride = false;
    forcedLink = LINK_NONE;
    Serial.println("[FSM] Manual override released");
}

const char* FSMManager::stateToString(State s) {
    switch (s) {
        case STATE_IDLE: return "IDLE";
        case STATE_WIFI_ACTIVE: return "WIFI";
        case STATE_FSO_ACTIVE: return "FSO";
        case STATE_LORA_EMERGENCY: return "LORA";
        case STATE_WARNING: return "WARNING";
        default: return "UNKNOWN";
    }
}

const char* FSMManager::linkToString(LinkType l) {
    switch (l) {
        case LINK_NONE: return "NONE";
        case LINK_WIFI: return "WIFI";
        case LINK_FSO: return "FSO";
        case LINK_LORA: return "LORA";
        default: return "UNKNOWN";
    }
}

} // namespace fsm
