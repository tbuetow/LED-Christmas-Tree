#pragma once
#include <stdint.h>

enum class Mode : uint8_t {
  RAINBOW_SOLID_SLOW, // Solid color - cycle slowly
  RAINBOW_SOLID_FAST, // Solic color - cycle quickly
  RAINBOW_WAVE_FAST, // rainbow of colors all the way around, rotating quickly
  GREEN_RED_PARADE, // Green and red fading between colors, marching around the circle
  GREEN_WHITE_PARADE, // Green and white marching around
  RED_WHITE_PARADE, // Red and white marching around
  SOLID_RED,
  SOLID_GREEN,
  SOLID_WHITE,
  SPARK_FLAME_WHITE,
  MODE_COUNT
};

void load_state();
void save_state_if_needed();

void next_mode();
void next_brightness();

Mode     current_mode();
uint8_t  gamma_corrected_brightness();
