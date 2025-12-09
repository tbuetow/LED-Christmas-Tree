/*
  main.c Slowly fades an RGB LED around a hue wheel, using Timer0 and Timer1
*/
#include <FastLED.h> //Reqires C++! Everything else is standard C.
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <EEPROM.h>
#include <button.h>


// PATTERN CONTROL
#define CYCLE_TIME_SECONDS_SLOW 300 //Max of 638 seconds! 
#define CYCLE_TIME_SECONDS_FAST 10 //Min of 3 - min step time is 10ms (timer) = 2.56sec/cycle
#define COLOR_HUE 80 // Green for minecraft pumpkins
// #define COLOR_HUE 23 // Lakeview Orange - scaled to 0..255 instead of 0..360deg
// #define COLOR_HUE 202 // Purple - scaled to 0..255 instead of 0..360deg
#define SPARK_HUE 190

// HARDWARE CONFIG
#define CLK 2 //chip pin 8
#define DATA 0 //chip pin 5
#define TICK_OUT 1 //chip pin 6
#define BRIGHTNESS_BUTTON 3 //chip pin 2
#define MODE_BUTTON 4 //chip pin 3
#define NUM_LEDS 6
#define EEPROM_ADDR_BRIGHTNESS 0x01
#define EEPROM_ADDR_MODE 0x02


// BRIGHTNESS and MODES
#define BRIGHTNESS_STEPS 4 //Number of steps to divide min 50 and max 255 into.

// To add another mode, add it to this enum and then add the matching case block in the switch statement in the while loop.
enum Mode : uint8_t {
  RAINBOW_SOLID_SLOW,
  RAINBOW_SOLID_FAST,
  RAINBOW_WAVE_SLOW,
  RAINBOW_WAVE_FAST,
  COLOR_WHITE_FADE,
  COLOR_WHITE_PARADE,
  SOLID_COLOR,
  SOLID_WHITE,
  STATIC_COLOR_WHITE_FADE,
  SPARK_FLAME_WHITE,
  SPARK_FLAME_COLOR,
  MODE_COUNT
};


// OTHER GLOBALS
volatile unsigned long ten_millis = 0;
Mode mode = Mode::RAINBOW_SOLID_SLOW;
uint8_t brightness_index = 0;
CRGB leds[NUM_LEDS];
bool write_eeprom_flag = false;


// BUTTONS
Button btn_brightness(&PINB, BRIGHTNESS_BUTTON);
Button btn_mode(&PINB, MODE_BUTTON);


void enable_timer1() {
  cli();
  GTCCR |= (1 << PSR1); //reset prescaler
  TCCR1 |= (1 << CTC1); //clear on match
  TCCR1 &= ~0x0F; // clear all 4 prescaler bits. Something else must be setting them - random internet dude said millis() is on timer1 on ATTiny85...
  TCCR1 |= (1 << CS13) | (1 << CS11); // 1/512 prescaler
  OCR1C = 156; //compare match every 8MHz / 512 prescaler / 156 count = 100.1603 compares/sec or 9.984ms (0.16% error) OCR1C resets TCNT1
  OCR1A = 156; //compare match every 8MHz / 512 prescaler / 156 count = 100.1603 compares/sec or 9.984ms (0.16% error) OCR1C fires COMPA
  OCR1B = 77; //this will match every 10ms, but 5ms before OCR1A (as the timer is only cleared on C)
  TIMSK |= (1 << OCIE1A) | (1 << OCIE1B); //Interrupt on match for both comparators
  sei();
}

void set_cpu_speed() { 
  // See datasheet 6.5.2 - CLKPR
  cli();
  CLKPR = (1<<CLKPCE);
  CLKPR = 0x00; 
  sei();
}

void set_io_pins() {
  DDRB &= ~((1 << BRIGHTNESS_BUTTON) | (1 << MODE_BUTTON)); //Button pins are inputs
  PORTB |= (1 << BRIGHTNESS_BUTTON) | (1 << MODE_BUTTON); //Pull ups active

  DDRB |= (1 << TICK_OUT) | (1 << DATA) | (1 << CLK); //Tick and spi are outputs
}

