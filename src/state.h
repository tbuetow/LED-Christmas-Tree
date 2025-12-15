#pragma once
#include <stdint.h>

enum class Mode : uint8_t {
  RAINBOW_SOLID_SLOW,
  RAINBOW_SOLID_FAST,
  RAINBOW_WAVE_FAST,
  COLOR_WHITE_PARADE,
  SOLID_COLOR,
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
