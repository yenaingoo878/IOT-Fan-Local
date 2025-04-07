#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <Arduino.h>

class ButtonInput {
  public:
    ButtonInput(uint8_t pin, unsigned long debounceDelay = 50);
    void begin();
    bool isPressed();

  private:
    uint8_t _pin;
    unsigned long _debounceDelay;
    bool _buttonState = HIGH;
    bool _lastStableState = HIGH;
    unsigned long _lastReadTime = 0;
};

#endif
