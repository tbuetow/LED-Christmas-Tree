#include "hardware.h"
#include "config.h"
#include "state.h"
#include "button.h"
#include <avr/io.h>

void init_CPU() {
  // Ensure the main clock runs without a prescaler for consistent timing.
  _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00);
}

void init_gpio() {
  // Button pins as inputs with pull-ups
  VPORTA.DIR &= ~(1 << Config::MODE_BUTTON_BIT);
  VPORTB.DIR &= ~(1 << Config::BRIGHTNESS_BUTTON_BIT);
  PORTA.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;
  PORTB.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;

  // Outputs: tick and APA102 signals
  VPORTB.DIR |= PIN4_bm | PIN5_bm;
  VPORTC.DIR |= (1 << Config::TICK_OUT_BIT);
  VPORTC.OUT &= ~(1 << Config::TICK_OUT_BIT);
}


// BUTTONS
Button btn_brightness(&VPORTB.IN, Config::BRIGHTNESS_BUTTON_BIT);
Button btn_mode(&VPORTA.IN, Config::MODE_BUTTON_BIT);

void service_buttons() {
  btn_brightness.update();
  btn_mode.update();

  if (btn_brightness.wasPressed())
    next_brightness();

  if (btn_mode.wasPressed())
    next_mode();
}