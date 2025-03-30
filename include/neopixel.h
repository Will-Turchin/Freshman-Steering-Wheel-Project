#include <Adafruit_NeoPixel.h>

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

class RevLights
{
private:
    static const int LED_PINS = 26;
    static const int NUM_PIXELS = 12;

    static const int REDLINE = 13000;
    static const int SHIFT_POINT = 12000;
    static const int RPM_DIFFERENCE = 500;
    
    // Were making an array of leds that each have an rpm threshold and a color
    struct ledRPMThreshold {
        int threshold;
        int color;
    };

    static Adafruit_NeoPixel pixels;

    static const int LIGHT_RED = (255 << 16) | (20 << 8) | 0;
    // FF1400
    static const int DARK_RED = (255 << 16) | (0 << 8) | 0;

    static const int LIGHT_GREEN = (120 << 16) | (255 << 8) | 0;
    static const int DARK_GREEN = (0 << 16) | (255 << 8) | 0;

    static const int LIGHT_BLUE = (0 << 16) | (250 << 8) | 187;
    static const int DARK_BLUE = (0 << 16) | (0 << 8) | 255;

    static const int YELLOW = (255 << 16) | (255 << 8) | 0;

    static const int BLANK = 0;


public:
    static ledRPMThreshold *ledRPMThresholds;
    RevLights()
    {
        ledRPMThresholds = new ledRPMThreshold[NUM_PIXELS];
        init();
    }
    ~RevLights()
    {
        delete[] ledRPMThresholds; // Free allocated memory
    }

    void static init()
    {
        pixels.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
        pixels.setBrightness(75); // Normal 150
        pixels.clear();
        pixels.show();

        delay(50);
        rpmBased(0);
        //ledRPMThresholds = new ledRPMThreshold[NUM_PIXELS];
        
        // fill the thresholds array
        for (int i = 8; i < 12; i++) {
            ledRPMThresholds[i].threshold = SHIFT_POINT;
            ledRPMThresholds[i].color = DARK_BLUE;
        }
        for (int i = 7; i >= 0; i--) {
            ledRPMThresholds[i].threshold = ledRPMThresholds[i+1].threshold - RPM_DIFFERENCE;

            if (i >= 4) {
                ledRPMThresholds[i].color = DARK_GREEN;
            } else {
                ledRPMThresholds[i].color = YELLOW;
            }
        }
    }

    void static rpmBased(int rpm)
    {
        for (int i = 0; i < 12; i++) { //checks each led threshold, if met sets it to it's color
            if (rpm >= ledRPMThresholds[i].threshold) {
                pixels.setPixelColor(i, ledRPMThresholds[i].color);
            } else {
                pixels.setPixelColor(i, BLANK);
            }
        }

        if(rpm >= REDLINE){ //If RPM past redline set the revlights all to red
            for (int i = 0; i < 12; i++) {
                pixels.setPixelColor(i, DARK_RED);
            }
        }
        pixels.show();
    }
};

#endif // NEOPIXEL_H