/*
  main.c RGB LED Effects for addressable LEDs. Allows for changing of effects and brightness with buttons.
*/
#include <FastLED.h>
#include <Arduino.h>
#include <stdint.h>
#include <EEPROM.h>
#include "button.h"
#include "config.h"
#include "hardware.h"
#include "state.h"


CRGB leds[Config::NUM_LEDS];
constexpr uint16_t RAINBOW_STEPS = 256;
constexpr uint32_t RAINBOW_SLOW_STEP_MS = (Config::CYCLE_TIME_SECONDS_SLOW * 1000UL) / RAINBOW_STEPS;
constexpr uint32_t RAINBOW_FAST_STEP_MS = (Config::CYCLE_TIME_SECONDS_FAST * 1000UL) / RAINBOW_STEPS;


constexpr uint8_t PARADE_STEP_MS = (Config::PARADE_CYCLE_TIME_SECONDS * 1000UL) / RAINBOW_STEPS;
constexpr uint8_t TWINKLE_STEP_MS = 16;
constexpr uint8_t TWINKLE_PEAK = 220;
constexpr uint8_t TWINKLE_UP_STEP = TWINKLE_PEAK / (Config::TWINKLE_RISE_TIME_MS / TWINKLE_STEP_MS);
constexpr uint8_t TWINKLE_DOWN_STEP = TWINKLE_PEAK / (Config::TWINKLE_FALL_TIME_MS / TWINKLE_STEP_MS);
constexpr uint8_t TWINKLE_REST_STEP = TWINKLE_PEAK / (Config::TWINKLE_REST_TIME_MS / TWINKLE_STEP_MS);

constexpr uint8_t NOISE_STEP_MS = 16;
constexpr uint8_t MIDPOINT = 127;

uint8_t twinkleValue[Config::NUM_LEDS];
bool twinkleRising[Config::NUM_LEDS];
uint8_t twinkleRest[Config::NUM_LEDS];

uint8_t hue_val = 0;
uint8_t parade_phase = 0;
uint16_t holidayNoiseZ = 0;


uint8_t dim_white_to_color(uint8_t color_sat_val) {
  //value scaling here (and elsewhere) does a good job of matching the apparent brightness of white to the color (hue=23).
  return 175 + scale8(cubicwave8(color_sat_val),80);
}

CRGB holidayNoiseColor(uint8_t noiseVal) {
  const CRGB red = CHSV(Config::RED_HUE, 255, 255);
  const CRGB green = CHSV(Config::GREEN_HUE, 255, 255);
  const CRGB white = CHSV(0, 0, 200);

  const uint8_t lowerBlendStart = MIDPOINT - Config::BLEND_DISTANCE; // e.g. 27 when BLEND_DISTANCE=100
  const uint8_t upperBlendEnd   = MIDPOINT + Config::BLEND_DISTANCE; // clamp handled by uint8 wrap avoidance

  if (noiseVal <= lowerBlendStart) {
    return red; // 0..lowerBlendStart
  }
  if (noiseVal <= MIDPOINT) {
    // lowerBlendStart..MIDPOINT: red -> white
    uint8_t range = MIDPOINT - lowerBlendStart;
    uint8_t mix = ((uint16_t)(noiseVal - lowerBlendStart) * 255) / range;
    return blend(red, white, mix);
  }
  if (noiseVal <= upperBlendEnd) {
    // MIDPOINT..upperBlendEnd: white -> green
    uint8_t range = upperBlendEnd - MIDPOINT;
    uint8_t mix = ((uint16_t)(noiseVal - MIDPOINT) * 255) / range;
    return blend(white, green, mix);
  }
  return green; // above upperBlendEnd
}

void updateTwinkles(uint8_t baseHue) {
  for (uint8_t i = 0; i < Config::NUM_LEDS; i++) {
    // Decide if a new twinkle should start
    if (!twinkleRising[i] && twinkleValue[i] == 0 && twinkleRest[i] == 0 && random8(100) < Config::TWINKLE_CHANCE) {
      twinkleRising[i] = true;
      twinkleRest[i] = TWINKLE_PEAK;
    }

    if (twinkleRising[i]) {
      twinkleValue[i] = qadd8(twinkleValue[i], TWINKLE_UP_STEP);
      if (twinkleValue[i] >= TWINKLE_PEAK) {
        twinkleValue[i] = TWINKLE_PEAK;
        twinkleRising[i] = false;
      }
    } else if (twinkleValue[i] > 0) {
      twinkleValue[i] = qsub8(twinkleValue[i], TWINKLE_DOWN_STEP);
    } else {
      twinkleRest[i] = qsub8(twinkleRest[i],TWINKLE_REST_STEP);
    }

    CRGB base = CHSV(baseHue, 255, 200);
    leds[i] = base + CHSV(0, 0, twinkleValue[i]);
  }
}


void setup () {
  init_CPU();
  init_gpio();
  delay(50);
  load_state();

  delay(100);
  FastLED.addLeds<APA102, Config::DATA_PIN, Config::CLK_PIN, Config::LED_COLOR_ORDER>(leds, Config::NUM_LEDS);
  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setTemperature(UncorrectedTemperature);
}

