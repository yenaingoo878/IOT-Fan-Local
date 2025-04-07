#include "ButtonInput.h"

ButtonInput::ButtonInput(uint8_t pin, unsigned long debounceDelay)
  : _pin(pin), _debounceDelay(debounceDelay) {}

void ButtonInput::begin() {
  pinMode(_pin, INPUT_PULLUP);
  _lastStableState = digitalRead(_pin);
  _lastReadTime = millis();
}

bool ButtonInput::isPressed() {
  bool currentState = digitalRead(_pin);

  if (currentState != _lastStableState) {
    _lastReadTime = millis();
  }

  if ((millis() - _lastReadTime) > _debounceDelay) {
    if (currentState != _buttonState) {
      _buttonState = currentState;
      if (_buttonState == LOW) {
        _lastStableState = currentState;
        return true;
      }
    }
  }

  _lastStableState = currentState;
  return false;
}
