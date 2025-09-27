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

    switch (msg.id) {
        // 1600: RPM
        case 1600: {
            uint16_t rpm = ((msg.buf[0] << 8) | msg.buf[1]);
            uint16_t speed = (msg.buf[2]);
            NextionInterface::setRPM(rpm);
            // NextionInterface::setSpeed(speed);
            RevLights::updateLights(rpm);
        }
            break;
        

        // 1609: Temps & Voltage
        case 1609: {
            NextionInterface::setWaterTemp(msg.buf[0] - 40);
            NextionInterface::setOilTemp(msg.buf[1] - 40);
            NextionInterface::setVoltage(msg.buf[5] * 0.1f);
        }
            break;
        

        // 1612: Warning flags
        case 1612: {
            // Bit masks (byte 5)
            constexpr uint8_t coolantMask     = 0b10000000; // bit 0
            constexpr uint8_t oilTempMask     = 0b00010000; // bit 3
            constexpr uint8_t oilPressureMask = 0b00001000; // bit 4
            constexpr uint8_t fuelPressureMask= 0b00000001; // bit 6
            bool coolantTempWarning = msg.buf[5] & coolantMask;
            bool oilTempWarning = msg.buf[5] & oilTempMask;
            bool oilPressureWarning = msg.buf[5] & oilPressureMask;
            bool fuelPressureWarning = msg.buf[5] & fuelPressureMask;            

            if (coolantTempWarning || oilTempWarning || oilPressureWarning || fuelPressureWarning) {
                NextionInterface::switchToWarning();
            } else {
                NextionInterface::switchToDriver();
            }
        }
            break;
        

        // 1613: Gear
        case 1613: {
            NextionInterface::setGear(msg.buf[6] & 15);
        }
            break;
        

        // 1284: WaterPump, FuelPump, Fan (fill in as needed)
        case 1284: {
            if(msg.buf[0] == 0 || msg.buf[0] == 1){
                if(msg.buf[1] == 0 || msg.buf[1] == 1){
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

                    NextionInterface::setFanBool(msg.buf[3]);
                }
                else
                    Serial.println("Fan Error");
            }
        }
            break;
        
  // keep break inside the case block
        // 1604: Oil Pressure
        case 1604: {
            // OilPressure is carried in bytes 6..7; header expects two uint8_t args
            NextionInterface::setOilPressure(msg.buf[6], msg.buf[7]);
            // TODO: machine light indicator (MLI)
        }
            break;
        

        // 1617: Lambda
        case 1617: 
            NextionInterface::setLambda(msg.buf[0]);
            break;

        
        case 1608: {
            int wheelSpeedFL = ((((static_cast<uint16_t>(msg.buf[0])) | (static_cast<uint16_t>(msg.buf[1]) << 8))*0.0277777777778)*15)*0.00094697;
            int wheelSpeedFR =  ((((static_cast<uint16_t>(msg.buf[2])) | (static_cast<uint16_t>(msg.buf[3]) << 8))*0.0277777777778)*15)*0.00094697;
            int speed = (wheelSpeedFL+wheelSpeedFR)/2; // gets the average between two wheels
        

            // Serial.print(msg.buf[3]);
            // Serial.print("  ");
            // Serial.println(msg.buf[2]);
            NextionInterface::setSpeed(speed);
        }

            break;

        // 2047: “Any warnings present” message
        case 2047: 
            if(msg.buf != 0){
                
            }
        


        default: 
            break;
        
    }
}
void CanInterface::send_shift(const bool up, const bool down,const bool button3, const bool button4){
    shift_msg.id = 2032;

    shift_msg.len = 6;

    // if(up){
    //     shift_msg.buf[0] = 111;
    //     shift_msg.buf[1] = 127;
    // } else {
    //     shift_msg.buf[0] = 0;
    //     shift_msg.buf[1] = 0;
    // }
    
    // if(down){
    //     shift_msg.buf[2] = 95;
    //     shift_msg.buf[3] = 127;
    // } else {
    //     shift_msg.buf[2] = 0;
    //     shift_msg.buf[3] = 0;
    // }

    if(button3){
        shift_msg.buf[4] = 95;
        shift_msg.buf[5] = 127;
    }else{
        shift_msg.buf[4] = 0;
        shift_msg.buf[5] = 0;
    }

    if(button4){
        shift_msg.buf[0] = 95;
        shift_msg.buf[1] = 127;
    }  else{
        shift_msg.buf[0] = 0;
        shift_msg.buf[1] = 0;
    }


    
    Can0.write(shift_msg);
}

void CanInterface::task(){
    Can0.events();
}