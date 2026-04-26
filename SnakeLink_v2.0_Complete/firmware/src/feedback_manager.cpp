/**
 * @file feedback_manager.cpp
 * @brief LED & Buzzer Implementation
 */

#include "feedback_manager.h"
#include <Arduino.h>

namespace feedback {

// Tone patterns (frequency Hz, duration ms)
static const uint16_t TONE_BOOT[] = {523, 659, 784};
static const uint16_t DUR_BOOT[] = {150, 150, 300};

static const uint16_t TONE_SWITCH[] = {600, 500, 400};
static const uint16_t DUR_SWITCH[] = {100, 100, 200};

static const uint16_t TONE_EMERGENCY[] = {1047, 1047};
static const uint16_t DUR_EMERGENCY[] = {100, 100};

static const uint16_t TONE_BATTERY[] = {200, 200};
static const uint16_t DUR_BATTERY[] = {200, 400};

FeedbackManager::FeedbackManager()
    : currentEvent(EVT_NONE), eventStartTime(0), lastUpdate(0),
      greenPattern(LED_OFF), bluePattern(LED_OFF), redPattern(LED_OFF),
      greenBrightness(0), blueBrightness(0), redBrightness(0),
      buzzerFreq(0), buzzerDuration(0), buzzerStart(0), buzzerActive(false),
      greenChannel(0), blueChannel(1), redChannel(2), buzzerChannel(3) {}

bool FeedbackManager::begin() {
    // Configure LED PWM channels
    ledcSetup(greenChannel, LED_PWM_FREQ, LED_PWM_RES);
    ledcAttachPin(PIN_LED_GREEN, greenChannel);

    ledcSetup(blueChannel, LED_PWM_FREQ, LED_PWM_RES);
    ledcAttachPin(PIN_LED_BLUE, blueChannel);

    ledcSetup(redChannel, LED_PWM_FREQ, LED_PWM_RES);
    ledcAttachPin(PIN_LED_RED, redChannel);

    // Configure buzzer PWM
    ledcSetup(buzzerChannel, BUZZER_PWM_FREQ, BUZZER_PWM_RES);
    ledcAttachPin(PIN_BUZZER, buzzerChannel);

    // Initial state: all off
    ledcWrite(greenChannel, 0);
    ledcWrite(blueChannel, 0);
    ledcWrite(redChannel, 0);
    ledcWrite(buzzerChannel, 0);

    Serial.println("[Feedback] LEDs & buzzer initialized");
    return true;
}

void FeedbackManager::update() {
    uint32_t now = millis();
    if (now - lastUpdate < 20) return;  // 50Hz update
    lastUpdate = now;

    updateLEDs();
    updateBuzzer();
}

void FeedbackManager::trigger(FeedbackEvent evt) {
    currentEvent = evt;
    eventStartTime = millis();

    switch (evt) {
        case EVT_BOOT:
            greenPattern = LED_BREATHE;
            bluePattern = LED_OFF;
            redPattern = LED_OFF;
            playPattern(TONE_BOOT, DUR_BOOT, 3);
            break;

        case EVT_READY:
            greenPattern = LED_BREATHE;
            bluePattern = LED_BLINK_DOUBLE;
            redPattern = LED_OFF;
            break;

        case EVT_TRANSMIT:
            greenPattern = LED_BREATHE;
            bluePattern = LED_BLINK_FAST;
            redPattern = LED_OFF;
            break;

        case EVT_RECEIVE:
            greenPattern = LED_BREATHE;
            bluePattern = LED_BLINK_FAST;
            redPattern = LED_OFF;
            break;

        case EVT_LINK_SWITCH:
            greenPattern = LED_BREATHE;
            bluePattern = LED_OFF;
            redPattern = LED_BLINK_FAST;
            playPattern(TONE_SWITCH, DUR_SWITCH, 3);
            break;

        case EVT_LORA_EMERGENCY:
            greenPattern = LED_BREATHE;
            bluePattern = LED_BLINK_SPARSE;
            redPattern = LED_OFF;
            break;

        case EVT_FSO_ALIGN:
            greenPattern = LED_BREATHE;
            bluePattern = LED_BLINK_SLOW;
            redPattern = LED_OFF;
            // Continuous tone handled by setAlignmentTone
            break;

        case EVT_EMERGENCY_RX:
            greenPattern = LED_BREATHE;
            bluePattern = LED_BLINK_FAST;
            redPattern = LED_BLINK_FAST;
            playPattern(TONE_EMERGENCY, DUR_EMERGENCY, 2);
            break;

        case EVT_BATTERY_LOW:
            greenPattern = LED_BREATHE;
            bluePattern = LED_OFF;
            redPattern = LED_BLINK_SLOW;
            playPattern(TONE_BATTERY, DUR_BATTERY, 2);
            break;

        case EVT_BATTERY_CRIT:
            greenPattern = LED_BLINK_FAST;
            bluePattern = LED_OFF;
            redPattern = LED_BLINK_FAST;
            playPattern(TONE_BATTERY, DUR_BATTERY, 2);
            break;

        case EVT_CRC_ERROR:
            greenPattern = LED_BREATHE;
            bluePattern = LED_OFF;
            redPattern = LED_STUTTER;
            break;

        default:
            break;
    }
}

void FeedbackManager::setLinkPattern(uint8_t link) {
    switch (link) {
        case 1: bluePattern = LED_BLINK_DOUBLE; break;   // Wi-Fi
        case 2: bluePattern = LED_BLINK_SLOW; break;     // FSO
        case 3: bluePattern = LED_BLINK_SPARSE; break;   // LoRa
        default: bluePattern = LED_OFF; break;
    }
}

void FeedbackManager::setEmergency(bool active) {
    if (active) {
        redPattern = LED_BLINK_FAST;
        playPattern(TONE_EMERGENCY, DUR_EMERGENCY, 2);
    } else {
        redPattern = LED_OFF;
        buzzerActive = false;
        ledcWrite(buzzerChannel, 0);
    }
}

void FeedbackManager::setAlignmentTone(uint16_t freq) {
    if (freq > 0) {
        playTone(freq, 50);  // 50ms updates
    } else {
        buzzerActive = false;
        ledcWrite(buzzerChannel, 0);
    }
}

void FeedbackManager::silence() {
    buzzerActive = false;
    ledcWrite(buzzerChannel, 0);
}

void FeedbackManager::updateLEDs() {
    uint32_t now = millis();
    setLED(greenChannel, greenPattern, now);
    setLED(blueChannel, bluePattern, now);
    setLED(redChannel, redPattern, now);
}

void FeedbackManager::setLED(int channel, LEDPattern pattern, uint32_t now) {
    uint8_t val = 0;
    switch (pattern) {
        case LED_OFF: val = 0; break;
        case LED_ON: val = 255; break;
        case LED_BREATHE: val = breatheValue(now); break;
        case LED_BLINK_FAST: val = ((now / 100) % 2) ? 255 : 0; break;
        case LED_BLINK_SLOW: val = ((now / 500) % 2) ? 255 : 0; break;
        case LED_BLINK_DOUBLE: {
            uint8_t phase = (now / 100) % 4;
            val = (phase == 0 || phase == 2) ? 255 : 0;
            break;
        }
        case LED_BLINK_SPARSE: val = ((now / 2000) % 2) ? 255 : 0; break;
        case LED_STUTTER: {
            uint8_t phase = (now / 50) % 6;
            val = (phase < 3) ? 255 : 0;
            break;
        }
    }
    ledcWrite(channel, val);
}

uint8_t FeedbackManager::breatheValue(uint32_t t) {
    // Sine wave breathing: period ~2 seconds
    float phase = (t % 2000) / 2000.0f * 2.0f * PI;
    return (uint8_t)((sin(phase) + 1.0f) * 127.5f);
}

void FeedbackManager::updateBuzzer() {
    if (!buzzerActive) return;

    uint32_t now = millis();
    if (now - buzzerStart >= buzzerDuration) {
        buzzerActive = false;
        ledcWrite(buzzerChannel, 0);
    }
}

void FeedbackManager::playTone(uint16_t freq, uint32_t duration) {
    buzzerFreq = freq;
    buzzerDuration = duration;
    buzzerStart = millis();
    buzzerActive = true;

    ledcWriteTone(buzzerChannel, freq);
    ledcWrite(buzzerChannel, 128);  // 50% duty cycle
}

void FeedbackManager::playPattern(const uint16_t* freqs, const uint16_t* durs, uint8_t count) {
    // Simple sequential pattern (blocking for simplicity, in production use async)
    for (uint8_t i = 0; i < count; i++) {
        playTone(freqs[i], durs[i]);
        delay(durs[i] + 50);
    }
}

} // namespace feedback
