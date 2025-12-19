#pragma once
#include <stdint.h>
#include <FastLED.h>

namespace Config {

// PATTERN CONTROL //
//Rainbow Cycles
constexpr uint16_t CYCLE_TIME_SECONDS_SLOW = 300; 
constexpr uint8_t CYCLE_TIME_SECONDS_FAST = 10; //also used for rainbow wave

//Parade Cycles
constexpr uint8_t PARADE_CYCLE_TIME_SECONDS = 2;

//Twinkles
constexpr uint16_t TWINKLE_RISE_TIME_MS = 200;
constexpr uint16_t TWINKLE_FALL_TIME_MS = 1650;
constexpr uint8_t TWINKLE_CHANCE = 10; // percent chance every cycle time

//Solid colors
constexpr uint8_t GREEN_HUE = 97; // Green - scaled to 0..255 instead of 0..360deg
constexpr uint8_t RED_HUE = 0; // Red - scaled to 0..255 instead of 0..360deg


// HARDWARE CONFIG //
constexpr uint8_t DATA_PIN = PIN_PB4;          // Physical PB4
constexpr uint8_t CLK_PIN = PIN_PB5;           // Physical PB5
constexpr uint8_t TICK_OUT_BIT = PIN0_bp;      // Physical PC0, accessed through VPORTC
constexpr uint8_t BRIGHTNESS_BUTTON_BIT = PIN0_bp; // Physical PB0
constexpr uint8_t MODE_BUTTON_BIT = PIN2_bp;        // Physical PA2
constexpr uint8_t NUM_LEDS = 10;
constexpr fl::EOrder LED_COLOR_ORDER = RBG; //RBG usually for adafruit, BGR usually for Aliexpress vendor COCurve 
constexpr uint8_t EEPROM_ADDR_BRIGHTNESS = 0x01;
constexpr uint8_t EEPROM_ADDR_MODE = 0x02;


// BRIGHTNESS and MODES //
constexpr uint8_t BRIGHTNESS_STEPS = 4; //Number of steps to divide min 50 and max 255 into.
constexpr uint8_t BUTTON_SAMPLE_MS = 10;


} // namespace Config

