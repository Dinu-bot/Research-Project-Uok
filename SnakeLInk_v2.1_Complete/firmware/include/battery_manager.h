/**
 * @file battery_manager.h
 * @brief Battery Monitoring & Voltage Reading
 */

#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include "config.h"

namespace battery {

enum BatteryLevel {
    BAT_FULL = 0,
    BAT_HEALTHY,
    BAT_MODERATE,
    BAT_LOW,
    BAT_CRITICAL,
    BAT_SHUTDOWN
};

class BatteryManager {
public:
    BatteryManager();

    void begin();
    void update();  // Call periodically

    float getVoltage() const { return voltage; }
    uint8_t getPercent() const { return percent; }
    BatteryLevel getLevel() const { return level; }
    bool isLow() const { return level >= BAT_LOW; }
    bool isCritical() const { return level >= BAT_CRITICAL; }

    // Calibration
    void setCalibration(float actualVoltage);

private:
    float voltage;
    uint8_t percent;
    BatteryLevel level;
    float calibrationOffset;
    uint32_t lastRead;

    static constexpr uint32_t READ_INTERVAL_MS = 5000;

    void readVoltage();
    BatteryLevel voltageToLevel(float v);
};

} // namespace battery

#endif // BATTERY_MANAGER_H