void load_settings_from_eeprom() {
  brightness_index = EEPROM.read(EEPROM_ADDR_BRIGHTNESS);
  if (brightness_index >= BRIGHTNESS_STEPS){
    brightness_index = 0; //Reset on re-flash unless EEPROM save fuse set
  }
  
  uint8_t mode_val = EEPROM.read(EEPROM_ADDR_MODE);
  if (mode_val >= static_cast<uint8_t>(Mode::MODE_COUNT)) {
    mode_val = 0; //default on re-flash unless EEPROM save fuse set
  }
  mode = static_cast<Mode>(mode_val);
}

void write_settings_to_eeprom_conditional() {
  if (write_eeprom_flag) {
    write_eeprom_flag = false;
    EEPROM.update(EEPROM_ADDR_BRIGHTNESS, brightness_index);
    EEPROM.update(EEPROM_ADDR_MODE, static_cast<uint8_t>(mode));
  }
}

uint8_t get_brightness_from_index(){
  // const uint8_t brightness_levels[4] = {255, 96, 32, 8}; //"nominally" gamma 2.2 corrected values provided by ChatGPT. I need to properly revisi the math at some point.
  // return brightness_levels[brightness_index]; 
  return dim8_video(255 - 75*brightness_index);
}

void increment_brightness_index() {
  brightness_index = (brightness_index + 1) % BRIGHTNESS_STEPS;
  write_eeprom_flag = true;
}

void increment_mode() {
  mode = static_cast<Mode>((mode + 1) % MODE_COUNT);
  write_eeprom_flag = true;
}

uint8_t dim_white_to_color(uint8_t color_sat_val) {
  //value scaling here (and elsewhere) does a good job of matching the apparent brightness of white to the color (hue=23).
  return 175 + scale8(cubicwave8(color_sat_val),80);
}

ISR(TIMER1_COMPA_vect) {
  //This is a ~10ms timer
  ten_millis++;
  // PORTB ^= (1 << TICK_OUT); //toggle the TICK_OUT pin for hardware debugging purposes
}

ISR(TIMER1_COMPB_vect) {
  //Button debouncing updates
  btn_brightness.update();
  btn_mode.update();

  //Button actions
  if (btn_brightness.wasPressed()) {
    increment_brightness_index();
  }
  if (btn_mode.wasPressed()) {
    increment_mode();
  }
}

uint8_t phase[NUM_LEDS];
uint8_t speed[NUM_LEDS];
uint8_t sparkLevel[NUM_LEDS]; // 0..255 intensity of white pop

uint8_t hue_val = 0;
uint8_t sat_val = 0;
uint8_t step_time = 0;
unsigned long update_time = 0;


void setupFlame() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    phase[i] = random8();
    speed[i] = random8(2, 7);  // each LED speed variation
    sparkLevel[i] = 0;
  }
}

void updateSparkFlameColor() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // advance waveform
    phase[i] += speed[i];
    uint8_t wave = quadwave8(phase[i]);  // smooth 0..255 triangle-like wave

    // occasionally trigger a bright spark
    if (random8() < 7) {  // ~3% chance each tick
      sparkLevel[i] = 255; // full white flash
    } else {
      // decay spark over time
      sparkLevel[i] = qsub8(sparkLevel[i], 10); // subtract with floor at 0
    }

    // blend between base color and white using sparkLevel and wave
    uint8_t flicker = scale8(wave, 180); // modulate overall intensity
    CRGB base = CHSV(COLOR_HUE, 255, 255);
    CRGB sparkColor = CHSV(SPARK_HUE, 255, 255);

    // base color hue
    leds[i] = base;

    // overlay white spark burst
    if (sparkLevel[i] > 0) {
      leds[i] = blend(base, sparkColor, sparkLevel[i]);
    }
  }
}

void updateSparkFlameSaturation() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
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
    leds[i] = CHSV(COLOR_HUE, sat, val);

    // overlay white spark burst
    if (sparkLevel[i] > 0) {
      leds[i] += CHSV(0, 0, sparkLevel[i]); // add pure white
    }
  }
}

void setup () {
  set_cpu_speed();
  enable_timer1();
  set_io_pins();
  delay(100);
  load_settings_from_eeprom();

  delay(200);
  FastLED.addLeds<APA102HD, DATA, CLK, BGR>(leds, NUM_LEDS);
}

