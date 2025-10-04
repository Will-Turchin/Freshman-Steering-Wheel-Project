#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#ifndef NEOPIXEL_H
#define NEOPIXEL_H


class RevLights {
public:
    constexpr static const int LED_PINS = 26;
    constexpr static const int NUM_PIXELS = 12;

    constexpr static const int REDLINE = 13000;
    constexpr static const int SHIFT_POINT = 12000;
    constexpr static const int RPM_DIFFERENCE = 500;
    
    // Were making an array of leds that each have an rpm threshold and a color
    struct ledRPMThreshold {
        int threshold;
        uint32_t color;
    };

    static Adafruit_NeoPixel pixels;
    // in GRB
    constexpr static const int LED_COLOR_RED = 0x00FF00;
    constexpr static const int LED_COLOR_GREEN = 0xFF0000;
    constexpr static const int LED_COLOR_BLUE = 0x0000FF;
    constexpr static const int LED_COLOR_YELLOW = 0x7FFF00;
    constexpr static const int LED_COLOR_OFF = 0;
    static ledRPMThreshold *ledRPMThresholds;
    static void begin(uint8_t brightness = 75, bool initSerial = false, uint32_t serialBaud = 9600);
    static void updateLights(int rpm);
    static void teardown(); 
    static void serialBegin();
};


#endif