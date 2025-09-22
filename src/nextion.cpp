#include "nextion.h"
//Inits Nextion as Loading Screen
page NextionInterface::current_page = page::LOADING;
//Sets initial value
uint8_t NextionInterface::waterTemp = 999;
uint8_t NextionInterface::oilTemp = 999;
uint16_t NextionInterface::oilPressure = 999;
float NextionInterface::batteryVoltage = 999;
uint16_t NextionInterface::engineRPM = 999;
float NextionInterface::lambda = -1;
char NextionInterface::gear = '?';
uint16_t NextionInterface::prevmph = -1;  
uint16_t NextionInterface::currentMessage = 0;

bool NextionInterface::startupWaterTemp = false;
bool NextionInterface::startupWaterPump = false;
bool NextionInterface::startupOilTemp = false;
bool NextionInterface::startupOilPump = false;
bool NextionInterface::startupVoltage = false;
bool NextionInterface::startupSpeed = false;
bool NextionInterface::startupRPM = false;
bool NextionInterface::neutral = false;
bool NextionInterface::startupFan = false;
bool NextionInterface::startupFuelPump = false;
bool NextionInterface::startupMLI = false;
bool NextionInterface::startupMessage = false;
NextionInterface::NextionInterface() {}

void NextionInterface::init() {
    Serial2.begin(9600);
    delay(200);
    Serial.println("Nextion Setup");
    switchToLoading();
}
//Converts the given value from Celsius to Farenheight
short NextionInterface::ctof(short celsius) {
    return (celsius * 9 / 5) + 32;
}

short NextionInterface::kmhtomph(short kmh){
    return (kmh /1.6);
}

//Sends Message to message
void NextionInterface::sendNextionMessage(String message) {
    // Serial.println(message);
    Serial2.print(message);
    Serial2.write(0xFF);
    Serial2.write(0xFF);
    Serial2.write(0xFF);
}
//Sets the Water Temp on Screen
void NextionInterface::setWaterTemp(int value) {
    if(value != waterTemp){
        waterTemp = value;

        String instruction = "waterTempVar.txt=\"" + String(ctof(value), DEC) + " " + char(176) + "F\"";
        sendNextionMessage(instruction);
    }
    if(!startupWaterTemp){
        String instruction = "waterTempVar.txt=\"" + String("999") + " " + char(176) + "F\"";
        sendNextionMessage(instruction);
        startupWaterTemp = true;
    }
}
//Set Oil Temp
void NextionInterface::setOilTemp(uint8_t value) {
    if(value != oilTemp){
        oilTemp = value;

        String instruction = "oilTempVar.txt=\"" + String(ctof(value), DEC) + " " + char(176) + "F\"";
        sendNextionMessage(instruction);
    }
    if(!startupOilTemp){
        String instruction = "oilTempVar.txt=\"" + String("999") + " " + char(176) + "F\"";
        sendNextionMessage(instruction);
        startupOilTemp = true;

    }
}
//Set Oil Pressure and send string
void NextionInterface::setOilPressure(uint8_t value, uint8_t value2) {
    uint16_t newOilPressure = (((static_cast<uint16_t>(value2)) | (static_cast<uint16_t>(value) << 8)) * 0.0145);
    // get one decimal of precision
    if(oilPressure != newOilPressure){
        Serial.printf("value: 0x%X 0x%X\n", value, value2);
        oilPressure = newOilPressure;
        String instruction = "oilpPressureVar.txt=\"" + static_cast<String>(oilPressure) + " PSI\"";
        sendNextionMessage(instruction);
    }
}
//Sets voltage for current screen would have to be multiplied by 100
void NextionInterface::setVoltage(float value) {
    if (value != batteryVoltage) {
        batteryVoltage = value;
        batteryVoltage = value;

        String instruction = "voltageVar.txt=\"" + String(value, DEC) + " V\"";
        sendNextionMessage(instruction);
    }
}
//Send a message to the driver
void NextionInterface::setDriverMessage(uint16_t value) {
    if(value != currentMessage) {
        currentMessage = value;

        String instruction = "messageDriver.txt=\"" + String(value) + "\"";
        sendNextionMessage(instruction);
    }
}
//Set the RPM on the screen
void NextionInterface::setRPM(uint16_t value) {
    value = kmhtomph(value);
    if(value != engineRPM){
        engineRPM = value;
        int roundedValue = (value / 100)*100;

        String instruction = "rpmVar.txt=\"" + String(roundedValue, DEC) + "\"";
        sendNextionMessage(instruction);
    }   
    if(!startupRPM){
        String instruction = "rpmVar.txt=\"" + String("999") + "\"";
        sendNextionMessage(instruction);
        startupRPM = true;
    }
}
//Set Gear level  can remove and fix nextion screen
void NextionInterface::setGear(int numGear) {
    char newGear;
    if (numGear == 0){
        newGear = 'N';
    }else{
        newGear = '0' + (numGear);
    }


    if (newGear != gear) {
        gear = newGear;
        Serial.println(gear);
        String instruction = "gearShiftVar.txt=\"" + String(gear) + '\"';
        sendNextionMessage(instruction);
    }
}
void NextionInterface::setButtonImage(String elementName, bool value) {

    String instruction = "";
    
    if (!value) {
        instruction = elementName + ".pic=" + String(RED_BUTTON_ID);

    } else{
        instruction = elementName + ".pic=" + String(GREEN_BUTTON_ID);
    }

    sendNextionMessage(instruction);
}

