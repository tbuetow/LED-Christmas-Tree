#include "hardware.h"
#include "config.h"
#include "input.h"
#include <avr/io.h>
#include <avr/interrupt.h>

volatile unsigned long tick_10ms = 0;

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

// Match Arduino core's TCA prescaler selection so TCB1 can use CLKTCA as its clock.
// TCB1_COMPARE needs to be less than 16535.
constexpr uint32_t TCA_PRESCALER_DIVISOR = 16UL; // 20MHz / 16 = 1.25MHz
constexpr uint16_t TCB1_COMPARE = static_cast<uint16_t>((F_CPU / TCA_PRESCALER_DIVISOR / 100UL) - 1UL); // 20,000,000 / 16 / 100 = 12,500. -1 because 0 indexed

void init_timer() {
  cli();
  TCB1.CTRLA = 0x00;
  TCB1.CTRLB = TCB_CNTMODE_INT_gc; // Periodic interrupt mode
  TCB1.CCMP = TCB1_COMPARE;
  TCB1.INTFLAGS = TCB_CAPT_bm;
  TCB1.INTCTRL = TCB_CAPT_bm;
  TCB1.CTRLA = TCB_CLKSEL_CLKTCA_gc | TCB_ENABLE_bm; // Clocked from TCA (prescaled) to keep count in range
  sei();
}

ISR(TCB1_INT_vect) {
  //This is a ~10ms timer
  tick_10ms++;
  //Button debouncing updates
  service_buttons();
}