void loop() {
  
  while(1) {
    Mode mode = current_mode();
    static Mode prevMode = mode;
    if (mode != prevMode) {
      for (uint8_t i = 0; i < Config::NUM_LEDS; i++) {
        twinkleValue[i] = 0;
        twinkleRising[i] = false;
        twinkleRest[i] = 0;
      }
      fill_solid(leds, Config::NUM_LEDS, CRGB::Black);
      prevMode = mode;
    }

    EVERY_N_MILLIS_I(buttonPollTimer, Config::BUTTON_SAMPLE_MS) { service_buttons(); }

    switch (mode)
    {
    case Mode::RAINBOW_SOLID_SLOW: //Slowly fade all leds toghether around the rainbow
      EVERY_N_MILLIS_I(rainbowSolidSlowTimer, RAINBOW_SLOW_STEP_MS) {
        fill_solid(leds, Config::NUM_LEDS, CHSV(hue_val++, 255, 255));
      }
      break;

    case Mode::RAINBOW_SOLID_FAST: //Quickly fade all leds toghether around the rainbow
      EVERY_N_MILLIS_I(rainbowSolidFastTimer, RAINBOW_FAST_STEP_MS) {
        fill_solid(leds, Config::NUM_LEDS, CHSV(hue_val++, 255, 255));
      }
      break;

    case Mode::RAINBOW_WAVE_FAST: //Quickly fade all leds through the rainbow. The rainbow also fades one complete hue cycle across the LEDs
      EVERY_N_MILLIS_I(rainbowWaveFastTimer, RAINBOW_FAST_STEP_MS) {
        for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val - i*255/(Config::NUM_LEDS), 255, 255);
        }
        hue_val++;
      }
      break;

    case Mode::GREEN_WHITE_PARADE: //LEDs fading between colors, marching around the circle
    case Mode::RED_WHITE_PARADE:
    case Mode::GREEN_RED_PARADE:
      EVERY_N_MILLIS_I(colorWhiteParadeTimer, PARADE_STEP_MS) {
        parade_phase = (parade_phase + 1) % 255;
      }
      for (uint8_t i = 0; i < Config::NUM_LEDS; i++) {
        uint8_t t = parade_phase; // controls speed

        const uint8_t width = 5;
        uint8_t pos_color = (i * (256 / width)) - t; //order of ops matters for overflow - saves on using 16 bit math.
        uint8_t pos_white = (i * (256 / width)) - t - 128;

        uint8_t bright_color = cubicwave8(pos_color);
        uint8_t bright_white = cubicwave8(pos_white);
        if (current_mode() == Mode::GREEN_WHITE_PARADE){
          leds[i] = CHSV(Config::GREEN_HUE, 255, bright_color) + CHSV(Config::GREEN_HUE, 0, scale8(bright_white,200));
        } else if (current_mode() == Mode::RED_WHITE_PARADE) {
          leds[i] = CHSV(Config::RED_HUE, 255, bright_color) + CHSV(Config::RED_HUE, 0, scale8(bright_white,200));
        } else {
          leds[i] = CHSV(Config::RED_HUE, 255, bright_color) + CHSV(Config::GREEN_HUE, 255, bright_white);
        }
      }
      break;

    case Mode::TWINKLE_RED:
    case Mode::TWINKLE_GREEN:
      EVERY_N_MILLIS_I(twinkleTimer, TWINKLE_STEP_MS) {
        updateTwinkles(mode == Mode::TWINKLE_RED ? Config::RED_HUE : Config::GREEN_HUE);
      }
      break;

    case Mode::HOLIDAY_NOISE:
      EVERY_N_MILLIS_I(noiseTimer, NOISE_STEP_MS) {
        holidayNoiseZ += Config::NOISE_Z_STEP;
        for (uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          uint16_t x = i * Config::NOISE_SAMPLE_DISTANCE;
          uint8_t noiseVal = inoise8(x, holidayNoiseZ);
          leds[i] = holidayNoiseColor(noiseVal);
        }
      }
      break;

    case Mode::SOLID_RED:
      for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(Config::RED_HUE,255,255);
        }
      break;
    case Mode::SOLID_GREEN: //LEDs are static color
      for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
        leds[i] = CHSV(Config::GREEN_HUE,255,255);
      }
      break;

    case Mode::SOLID_WHITE: //LEDs are white
      for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(0,0,175);
      }
      break;

  default:
      break;
    }
    FastLED.setBrightness(gamma_corrected_brightness());
    VPORTC.OUT |= (1 << Config::TICK_OUT_BIT); //rising edge on start of LED write out
    FastLED.show();  // Update LEDs. Want to run this as often as possible.
    VPORTC.OUT &= ~(1 << Config::TICK_OUT_BIT); // pulse width = led write time, period = loop time
    save_state_if_needed();

    // VPORTC.OUT ^= (1 << TICK_OUT); //toggle the TICK_OUT pin for hardware debugging purposes
  }
}
