#include "neopixel.h"
Adafruit_NeoPixel RevLights::pixels(RevLights::NUM_PIXELS, RevLights::LED_PINS, NEO_GRB + NEO_KHZ800);
RevLights::ledRPMThreshold* RevLights::ledRPMThresholds = nullptr;

void RevLights::begin(uint8_t brightness, bool initSerial, uint32_t serialBaud)
    {
        if(initSerial){
            Serial.begin(serialBaud);
            Serial.println("RevLights begin()");
        }

        if(!ledRPMThresholds){
            ledRPMThresholds = new ledRPMThreshold[NUM_PIXELS];
        }

        pixels.begin();
        pixels.setBrightness(brightness);
        pixels.clear();
        pixels.show();
        for (int i = 8; i < 12; i++) {
            ledRPMThresholds[i].threshold = SHIFT_POINT;
            ledRPMThresholds[i].color = LED_COLOR_BLUE;
        }
        for (int i = 7; i >= 0; i--) {
            ledRPMThresholds[i].threshold = ledRPMThresholds[i+1].threshold - RPM_DIFFERENCE;
            if (i >= 4) {
                ledRPMThresholds[i].color = LED_COLOR_GREEN;
            } else {
                ledRPMThresholds[i].color = LED_COLOR_YELLOW;
            }
        }
        delay(100);
        updateLights(0);
}
void  RevLights::updateLights(int rpm) //DEV NOTE: If this class is failing, it likely means data types arent being initialized
    {   
        
    pixels.clear();

    if (rpm >= REDLINE) {
        // All red at/over redline
        for (int i = 0; i < NUM_PIXELS; ++i) {
            pixels.setPixelColor(i, LED_COLOR_RED);
        }
    } else if (rpm == 0) {
        // All green when engine is off
        for (int i = 0; i < NUM_PIXELS; ++i) {
            pixels.setPixelColor(i, LED_COLOR_GREEN);
        }
    } else {
        // Fill according to thresholds
        for (int i = 0; i < NUM_PIXELS; ++i) {
            if (rpm >= ledRPMThresholds[i].threshold) {
                pixels.setPixelColor(i, ledRPMThresholds[i].color);
            } else {
                pixels.setPixelColor(i, LED_COLOR_OFF);
            }
        }
    }

    pixels.show();
    }



    