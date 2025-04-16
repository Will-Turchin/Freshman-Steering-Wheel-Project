#include <Adafruit_NeoPixel.h>

#ifndef NEOPIXEL_H
#define NEOPIXEL_H


class RevLights
{
private:
    constexpr static const int LED_PINS = 26;
    constexpr static const int NUM_PIXELS = 12;

    constexpr static const int REDLINE = 13000;
    constexpr static const int SHIFT_POINT = 12000;
    constexpr static const int RPM_DIFFERENCE = 500;
    
    // Were making an array of leds that each have an rpm threshold and a color
    struct ledRPMThreshold {
        int threshold;
        int color;
    };

    static Adafruit_NeoPixel pixels;
    // in GRB
    constexpr static const int LED_COLOR_RED = 0x00FF00;
    constexpr static const int LED_COLOR_GREEN = 0xFF0000;
    constexpr static const int LED_COLOR_BLUE = 0x0000FF;
    constexpr static const int LED_COLOR_YELLOW = 0x7FFF00;
    constexpr static const int LED_COLOR_OFF = 0;


public:
    static ledRPMThreshold *ledRPMThresholds;
    RevLights()
    {
        Serial.println("RevLights constructor called.");
        ledRPMThresholds = new ledRPMThreshold[NUM_PIXELS]; //allocate memory
        init();
    }
    ~RevLights()
    {
        delete[] ledRPMThresholds; // Free allocated memory
    }

    void static init()
    {
        Serial.begin(9600);
        Serial.println("Rev Lights Initalized");
        ledRPMThresholds = new ledRPMThreshold[NUM_PIXELS]; // INITIALIZE ledRPMThreshold (REQUIRED)
        pixels.begin();// INITIALIZE NeoPixel strip object (REQUIRED)
        pixels.setBrightness(75); // Normal 150
        pixels.clear();
        pixels.show();
        
        
        // fill the thresholds array
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

    void static updateLights(int rpm) //DEV NOTE: If this class is failing, it likely means data types arent being initialized
    {   
        
        pixels.clear();
        if(rpm >= REDLINE){ //If RPM past redline set the revlights all to red
            for (int i = 0; i < 12; i++) {
                pixels.setPixelColor(i, LED_COLOR_RED);
            }
        } else {
            for (int i = 0; i < 12; i++) { //checks each led threshold, if met sets it to it's color
                if (rpm >= ledRPMThresholds[i].threshold) {
                    pixels.setPixelColor(i, ledRPMThresholds[i].color);
                } else {
                    pixels.setPixelColor(i, LED_COLOR_OFF);
                }
            }
            
        }
    
        pixels.show();
    }
};

#endif // NEOPIXEL_H