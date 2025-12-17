/*
  main.c RGB LED Effects for addressable LEDs. Allows for changing of effects and brightness with buttons.
*/
#include <FastLED.h>
#include <Arduino.h>
#include <stdint.h>
#include <EEPROM.h>
#include "button.h"
#include "input.h"
#include "config.h"
#include "hardware.h"
#include "state.h"


CRGB leds[Config::NUM_LEDS];

uint8_t phase[Config::NUM_LEDS];
uint8_t speed[Config::NUM_LEDS];
uint8_t sparkLevel[Config::NUM_LEDS]; // 0..255 intensity of white pop

uint8_t hue_val = 0;
uint8_t sat_val = 0;
uint8_t step_time = 0;
unsigned long update_time = 0;


uint8_t dim_white_to_color(uint8_t color_sat_val) {
  //value scaling here (and elsewhere) does a good job of matching the apparent brightness of white to the color (hue=23).
  return 175 + scale8(cubicwave8(color_sat_val),80);
}


void setupFlame() {
  for (uint8_t i = 0; i < Config::NUM_LEDS; i++) {
    phase[i] = random8();
    speed[i] = random8(2, 7);  // each LED speed variation
    sparkLevel[i] = 0;
  }
}


void updateSparkFlameSaturation() {
  for (uint8_t i = 0; i < Config::NUM_LEDS; i++) {
    // advance waveform
    phase[i] += speed[i];
    uint8_t wave = quadwave8(phase[i]);  // smooth 0..255 triangle-like wave

    // occasionally trigger a bright spark
    if (random8() < 3) {  // ~1-2% chance each tick
      sparkLevel[i] = dim_white_to_color(0); // full white flash
    } else {
      // decay spark over time
      sparkLevel[i] = qsub8(sparkLevel[i], 10); // subtract with floor at 0
    }

    // blend between base color and white using sparkLevel and wave
    uint8_t flicker = scale8(wave, 180); // modulate overall intensity
    uint8_t sat = 200 - scale8(sparkLevel[i], 180); // desaturate during spark
    uint8_t val = 255;

    // base color hue
    leds[i] = CHSV(Config::COLOR_HUE, sat, val);

    // overlay white spark burst
    if (sparkLevel[i] > 0) {
      leds[i] += CHSV(0, 0, sparkLevel[i]); // add pure white
    }
  }
}

void setup () {
  init_CPU();
  init_timer();
  init_gpio();
  delay(100);
  load_state();

  delay(200);
  FastLED.addLeds<APA102, Config::DATA_PIN, Config::CLK_PIN, Config::LED_COLOR_ORDER>(leds, Config::NUM_LEDS);
  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setTemperature(UncorrectedTemperature);
}

void loop() {
  
  while(1) {
    unsigned long now = tick_10ms;
    switch (current_mode())
    {
    case Mode::RAINBOW_SOLID_SLOW: //Slowly fade all leds toghether around the rainbow
      step_time = (uint8_t)((uint16_t)Config::CYCLE_TIME_SECONDS_SLOW * 4 / 10); //4/10 is a close approximation of 1000ms/s / 256 step/cycle / 10ms tick. 16 bit math to avoid overflows
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val, 255, 255);
        }
        hue_val++;
      }
      break;

    case Mode::RAINBOW_SOLID_FAST: //Quickly fade all leds toghether around the rainbow
      step_time = (uint8_t)((uint16_t)Config::CYCLE_TIME_SECONDS_FAST * 4 / 10);
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val, 255, 255);
        }
        hue_val++;
      }
      break;

    case Mode::RAINBOW_WAVE_FAST: //Quickly fade all leds through the rainbow. The rainbow also fades one complete hue cycle across the LEDs
      step_time = (uint8_t)((uint16_t)Config::CYCLE_TIME_SECONDS_FAST * 4 / 10);
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val - i*255/(Config::NUM_LEDS), 255, 255);
        }
        hue_val++;
      }
      break;

    case Mode::COLOR_WHITE_PARADE: //LEDs fading from color to white and back, every other LED 180 degrees out of phase
      for (uint8_t i = 0; i < Config::NUM_LEDS; i++) {
        uint8_t t = (millis() / 16) % 255; // controls speed

        const uint8_t width = 4;
        uint8_t pos_color = (i * (256 / width)) - t; //order of ops matters for overflow - saves on using 16 bit math.
        uint8_t pos_white = (i * (256 / width)) - t - 128;

        uint8_t bright_color = cubicwave8(pos_color);
        uint8_t bright_white = cubicwave8(pos_white);

        leds[i] = CHSV(Config::COLOR_HUE, 255, bright_color) + CHSV(Config::COLOR_HUE, 0, scale8(bright_white,175));

      }
      break;

    case Mode::SOLID_COLOR: //LEDs are static color
      for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(Config::COLOR_HUE,255,255); //HSV Value here, and everywhere, further controlled by global brightness. Leave at 255!
      }
      break;

    case Mode::SOLID_WHITE: //LEDs are white
      for(uint8_t i = 0; i < Config::NUM_LEDS; i++) {
          leds[i] = CHSV(Config::COLOR_HUE,0,dim_white_to_color(0));
      }
      break;


    case Mode::SPARK_FLAME_WHITE: //CHAT-GPT assisted spark/pop flame effects
      if (now - update_time >= 10) {
        update_time = now;
        updateSparkFlameSaturation();
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
