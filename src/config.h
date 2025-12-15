#pragma once
#include <stdint.h>
#include <FastLED.h>

namespace Config {

// PATTERN CONTROL
constexpr uint16_t CYCLE_TIME_SECONDS_SLOW = 300; //Max of 638 seconds! 
constexpr uint8_t CYCLE_TIME_SECONDS_FAST = 10; //Min of 3 - min step time is 10ms (timer) = 2.56sec/cycle
constexpr uint8_t COLOR_HUE = 80; // Green for minecraft pumpkins - scaled to 0..255 instead of 0..360deg
constexpr uint8_t SPARK_HUE = 190;


// HARDWARE CONFIG
constexpr uint8_t DATA_PIN = PIN_PB4;          // Physical PB4
constexpr uint8_t CLK_PIN = PIN_PB5;           // Physical PB5
constexpr uint8_t TICK_OUT_BIT = PIN0_bp;      // Physical PC0, accessed through VPORTC
constexpr uint8_t BRIGHTNESS_BUTTON_BIT = PIN0_bp; // Physical PB0
constexpr uint8_t MODE_BUTTON_BIT = PIN2_bp;        // Physical PA2
constexpr uint8_t NUM_LEDS = 10;
constexpr fl::EOrder LED_COLOR_ORDER = BGR;
constexpr uint8_t EEPROM_ADDR_BRIGHTNESS = 0x01;
constexpr uint8_t EEPROM_ADDR_MODE = 0x02;


// BRIGHTNESS and MODES
constexpr uint8_t BRIGHTNESS_STEPS = 4; //Number of steps to divide min 50 and max 255 into.

} // namespace Config
