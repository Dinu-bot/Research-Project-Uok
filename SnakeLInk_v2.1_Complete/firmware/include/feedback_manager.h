/**
 * @file feedback_manager.h
 * @brief LED & Buzzer Feedback System
 * @description Coordinated multi-channel feedback for field operations.
 */

#ifndef FEEDBACK_MANAGER_H
#define FEEDBACK_MANAGER_H

#include "config.h"

namespace feedback {

enum FeedbackEvent {
    EVT_NONE = 0,
    EVT_BOOT,
    EVT_READY,
    EVT_TRANSMIT,
    EVT_RECEIVE,
    EVT_LINK_SWITCH,
    EVT_LORA_EMERGENCY,
    EVT_FSO_ALIGN,
    EVT_EMERGENCY_RX,
    EVT_BATTERY_LOW,
    EVT_BATTERY_CRIT,
    EVT_CRC_ERROR,
    EVT_BUTTON_PRESS
};

enum LEDPattern {
    LED_OFF = 0,
    LED_ON,
    LED_BREATHE,
    LED_BLINK_FAST,
    LED_BLINK_SLOW,
    LED_BLINK_DOUBLE,
    LED_BLINK_SPARSE,
    LED_STUTTER
};

class FeedbackManager {
public:
    FeedbackManager();

    bool begin();
    void update();  // Call from background task

    // Trigger events
    void trigger(FeedbackEvent evt);
    void setLinkPattern(uint8_t link);  // 1=WiFi, 2=FSO, 3=LoRa
    void setEmergency(bool active);
    void setAlignmentTone(uint16_t freq);  // 300-1200Hz proportional to signal
    void silence();

private:
    FeedbackEvent currentEvent;
    uint32_t eventStartTime;
    uint32_t lastUpdate;

    // LED state
    LEDPattern greenPattern;
    LEDPattern bluePattern;
    LEDPattern redPattern;
    uint8_t greenBrightness;
    uint8_t blueBrightness;
    uint8_t redBrightness;

    // Buzzer
    uint16_t buzzerFreq;
    uint32_t buzzerDuration;
    uint32_t buzzerStart;
    bool buzzerActive;

    // PWM channels
    int greenChannel;
    int blueChannel;
    int redChannel;
    int buzzerChannel;

    void updateLEDs();
    void updateBuzzer();
    void setLED(int channel, LEDPattern pattern, uint32_t now);
    uint8_t breatheValue(uint32_t t);
    void playTone(uint16_t freq, uint32_t duration);
    void playPattern(const uint16_t* freqs, const uint16_t* durations, uint8_t count);
};

} // namespace feedback

#endif // FEEDBACK_MANAGER_H
