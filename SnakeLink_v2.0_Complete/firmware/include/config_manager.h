/**
 * @file config_manager.h
 * @brief Persistent Configuration via NVS
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config.h"
#include <Preferences.h>

namespace config {

struct DeviceConfig {
    char deviceName[MAX_DEVICE_NAME_LEN];
    uint8_t deviceID[DEVICE_ID_LEN];
    uint8_t encryptionKey[DEVICE_KEY_LEN];
    float wifiDegradeThreshold;
    float wifiRecoverThreshold;
    float fsoDegradeThreshold;
    float fsoRecoverThreshold;
    uint32_t stabilityWindowMs;
    uint32_t warningWindowMs;
    bool highSecurityMode;
};

class ConfigManager {
public:
    ConfigManager();

    bool begin();
    void loadDefaults();
    bool save();
    bool load();
    bool reset();

    DeviceConfig& get() { return cfg; }
    const DeviceConfig& getConst() const { return cfg; }

    // Key fingerprint for visual verification
    void getKeyFingerprint(char* out, size_t len);

private:
    Preferences prefs;
    DeviceConfig cfg;
    bool initialized;
    static constexpr const char* NAMESPACE = "snakelink";
};

} // namespace config

#endif // CONFIG_MANAGER_H
