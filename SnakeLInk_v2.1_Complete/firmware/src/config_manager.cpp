/**
 * @file config_manager.cpp
 */

#include "config_manager.h"
#include <Arduino.h>

namespace config {

ConfigManager::ConfigManager() : initialized(false) {
    memset(&cfg, 0, sizeof(cfg));
}

bool ConfigManager::begin() {
    initialized = prefs.begin(NAMESPACE, false);
    if (!initialized) {
        Serial.println("[Config] NVS init failed, using defaults");
        loadDefaults();
        return false;
    }

    if (!load()) {
        Serial.println("[Config] No saved config, loading defaults");
        loadDefaults();
        save();
    }
    return true;
}

void ConfigManager::loadDefaults() {
    strncpy(cfg.deviceName, "SnakeLink", sizeof(cfg.deviceName));

    // Generate random device ID
    esp_fill_random(cfg.deviceID, DEVICE_ID_LEN);

    // Generate random encryption key
    esp_fill_random(cfg.encryptionKey, DEVICE_KEY_LEN);

    cfg.wifiDegradeThreshold = WIFI_RSSI_DEGRADE;
    cfg.wifiRecoverThreshold = WIFI_RSSI_RECOVER;
    cfg.fsoDegradeThreshold = FSO_VOLTAGE_DEGRADE;
    cfg.fsoRecoverThreshold = FSO_VOLTAGE_RECOVER;
    cfg.stabilityWindowMs = WIFI_RSSI_STABLE_MS;
    cfg.warningWindowMs = MBB_WARNING_DURATION_MS;
    cfg.highSecurityMode = false;
}

bool ConfigManager::save() {
    if (!initialized) return false;

    prefs.putString("name", cfg.deviceName);
    prefs.putBytes("devID", cfg.deviceID, DEVICE_ID_LEN);
    prefs.putBytes("key", cfg.encryptionKey, DEVICE_KEY_LEN);
    prefs.putFloat("wifiDeg", cfg.wifiDegradeThreshold);
    prefs.putFloat("wifiRec", cfg.wifiRecoverThreshold);
    prefs.putFloat("fsoDeg", cfg.fsoDegradeThreshold);
    prefs.putFloat("fsoRec", cfg.fsoRecoverThreshold);
    prefs.putULong("stabMs", cfg.stabilityWindowMs);
    prefs.putULong("warnMs", cfg.warningWindowMs);
    prefs.putBool("highSec", cfg.highSecurityMode);

    Serial.println("[Config] Saved to NVS");
    return true;
}

bool ConfigManager::load() {
    if (!initialized) return false;

    if (!prefs.isKey("name")) return false;

    String name = prefs.getString("name", "SnakeLink");
    strncpy(cfg.deviceName, name.c_str(), sizeof(cfg.deviceName));

    size_t len = prefs.getBytes("devID", cfg.deviceID, DEVICE_ID_LEN);
    if (len != DEVICE_ID_LEN) return false;

    len = prefs.getBytes("key", cfg.encryptionKey, DEVICE_KEY_LEN);
    if (len != DEVICE_KEY_LEN) return false;

    cfg.wifiDegradeThreshold = prefs.getFloat("wifiDeg", WIFI_RSSI_DEGRADE);
    cfg.wifiRecoverThreshold = prefs.getFloat("wifiRec", WIFI_RSSI_RECOVER);
    cfg.fsoDegradeThreshold = prefs.getFloat("fsoDeg", FSO_VOLTAGE_DEGRADE);
    cfg.fsoRecoverThreshold = prefs.getFloat("fsoRec", FSO_VOLTAGE_RECOVER);
    cfg.stabilityWindowMs = prefs.getULong("stabMs", WIFI_RSSI_STABLE_MS);
    cfg.warningWindowMs = prefs.getULong("warnMs", MBB_WARNING_DURATION_MS);
    cfg.highSecurityMode = prefs.getBool("highSec", false);

    Serial.println("[Config] Loaded from NVS");
    return true;
}

bool ConfigManager::reset() {
    if (!initialized) return false;
    prefs.clear();
    loadDefaults();
    return save();
}

void ConfigManager::getKeyFingerprint(char* out, size_t len) {
    if (len < 3) return;
    // Use first byte of key as hex fingerprint
    snprintf(out, len, "%02X", cfg.encryptionKey[0]);
}

} // namespace config
