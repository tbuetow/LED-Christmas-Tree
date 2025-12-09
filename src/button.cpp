#include <button.h>

Button::Button(volatile uint8_t* pinReg, uint8_t pin) : 
    _pinReg(pinReg),
    _pinMask((1 << pin)),
    _history(0xFF),
    _debouncedState(true),
    _pressFlag(false),
    _releaseFlag(false)
{}

void Button::update() {
  bool sample = (*_pinReg & _pinMask) != 0;  // active-low
  _history = (_history << 1) | sample;

  if (_history == 0x00 && _debouncedState) {
    _debouncedState = false;
    _pressFlag = true;
  }
  if (_history == 0xFF && !_debouncedState) {
    _debouncedState = true;
    _releaseFlag = true;
  }
}

bool Button::wasPressed() {
  bool ret = _pressFlag;
  _pressFlag = false;
  return ret;
}

bool Button::wasReleased() {
  bool ret = _releaseFlag;
  _releaseFlag = false;
  return ret;
}