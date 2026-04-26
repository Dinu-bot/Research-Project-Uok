/**
 * @file button_manager.cpp
 */

#include "button_manager.h"
#include <Arduino.h>

namespace buttons {

ButtonManager::ButtonManager() 
    : alignPressCb(nullptr), alignLongCb(nullptr),
      modePressCb(nullptr), modeLongCb(nullptr), emergencyCb(nullptr) {
    alignBtn = {PIN_FSO_ALIGN_BTN, IDLE, 0, 0, true};
    modeBtn = {PIN_MODE_BTN, IDLE, 0, 0, true};
}

void ButtonManager::begin() {
    pinMode(PIN_FSO_ALIGN_BTN, INPUT_PULLUP);
    pinMode(PIN_MODE_BTN, INPUT_PULLUP);
    Serial.println("[Buttons] Initialized");
}

void ButtonManager::update() {
    updateButton(alignBtn);
    updateButton(modeBtn);
}

bool ButtonManager::readButton(gpio_num_t pin) {
    return digitalRead((uint8_t)pin) == LOW;  // Active-low with pull-up
}

void ButtonManager::updateButton(Button& btn) {
    bool reading = readButton(btn.pin);
    uint32_t now = millis();

    switch (btn.state) {
        case IDLE:
            if (reading && !btn.lastReading) {
                btn.state = DEBOUNCE;
                btn.lastChange = now;
            }
            break;

        case DEBOUNCE:
            if (now - btn.lastChange >= BUTTON_DEBOUNCE_MS) {
                if (reading) {
                    btn.state = PRESSED;
                    btn.pressTime = now;

                    // Trigger press callback
                    if (btn.pin == PIN_FSO_ALIGN_BTN && alignPressCb) alignPressCb();
                    else if (btn.pin == PIN_MODE_BTN && modePressCb) modePressCb();
                } else {
                    btn.state = IDLE;
                }
            }
            break;

        case PRESSED:
            if (!reading) {
                btn.state = IDLE;
            } else if (now - btn.pressTime >= BUTTON_LONG_PRESS_MS) {
                btn.state = LONG_PRESS;

                // Trigger long press
                if (btn.pin == PIN_FSO_ALIGN_BTN && alignLongCb) alignLongCb();
                else if (btn.pin == PIN_MODE_BTN) {
                    if (modeLongCb) modeLongCb();
                    if (emergencyCb) emergencyCb();
                }
            }
            break;

        case LONG_PRESS:
            if (!reading) {
                btn.state = IDLE;
            }
            break;
    }

    btn.lastReading = reading;
}

} // namespace buttons
