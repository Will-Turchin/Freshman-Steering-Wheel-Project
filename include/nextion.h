#ifndef NEXTION_H
#define NEXTION_H

#include <Arduino.h>
#include "can.h"

enum page
{
    LOADING,
    STARTUP,
    DRIVER,
    YIPPEE,
    WARNING
};

class NextionInterface
{
private:
    static short ctof(short celsius);

    static page current_page;

    static void sendNextionMessage(String message);

    static int const RGB565_GREEN = 1472;
    static int const RGB565_ORANGE = 47936;
    static int const RGB565_RED = 45056;
    static int const RGB565_BLACK = 0;

    static int const GREEN_BUTTON_ID = 19;
    static int const RED_BUTTON_ID = 20;

    static bool neutral;
    static uint8_t waterTemp;
    static uint8_t oilTemp;
    static uint16_t oilPressure;
    static float batteryVoltage;
    static uint16_t engineRPM;
    static uint8_t lambda;
    static uint8_t gear;

public:
    NextionInterface();

    static void init();

    static void setWaterTemp(uint8_t value);

    static void setOilTemp(uint8_t value);

    static void setOilPressure(uint8_t value, uint8_t value2);

    static void setVoltage(float value);

    static void setRPM(uint16_t value);

    static void setGear(const CAN_message_t &msg);

    static void setButtonImage(String elementName, bool value);

    static void setFuelPumpBool(bool value);
    static void setFanBool(bool value);
    static void setWaterPumpBool(bool value);
    static void setMLIBool(bool value);
    static void setMessageBool(bool value);

    static void setFuelPumpValue(bool value);

    static void setFanValue(bool value);

    static void setWaterPumpValue(bool value);

    static void setLambda(float value);

    static void setNeutral(bool value);

    static void switchToLoading();
    static void switchToStartUp();
    static void switchToDriver();
    static void switchToYippee();
    static void switchToWarning();

    static page getCurrentPage();
};

#endif // NEXTION_H