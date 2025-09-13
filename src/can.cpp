#include "Arduino.h"

#include "nextion.h"
#include "neopixel.h"

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanInterface::Can0; //Declare Object CanInterface

CanInterface::CanInterface(){
    // deprecated function
}

CAN_message_t CanInterface::shift_msg; //Receives message from teensy
bool CanInterface::canActive = false;

bool CanInterface::init(){ // Init Can Interface Probaly dont change lol
    pinMode(32,OUTPUT); digitalWrite(32,HIGH); 
    pinMode(33,OUTPUT); digitalWrite(33,HIGH);

    Can0.begin();
    Can0.setBaudRate(1000000); //needs to be million to talk with CAN
    Can0.setMaxMB(16);
    Can0.enableFIFO();
    Can0.enableFIFOInterrupt();
    Can0.onReceive(receive_can_updates);
    return 1;
}
//Declares cases for each of the following i.e. Overrun sets up an overflow flags
void CanInterface::print_can_sniff(const CAN_message_t &msg){
    Serial.print("MB "); Serial.print(msg.mb);
    Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
    Serial.print("  LEN: "); Serial.print(msg.len);
    Serial.print(" EXT: "); Serial.print(msg.flags.extended);
    Serial.print(" TS: "); Serial.print(msg.timestamp);
    Serial.print(" ID: "); Serial.print(msg.id, DEC);
    Serial.print(" Buffer: ");
    //Prints this in Decimal
    for ( uint8_t i = 0; i < msg.len; i++ ) {
        Serial.print(msg.buf[i], DEC); Serial.print(" ");
    } 
    Serial.println();
}
//Reads the and sets the values for all ideal places baced on box
void CanInterface::receive_can_updates(const CAN_message_t &msg) {
    canActive = true;


    switch (msg.id){
        /*
        TO DO:
        The following code contains the cases for each of the sensor values gathered from CAN.
        One of the cases below has been filled in as an example.

        Additionally, some of the cases are written as ints, others are in hexcode, convert all hexcode cases
        to their integer value.
        */
        case 1600:
            //rpm
            uint16_t rpm = (msg.buf[0] << 8 | msg.buf[1]);
            NextionInterface::setRPM(rpm);
            RevLights::updateLights(rpm);
            break;
        case 1609:
            NextionInterface::setWaterTemp(msg.buf[0] -40);
            NextionInterface::setOilTemp(msg.buf[1] - 40);
            NextionInterface::setVoltage(msg.buf[5] * 0.1);
            break;
        case 1612:
            //coolantTempWarning, oilTempWarning, oilPressureWarning, fuelPressureWarning
            //Example from Coolant Temp: 47|1@0+ (1,0) [0|1] (47 is byte placement) (1@0+ 1 is the length, @0 is Motorola/ Big endian bit ordering, + is unsigned) (Factor, offset) physical = raw x facto + offset) [min|max] valid physical
            //Coolant Temp would then be calculated through Start bit / 8 then 7 - (startBit % 8) 
            //i.e 47 / 8 = 5 7 - (47 % 8) = 7 - 7 = 0
            //Flag is in bit 0 byte 5
            constexpr uint8_t coolantMask = 0b00000001;
            constexpr uint8_t oilTempMask = 0b00001000;
            constexpr uint8_t oilPressureMask = 0b00010000;
            constexpr uint8_t fuelPressureMask = 0b01000000;
            uint8_t warnByte = msg.buf[5];
             if(warnByte && (coolantMask || oilTempMask || oilPressureMask || fuelPressureMask)) {
                NextionInterface::switchToWarning();
            } else { // otherwise switch to driver screen
                NextionInterface::switchToDriver();
            }
            break;
        case 1613:
            NextionInterface::setGear(msg.buf[6] & 0x0F);
            break;
        case 1284:
            // WaterPump, FuelPump, Fan

            break;
        case 1604:
            // OilPressure
            
            break;
        // TODO: machine light indicator (MLI)
        case 1617:
            //lambda
            break;
        case 2047:
            // this is for warnings. if any value is greater than 0 it's big bad
            if (msg.buf != 0) {
                // TODO
            }
        default:
            break;
    }
}

void CanInterface::task(){
    Can0.events();
}