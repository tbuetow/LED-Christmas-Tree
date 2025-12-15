#include "state.h"
#include "config.h"
#include <EEPROM.h>

static Mode mode = Mode::RAINBOW_SOLID_SLOW;
static uint8_t brightness_index = 0;
static bool write_eeprom_flag = false;

void load_state() {
  brightness_index = EEPROM.read(Config::EEPROM_ADDR_BRIGHTNESS);
  if (brightness_index >= Config::BRIGHTNESS_STEPS)
    brightness_index = 0;

  uint8_t m = EEPROM.read(Config::EEPROM_ADDR_MODE);
  if (m >= static_cast<uint8_t>(Mode::MODE_COUNT))
    m = 0;

  mode = static_cast<Mode>(m);
}

void save_state_if_needed() {
  if (!write_eeprom_flag) return;
  write_eeprom_flag = false;
  EEPROM.update(Config::EEPROM_ADDR_BRIGHTNESS, brightness_index);
  EEPROM.update(Config::EEPROM_ADDR_MODE, static_cast<uint8_t>(mode));
}

void next_brightness() {
  brightness_index = (brightness_index + 1) % Config::BRIGHTNESS_STEPS;
  write_eeprom_flag = true;
}

void next_mode() {
  mode = static_cast<Mode>((static_cast<uint8_t>(mode) + 1)
                            % static_cast<uint8_t>(Mode::MODE_COUNT));
  write_eeprom_flag = true;
}

Mode current_mode() { return mode; }


uint8_t gamma_corrected_brightness(){
  // const uint8_t brightness_levels[4] = {255, 96, 32, 8}; //"nominally" gamma 2.2 corrected values provided by ChatGPT. I need to properly revisi the math at some point.
  // return brightness_levels[brightness_index]; 
  return dim8_video(255 - 75*brightness_index);
}