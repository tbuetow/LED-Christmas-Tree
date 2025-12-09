#ifndef BUTTON_H
#define BUTTON_H

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

class Button {
public:
  Button(volatile uint8_t* pinReg, uint8_t pin);

  void update();  // call this every 10ms from a timer ISR
  bool wasPressed();   // returns true once when pressed
  bool wasReleased();  // returns true once when released

private:
  volatile uint8_t* _pinReg;
  uint8_t _pinMask;
  uint8_t _history;
  bool _debouncedState;
  bool _pressFlag;
  bool _releaseFlag;
};

#endif