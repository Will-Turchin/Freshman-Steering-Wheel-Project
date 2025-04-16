#include "Arduino.h"

#include "nextion.h"
#include "neopixel.h"

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanInterface::Can0;

CanInterface::CanInterface(){


    // uint16_t volt5 = 5000;
    // uint8_t volt5L = volt5 & 0x00FF;
    // uint8_t volt5H = volt5 >> 8;
}

CAN_message_t CanInterface::shift_msg;
bool CanInterface::canActive = false;
//static RevLights* lights;

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
    // Can0.mailboxStatus();
    //lights = new RevLights();
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
    page currentPage = NextionInterface::getCurrentPage();
    canActive = true;


    if(currentPage != page::DRIVER){
        NextionInterface::switchToDriver();
    }


    switch (msg.id){
        case 1600:
        //   Serial.println(msg.id);
            NextionInterface::setRPM((msg.buf[1] | (msg.buf[0] << 8)) );
            // NextionInterface::setWaterTemp(msg.buf[3]);


            RevLights::updateLights((msg.buf[1] | (msg.buf[0] << 8)));
            break;
        

        case 1614:
            // TODO: CHECK LATER
            NextionInterface::setNeutral((msg.buf[4] & 16 ) << 0x10);
            break;

        case 0x649:
            //VERIFIED VOLTAGE FUNCTION
            NextionInterface::setWaterTemp(msg.buf[0]  +40);
            NextionInterface::setOilTemp(msg.buf[1] + 40);
            NextionInterface::setVoltage(msg.buf[5] * 0.1);
            break;

        // TODO: this doesnt appear on the ECUsensors file, ignoring for now
        case 1284:
            if(msg.buf[0] == 0 || msg.buf[0] == 1){
                if(msg.buf[1] == 0 || msg.buf[1] == 1){
                    NextionInterface::setWaterPumpValue(msg.buf[1]);
                    NextionInterface::setWaterPumpBool(msg.buf[1]);
                } 
                else
                    Serial.println("Water Pump Error");

                if(msg.buf[0] == 0 || msg.buf[0] == 1){
                    //NextionInterface::setFuelPumpValue(msg.buf[0]);
                    NextionInterface::setFuelPumpBool(msg.buf[0]);
                }   
                else
                    Serial.println("Fuel Pump Error");

                if(msg.buf[3] == 0 || msg.buf[3] == 1){
                    NextionInterface::setFanValue(msg.buf[3]);
                    NextionInterface::setFanBool(msg.buf[3]);
                }
                else
                    Serial.println("Fan Error");
            }
            break;

        case 1604:
            // The math here is PROBABLY right Im not going to check it -Dawson
            NextionInterface::setOilPressure(msg.buf[6], msg.buf[7]);
            break;

        case 1623:
            // Once again I assume this works even though it likely does not. -Dawson
            NextionInterface::setGear(msg);
            break;

        // TODO: machine light indicator (MLI)

        // lambda bool
        case 1617:
            NextionInterface::setLambda(msg.buf[0] * 100);
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