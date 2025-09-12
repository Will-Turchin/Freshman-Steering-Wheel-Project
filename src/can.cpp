#include "Arduino.h"

#include "nextion.h"
#include "neopixel.h"

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanInterface::Can0;

CanInterface::CanInterface(){
    // deprecated function
}

CAN_message_t CanInterface::shift_msg;
bool CanInterface::canActive = false;

bool CanInterface::init(){
    // pinMode(6, OUTPUT); digitalWrite(6, LOW); /* optional tranceiver enable pin */
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

void CanInterface::print_can_sniff(const CAN_message_t &msg){
    Serial.print("MB "); Serial.print(msg.mb);
    Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
    Serial.print("  LEN: "); Serial.print(msg.len);
    Serial.print(" EXT: "); Serial.print(msg.flags.extended);
    Serial.print(" TS: "); Serial.print(msg.timestamp);
    Serial.print(" ID: "); Serial.print(msg.id, DEC);
    Serial.print(" Buffer: ");
    for ( uint8_t i = 0; i < msg.len; i++ ) {
        Serial.print(msg.buf[i], DEC); Serial.print(" ");
    } 
    Serial.println();
}

void CanInterface::receive_can_updates(const CAN_message_t &msg){
    //page currentPage = NextionInterface::getCurrentPage();
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
            break;
        case 0x649:
            NextionInterface::setWaterTemp(msg.buf[0] -40);
            NextionInterface::setOilTemp(msg.buf[1] - 40);
            NextionInterface::setVoltage(msg.buf[5] * 0.1);
            break;
        case 0x64C:
            //coolantTempWarning, oilTempWarning, oilPressureWarning, fuelPressureWarning

            /*if(any of the warnings) {
                NextionInterface::switchToWarning();
            } else { // otherwise switch to driver screen
                NextionInterface::switchToDriver();
            }*/
            break;
        case 0x64D:
            NextionInterface::setGear(msg.buf[6] & 0x0F);
            break;
        case 1284:
            // WaterPump, FuelPump, Fan
            break;
        case 1604:
            // OilPressur
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


void CanInterface::send_shift(const bool up, const bool down,const bool button3){
    //DO NOT TOUCH THIS CODE
    shift_msg.id = 0x07F0;

    shift_msg.len = 6;

    if(up){
        shift_msg.buf[0] = 0x6F;
        shift_msg.buf[1] = 0x7F;
    } else {
        shift_msg.buf[0] = 0;
        shift_msg.buf[1] = 0;
    }
    
    if(down){
        shift_msg.buf[2] = 0x5F;
        shift_msg.buf[3] = 0x7F;
    } else {
        shift_msg.buf[2] = 0;
        shift_msg.buf[3] = 0;
    }

    if(button3){
        shift_msg.buf[4] = 0x5F;
        shift_msg.buf[5] = 0x7F;
    }else{
        shift_msg.buf[4] = 0;
        shift_msg.buf[5] = 0;
    }

    Can0.write(shift_msg);
}

void CanInterface::task(){
    Can0.events();
}