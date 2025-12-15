#include "input.h"
#include "state.h"
#include "config.h"
#include "button.h"
#include <avr/io.h>

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