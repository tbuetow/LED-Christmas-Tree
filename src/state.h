#pragma once
#include <stdint.h>

enum class Mode : uint8_t {
  RAINBOW_SOLID_SLOW, // Solid color - cycle slowly
  RAINBOW_SOLID_FAST, // Solid color - cycle quickly
  RAINBOW_WAVE_FAST, // rainbow of colors all the way around, rotating quickly
  GREEN_RED_PARADE,  // Red and green marching colors
  GREEN_WHITE_PARADE, // Green and white marching colors
  RED_WHITE_PARADE, // Red and white marching colors
  TWINKLE_RED, // Red base with white twinkles
  TWINKLE_GREEN, // Green base with white twinkles
  HOLIDAY_NOISE, // Red/green/white organic swirl
  SOLID_RED, // Solid colors
  SOLID_GREEN,
  SOLID_WHITE,
  MODE_COUNT // Leave this here, not a real mode, required for accounting
};

void load_state();
void save_state_if_needed();

void next_mode();
void next_brightness();

Mode     current_mode();
uint8_t  gamma_corrected_brightness();
