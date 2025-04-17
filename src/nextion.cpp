#include "nextion.h"

page NextionInterface::current_page = page::LOADING;

uint8_t NextionInterface::waterTemp = 1;
uint8_t NextionInterface::oilTemp = 1;
uint8_t NextionInterface::oilPressure = 1;
float NextionInterface::batteryVoltage = -1;
uint16_t NextionInterface::engineRPM = 1;
float NextionInterface::lambda = -1;
char NextionInterface::gear = '?';

bool NextionInterface::neutral = false;

NextionInterface::NextionInterface() {}

void NextionInterface::init()
{
    Serial2.begin(9600);
    delay(200);
    Serial.println("Nextion Setup");
    switchToLoading();
}

short NextionInterface::ctof(short celsius)
{
    return (celsius * 9 / 5) + 32;
}

void NextionInterface::sendNextionMessage(String message)
{
    Serial.println(message);
    Serial2.print(message);
    Serial2.write(0xFF);
    Serial2.write(0xFF);
    Serial2.write(0xFF);
}

void NextionInterface::setWaterTemp(int value)
{
    if(value != waterTemp){
        waterTemp = value;

        String instruction = "watertempvalue.txt=\"" + String(ctof(value), DEC) + " " + char(176) + "F\"";
        sendNextionMessage(instruction);
    }
}

void NextionInterface::setOilTemp(uint8_t value)

{
    if(value != oilTemp){
        oilTemp = value;
       
        String instruction = "oiltempvalue.txt=\"" + String(ctof(value), DEC) + " " + char(176) + "F\"";
        sendNextionMessage(instruction);
    }
}

void NextionInterface::setOilPressure(uint8_t value, uint8_t value2)
{
    // We're just getting it as an int for now, not even rounding.
    uint8_t newOilPressure = (value2 | (value << 8)) * 0.01;
    // get one decimal of precision
    if(oilPressure != newOilPressure){
        oilPressure = newOilPressure;
        String instruction = "oilpressvalue.txt=\"" + (String) oilPressure + " PSI\"";
        sendNextionMessage(instruction);
    }
}

void NextionInterface::setVoltage(float value)
{
    if (value != batteryVoltage) {
        batteryVoltage = value;
        batteryVoltage = value;

        String instruction = "voltvalue.txt=\"" + String(value, 1) + " V\"";
        sendNextionMessage(instruction);
    }
}

void NextionInterface::setRPM(uint16_t value)
{
    if(value != engineRPM){
        engineRPM = value;
        int roundedValue = (value / 100)*100;

        String instruction = "rpm.txt=\"" + String(roundedValue, DEC) + "\"";
        sendNextionMessage(instruction);
    }   
}

void NextionInterface::setGear(int numGear)
{
    // Serial.println(numGear);
    char newGear;
    if (numGear == 0){
        newGear = 'N';
    }else{
        newGear = '0' + (numGear);
    }


    if (newGear != gear) {
        gear = newGear;
        Serial.println(gear);
        String instruction = "gear.txt=\"" + String(gear) + '\"';
        sendNextionMessage(instruction);
    }
}

void NextionInterface::setButtonImage(String elementName, bool value) {

    // String instruction = "";

    // if (value) {
    //     instruction = elementName + ".pic=" + String(GREEN_BUTTON_ID, DEC);
    // } else {
    //     instruction = elementName + ".pic=" + String(RED_BUTTON_ID, DEC);
    // }

    // sendNextionMessage(instruction);

}

void NextionInterface::setFuelPumpBool(bool value)
{
    setButtonImage("fuelpumpbool", value);
}

void NextionInterface::setFanBool(bool value)
{
    setButtonImage("fanbool", value);
}

void NextionInterface::setWaterPumpBool(bool value)
{
    setButtonImage("waterpumpbool", value);
}

void NextionInterface::setMLIBool(bool value) {
    setButtonImage("mlibool", value);
}