void loop() {
  
  while(1) {
    unsigned long now = ten_millis;
    switch (mode)
    {
    case RAINBOW_SOLID_SLOW: //Slowly fade all leds toghether around the rainbow
      step_time = (uint8_t)((uint16_t)CYCLE_TIME_SECONDS_SLOW * 4 / 10); //4/10 is a close approximation of 1000ms/s / 256 step/cycle / 10ms tick. 16 bit math to avoid overflows
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val, 255, 255);
        }
        hue_val++;
      }
      break;

    case RAINBOW_SOLID_FAST: //Quickly fade all leds toghether around the rainbow
      step_time = (uint8_t)((uint16_t)CYCLE_TIME_SECONDS_FAST * 4 / 10);
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val, 255, 255);
        }
        hue_val++;
      }
      break;

    case RAINBOW_WAVE_SLOW: //Slowly fade all leds through the rainbow. The rainbow also fades one complete hue cycle across the LEDs
      step_time = (uint8_t)((uint16_t)CYCLE_TIME_SECONDS_SLOW * 4 / 10);
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val - i*255/(NUM_LEDS)/2, 255, 255);
        }
        hue_val++;
      }
      break;

    case RAINBOW_WAVE_FAST: //Quickly fade all leds through the rainbow. The rainbow also fades one complete hue cycle across the LEDs
      step_time = (uint8_t)((uint16_t)CYCLE_TIME_SECONDS_FAST * 4 / 10);
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(hue_val - i*255/(NUM_LEDS), 255, 255);
        }
        hue_val++;
      }
      break;

    case COLOR_WHITE_PARADE: //LEDs fading from color to white and back, every other LED 180 degrees out of phase
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        uint8_t t = (millis() / 16) % 255; // controls speed

        const uint8_t width = 4;
        uint8_t pos_color = (i * (256 / width)) - t; //order of ops matters for overflow - saves on using 16 bit math.
        uint8_t pos_white = (i * (256 / width)) - t - 128;

        uint8_t bright_color = cubicwave8(pos_color);
        uint8_t bright_white = cubicwave8(pos_white);

        leds[i] = CHSV(COLOR_HUE, 255, bright_color) + CHSV(COLOR_HUE, 0, scale8(bright_white,175));

      }
      break;

    case COLOR_WHITE_FADE: //LEDs all together fading between color and white
      step_time = (uint8_t)((uint16_t)CYCLE_TIME_SECONDS_FAST * 4 / 10); //4/10 is a close approximation of 1000ms/s / 256 step/cycle / 10ms tick. 16 bit math to avoid overflows
      if (now - update_time >= step_time) {
        update_time = now;
        for(uint8_t i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(COLOR_HUE, cubicwave8(sat_val), dim_white_to_color(sat_val));
        }
        sat_val++;
      }
      break;

    case SOLID_COLOR: //LEDs are static color
      for(uint8_t i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(COLOR_HUE,255,255); //HSV Value here, and everywhere, further controlled by global brightness. Leave at 255!
      }
      break;

    case SOLID_WHITE: //LEDs are white
      for(uint8_t i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(COLOR_HUE,0,dim_white_to_color(0));
      }
      break;

    case STATIC_COLOR_WHITE_FADE: //static white on one end and color on the other
      for(uint8_t i = 0; i < NUM_LEDS; i++) {
          uint8_t fade = cubicwave8((255/(NUM_LEDS-1)/2)*i);
          leds[NUM_LEDS-1-i] = CHSV(COLOR_HUE, fade, dim_white_to_color(fade));
      }
      break;

    case SPARK_FLAME_WHITE: //CHAT-GPT assisted spark/pop flame effects
      if (now - update_time >= 10) {
        update_time = now;
        updateSparkFlameSaturation();
      }
      break;

    case SPARK_FLAME_COLOR: //CHAT-GPT assisted spark/pop flame effects
      if (now - update_time >= 10) {
        update_time = now;
        updateSparkFlameColor();
      }
      break;

    default:
      break;
    }
    FastLED.setBrightness(get_brightness_from_index());
    PORTB |= (1 << TICK_OUT); //rising edge on start of LED write out
    FastLED.show();  // Update LEDs. Want to run this as often as possible.
    PORTB &= ~(1 << TICK_OUT); // pulse width = led write time, period = loop time
    write_settings_to_eeprom_conditional();

    // PORTB ^= (1 << TICK_OUT); //toggle the TICK_OUT pin for hardware debugging purposes
  }
}