void NextionInterface::setFuelPumpBool(bool value) {
    setButtonImage("fuelPumpVar", value);
    if(!startupFuelPump){
        setButtonImage("fuelPumpVar", 1);
        startupFuelPump = true;
    }
}

void NextionInterface::setFanBool(bool value) {
    setButtonImage("fanVar", value);
    if(!startupFan){
        setButtonImage("fanVar", 1);
        startupFan = true;
    }
}

void NextionInterface::setWaterPumpBool(bool value) {
    setButtonImage("waterPumpVar", value);
    if(!startupWaterPump){
        setButtonImage("waterPumpVar", 1);
        startupWaterPump = true;
    }
}

void NextionInterface::setMLIBool(bool value) {
    setButtonImage("MLIVar", value);
    if(!startupMLI){
        setButtonImage("MLIVar", 1);
        startupMLI = true;
    }
}

void NextionInterface::setMessageBool(bool value) {
    setButtonImage("MessageVar", value);
    if(!startupMessage){
        setButtonImage("MessageVar", 1);
        startupMessage = true;
    }
}

void NextionInterface::setLambda(float value) {
    if (value != lambda) {
        lambda = value;
        // to give context, these are values from Powertrain
        // float max = 1.5;
        // float high = 1.2;
        // float low = 0.8;
        // float min = 0.5;
        Serial.println(lambda);
        String instruction = "lambda.txt=\"" + String(value, 3) + " LA\"";
        sendNextionMessage(instruction);
    }
}

void NextionInterface::setNeutral(bool value) {
    if(value != neutral){
        neutral = value;
        if(value) {
            sendNextionMessage("gearShiftVar.txt=\"N\"");
        }
    }
}

void NextionInterface::setSpeed(int value){
    if(prevmph != value){
        prevmph = value;
        String instruction = "speedVar.txt=\"" + String(value, DEC) + " MPH" + "\"";
        sendNextionMessage(instruction);
    }
}

/*
The following switch the pages of the screen, fil in each of them
The first is given as an example
*/

void NextionInterface::switchToLoading() {
    if(current_page != page::LOADING){
        sendNextionMessage("page LOADING");
        current_page = page::LOADING;
    }  
}

void NextionInterface::switchToStartUp() {
    if(current_page != page::STARTUP){
        sendNextionMessage("page STARTUP");
        current_page = page::STARTUP;
    }
}

void NextionInterface::switchToDriver() {
    if(current_page != page::DRIVER){
        sendNextionMessage("page DRIVER");
        current_page = page::DRIVER;
    }
}

void NextionInterface::switchToYippee() {
    
}

void NextionInterface::switchToWarning() {
    if(current_page != page::WARNING){
        sendNextionMessage("page WARNING");
        current_page = page::WARNING;
    }
}


page NextionInterface::getCurrentPage() {
    return current_page;
}