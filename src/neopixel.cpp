#include "neopixel.h"

RevLights::LED_PINS = 26;
RevLights::NUM_PIXELS = 12;

Adafruit_NeoPixel RevLights::pixels(NUM_PIXELS, LED_PINS, NEO_GRB + NEO_KHZ800);