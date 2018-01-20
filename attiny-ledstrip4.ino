#include "FastLED.h"

// LED strip
#define NUM_LEDS 60
#define DATA_PIN 4

// Color dots
#define MAX_DOTS 18 // Try lowering if you experience crashes (out of memory)
#define MIN_VELOCITY 3
#define MAX_VELOCITY 7
#define VELOCITY_DIVISOR 1
#define TRAIL_LENGTH 8
#define SPAWN_RATE 10000000 // Chance every frame (out of 2,147,483,647) that a new dot is spawned.

// Color
#define HUE_VELOCITY 100 // Larger means slower (increment every X frames)
#define COLOR_JITTER 35
#define COLOR_WHEEL_SPLITS 3
#define SATURATION 160
#define SATURATION_JITTER 80 // S = SATURATION + random(SATURATION_JITTER)
//#define VALUE 200
#define COLOR_CORRECTION TypicalSMD5050
//#define COLOR_CORRECTION 0xAF70FF

// Power limiter
// #define MAX_MA 2000

CRGB leds[NUM_LEDS];
long counter = 0;
uint8_t hue = 0;

struct dot {
  bool active; // whether the dot is still visible (i.e. can it be respawned?)
  int pos; // uses 255 steps per pixel to represent sub-pixel accuracy
  uint8_t velocity; // how many subpixels to move per iteration
  int8_t dir; // which direction is the dot moving in (-1 = left, 1 = right)
  CHSV color;
};

struct dot dots[MAX_DOTS];

void spawn_dot() {
  for (int i = 0; i < MAX_DOTS; i++) {
    if (!dots[i].active) {
      dots[i].active = true;

      bool startEdge = random(2);
      dots[i].pos = startEdge ? 0 : NUM_LEDS * 255;
      dots[i].velocity = random(MIN_VELOCITY - 1, MAX_VELOCITY) + 1;
      dots[i].dir = startEdge ? 1 : -1;

      //uint8_t rand_hue = hue + (255 / COLOR_WHEEL_SPLITS) * random(COLOR_WHEEL_SPLITS);
      uint8_t rand_hue = hue + COLOR_JITTER * random(COLOR_WHEEL_SPLITS);
        
      dots[i].color = CHSV(rand_hue, SATURATION + random(SATURATION_JITTER), 255);

      // we found an inactive dot, stop
      return;
    }
  }
}
void init_dot(dot d) {
  d.active = false;
  d.pos = -256;
  d.velocity = 0;
  d.dir = 1;
}

void setup() { 
  randomSeed(analogRead(0));
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(COLOR_CORRECTION);
  //FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MA);

  hue = random(256);

  for (int i = 0; i < MAX_DOTS; i++) {
    init_dot(dots[i]);
  }

  spawn_dot();
}

void loop() {
  counter++;

  if (counter % HUE_VELOCITY == 0) {
    //hue = (hue + 1) % 256;
    hue++;
  }

  if (random() < SPAWN_RATE) {
    spawn_dot();
  }

  // Clear out all leds
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Loop through each dot
  for (int i = 0; i < MAX_DOTS; i++) {
    if (!dots[i].active) {
      continue;
    }
    
    // Dot movement
    if (counter % VELOCITY_DIVISOR == 0) {
      dots[i].pos += dots[i].dir * dots[i].velocity;
    }
    
    // Convert subpixel pos to pixel pos
    int dot_pos = dots[i].pos / 256;

    if (dots[i].pos < 0) {
      // Because integer division truncates the decimal part,
      // we need to subtract one from negative numbers
      dot_pos--;
    }
    
    uint8_t sub_dot_pos = dots[i].pos;// % 256;
    
    // Subpixel pos reversed if dot travels left
    if (dots[i].dir == -1) {
      sub_dot_pos = 255 - sub_dot_pos;
    }

    // Draw dot and trail
    bool trail_was_in_bounds = false;
    for (int j = 0; j < TRAIL_LENGTH; j++) {
      int trail_pos = dot_pos + j * dots[i].dir * -1;

      // Bounds check
      if (trail_pos >= 0 && trail_pos < NUM_LEDS) {
        trail_was_in_bounds = true;

        CHSV temp = dots[i].color;

        if (!j) {
          // Fade in first led
          temp.v = sub_dot_pos;
        } else {
          // Fade out other leds in the trail
          temp.v = 255 - ((j - 1) * 255 + sub_dot_pos) / TRAIL_LENGTH;
          temp.v = ease8InOutQuad(temp.v);
        }
       
        leds[trail_pos] += temp;
      }
    }

    // If no part of the trail was in bounds, mark dot as inactive
    if (!trail_was_in_bounds) {
      dots[i].active = false;
    }
  }

  FastLED.show();
}
