/**
 * @file button_manager.h
 * @brief Button Handling with Debounce & Long Press
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "config.h"
#include <functional>

namespace buttons {

using ButtonCallback = std::function<void()>;
using ButtonLongCallback = std::function<void()>;

class ButtonManager {
public:
    ButtonManager();

    void begin();
    void update();  // Call frequently

    // Callbacks
    void onAlignPress(ButtonCallback cb) { alignPressCb = cb; }
    void onAlignLongPress(ButtonLongCallback cb) { alignLongCb = cb; }
    void onModePress(ButtonCallback cb) { modePressCb = cb; }
    void onModeLongPress(ButtonLongCallback cb) { modeLongCb = cb; }
    void onEmergency(ButtonCallback cb) { emergencyCb = cb; }

    bool isAlignPressed() const { return alignState == PRESSED; }
    bool isModePressed() const { return modeState == PRESSED; }

private:
    enum ButtonState { IDLE, DEBOUNCE, PRESSED, LONG_PRESS };

    struct Button {
        gpio_num_t pin;
        ButtonState state;
        uint32_t pressTime;
        uint32_t lastChange;
        bool lastReading;
    };

    Button alignBtn;
    Button modeBtn;

    ButtonCallback alignPressCb;
    ButtonLongCallback alignLongCb;
    ButtonCallback modePressCb;
    ButtonLongCallback modeLongCb;
    ButtonCallback emergencyCb;

    void updateButton(Button& btn);
    bool readButton(gpio_num_t pin);
};

} // namespace buttons

#endif // BUTTON_MANAGER_H