void NextionInterface::setMessageBool(bool value) {
    setButtonImage("messagebool", value);
}

void NextionInterface::setFuelPumpValue(bool value)
{
//     String instruction = "";

//     if (value)
//     {
//         instruction = "fuelpumpvalue.bco=" + String(RGB565_GREEN, DEC);
//         sendNextionMessage(instruction);

//         instruction = "fuelpumpvalue.txt=\"ON\"";
//         sendNextionMessage(instruction);
//     }
//     else
//     {
//         instruction = "fuelpumpvalue.bco=" + String(RGB565_RED, DEC);
//         sendNextionMessage(instruction);

//         instruction = "fuelpumpvalue.txt=\"OFF\"";
//         sendNextionMessage(instruction);
//     }
}

void NextionInterface::setFanValue(bool value)
{
//     // String instruction = "";

//     // if (value)
//     // {
//     //     instruction = "fanvalue.bco=" + String(RGB565_GREEN, DEC);
//     //     sendNextionMessage(instruction);

//     //     instruction = "fanvalue.txt=\"ON\"";
//     //     sendNextionMessage(instruction);
//     // }
//     // else
//     // {
//     //     instruction = "fanvalue.bco=" + String(RGB565_RED, DEC);
//     //     sendNextionMessage(instruction);

//     //     instruction = "fanvalue.txt=\"OFF\"";
//     //     sendNextionMessage(instruction);
//     // }
}

void NextionInterface::setWaterPumpValue(bool value)
{
//     // String instruction = "";

//     // if (value)
//     // {
//     //     instruction = "waterpumpvalue.bco=" + String(RGB565_GREEN, DEC);
//     //     sendNextionMessage(instruction);

//     //     instruction = "waterpumpvalue.txt=\"ON\"";
//     //     sendNextionMessage(instruction);
//     // }
//     // else
//     // {
//     //     instruction = "waterpumpvalue.bco=" + String(RGB565_RED, DEC);
//     //     sendNextionMessage(instruction);

//     //     instruction = "waterpumpvalue.txt=\"OFF\"";
//     //     sendNextionMessage(instruction);
//     // }
}

void NextionInterface::setLambda(float value)
{
    if (value != lambda) {
        lambda = value;

        // float max = 1.5;
        // float high = 1.2;
        // float low = 0.8;
        // float min = 0.5;
        Serial.println(lambda);
        String instruction = "lambdabool.txt=\"" + String(value, 3) + " LA\"";
        sendNextionMessage(instruction);
    
        // if (value > max || value < min)
        // {
        //     instruction = "lambdavalue.bco=" + String(RGB565_RED, DEC);
        //     sendNextionMessage(instruction);
        // }
        // else if (value > high || value < low)
        // {
        //     instruction = "lambdavalue.bco=" + String(RGB565_ORANGE, DEC);
        //     sendNextionMessage(instruction);
        // }
        // else
        // {
        //     instruction = "lambdavalue.bco=" + String(RGB565_GREEN, DEC);
        //     sendNextionMessage(instruction);
        // }
    }
}

void NextionInterface::setNeutral(bool value)
{
    if(value != neutral){
        neutral = value;
        if(value) {
            sendNextionMessage("gear.txt=\"N\"");
        }
    }
}

void NextionInterface::switchToLoading()
{
    sendNextionMessage("page loading");
    current_page = page::LOADING;
}

void NextionInterface::switchToStartUp()
{
    sendNextionMessage("page startup");
    current_page = page::STARTUP;
}

void NextionInterface::switchToDriver()
{
    sendNextionMessage("page driver");
    current_page = page::DRIVER;
}

void NextionInterface::switchToYippee()
{
    sendNextionMessage("page yippee");
    current_page = page::YIPPEE;
}

void NextionInterface::switchToWarning() {
    sendNextionMessage("page warning");
    current_page = page::WARNING;
}


page NextionInterface::getCurrentPage()
{
    return current_page;
}