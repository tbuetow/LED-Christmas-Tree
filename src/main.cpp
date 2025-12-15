/*
  main.c RGB LED Effects for addressable LEDs. Allows for changing of effects and brightness with buttons.
*/
#include <FastLED.h>
#include <Arduino.h>
#include <stdint.h>
#include <EEPROM.h>
#include <button.h>


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

// To add another mode, add it to this enum and then add the matching case block in the switch statement in the while loop.
enum Mode : uint8_t {
  RAINBOW_SOLID_SLOW,
  RAINBOW_SOLID_FAST,
  RAINBOW_WAVE_FAST,
  COLOR_WHITE_PARADE,
  SOLID_COLOR,
  SOLID_WHITE,
  SPARK_FLAME_WHITE,
  MODE_COUNT
};


// OTHER GLOBALS
volatile unsigned long ten_millis = 0;
Mode mode = Mode::RAINBOW_SOLID_SLOW;
uint8_t brightness_index = 0;
CRGB leds[NUM_LEDS];
bool write_eeprom_flag = false;


// BUTTONS
Button btn_brightness(&VPORTB.IN, BRIGHTNESS_BUTTON_BIT);
Button btn_mode(&VPORTA.IN, MODE_BUTTON_BIT);


// Match Arduino core's TCA prescaler selection so TCB1 can use CLKTCA as its clock.
// TCB1_COMPARE needs to be less than 16535.
constexpr uint32_t TCA_PRESCALER_DIVISOR = 16UL; // 20MHz / 16 = 1.25MHz
constexpr uint16_t TCB1_COMPARE = static_cast<uint16_t>((F_CPU / TCA_PRESCALER_DIVISOR / 100UL) - 1UL); // 20,000,000 / 16 / 100 = 12,500. -1 because 0 indexed

void enable_tick_timer() {
  cli();
  TCB1.CTRLA = 0x00;
  TCB1.CTRLB = TCB_CNTMODE_INT_gc; // Periodic interrupt mode
  TCB1.CCMP = TCB1_COMPARE;
  TCB1.INTFLAGS = TCB_CAPT_bm;
  TCB1.INTCTRL = TCB_CAPT_bm;
  TCB1.CTRLA = TCB_CLKSEL_CLKTCA_gc | TCB_ENABLE_bm; // Clocked from TCA (prescaled) to keep count in range
  sei();
}

void set_cpu_speed() { 
  // Ensure the main clock runs without a prescaler for consistent timing.
  _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00);
}

void set_io_pins() {
  // Button pins as inputs with pull-ups
  VPORTA.DIR &= ~(1 << MODE_BUTTON_BIT);
  VPORTB.DIR &= ~(1 << BRIGHTNESS_BUTTON_BIT);
  PORTA.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;
  PORTB.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;

  // Outputs: tick and APA102 signals
  VPORTB.DIR |= PIN4_bm | PIN5_bm;
  VPORTC.DIR |= (1 << TICK_OUT_BIT);
  VPORTC.OUT &= ~(1 << TICK_OUT_BIT);
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

ISR(TCB1_INT_vect) {
  //This is a ~10ms timer
  ten_millis++;
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
  enable_tick_timer();
  set_io_pins();
  delay(100);
  load_settings_from_eeprom();

  delay(200);
  FastLED.addLeds<APA102HD, DATA_PIN, CLK_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS);
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


    case SPARK_FLAME_WHITE: //CHAT-GPT assisted spark/pop flame effects
      if (now - update_time >= 10) {
        update_time = now;
        updateSparkFlameSaturation();
      }
      break;

  default:
      break;
    }
    FastLED.setBrightness(get_brightness_from_index());
    VPORTC.OUT |= (1 << TICK_OUT_BIT); //rising edge on start of LED write out
    FastLED.show();  // Update LEDs. Want to run this as often as possible.
    VPORTC.OUT &= ~(1 << TICK_OUT_BIT); // pulse width = led write time, period = loop time
    write_settings_to_eeprom_conditional();

    // VPORTC.OUT ^= (1 << TICK_OUT); //toggle the TICK_OUT pin for hardware debugging purposes
  }
}
