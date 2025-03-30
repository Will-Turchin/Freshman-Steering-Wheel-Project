#include "neopixel.h"

// Define static variables outside the class
RevLights::ledRPMThreshold* RevLights::ledRPMThresholds = nullptr; //init pointer 

Adafruit_NeoPixel RevLights::pixels(RevLights::NUM_PIXELS, RevLights::LED_PINS, NEO_GRB + NEO_KHZ800);