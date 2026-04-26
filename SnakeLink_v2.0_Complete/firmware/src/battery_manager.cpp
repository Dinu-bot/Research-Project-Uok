/**
 * @file battery_manager.cpp
 */

#include "battery_manager.h"
#include <Arduino.h>

namespace battery {

BatteryManager::BatteryManager() 
    : voltage(4.2f), percent(100), level(BAT_FULL),
      calibrationOffset(0.0f), lastRead(0) {}

void BatteryManager::begin() {
    analogSetPinAttenuation(PIN_BATTERY_ADC, BATTERY_ADC_ATTEN);
    analogReadResolution(BATTERY_ADC_WIDTH);
    readVoltage();
    Serial.printf("[Battery] Initial: %.2fV (%u%%)\n", voltage, percent);
}

void BatteryManager::update() {
    uint32_t now = millis();
    if (now - lastRead < READ_INTERVAL_MS) return;
    lastRead = now;
    readVoltage();
}

void BatteryManager::readVoltage() {
    // Average 10 samples for noise reduction
    uint32_t sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(PIN_BATTERY_ADC);
        delayMicroseconds(100);
    }
    float raw = sum / 10.0f;

    // Convert to voltage: 12-bit ADC, 3.3V reference, 11dB attenuation
    // With 11dB attenuation, full scale is ~2.6V effective at pin
    // But ESP32 ADC is non-linear; use calibrated formula
    float pinVoltage = (raw / 4095.0f) * 3.3f;

    // Apply divider ratio and calibration
    voltage = (pinVoltage * BATTERY_DIVIDER_RATIO) + calibrationOffset;

    // Constrain to valid range
    if (voltage > BATTERY_VOLTAGE_MAX) voltage = BATTERY_VOLTAGE_MAX;
    if (voltage < BATTERY_VOLTAGE_MIN) voltage = BATTERY_VOLTAGE_MIN;

    percent = voltageToPercent(voltage);
    level = voltageToLevel(voltage);
}

BatteryLevel BatteryManager::voltageToLevel(float v) {
    if (v >= 4.20f) return BAT_FULL;
    if (v >= 3.90f) return BAT_HEALTHY;
    if (v >= 3.70f) return BAT_MODERATE;
    if (v >= 3.50f) return BAT_LOW;
    if (v >= 3.30f) return BAT_CRITICAL;
    return BAT_SHUTDOWN;
}

void BatteryManager::setCalibration(float actualVoltage) {
    calibrationOffset = actualVoltage - voltage;
    readVoltage();  // Re-read with new calibration
}

} // namespace battery
