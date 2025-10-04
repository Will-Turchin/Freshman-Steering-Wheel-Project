#include "main.h"
#include "neopixel.h"
#include <IntervalTimer.h>


// Define the callback function
void buttonsCallback();
 void shifterCallback();
// Create an IntervalTimer object
IntervalTimer timer;


int const shiftUp = 43;
int const shiftDown = 42;
int const button3 = 44;
int const button4 = 45;
int const button5 = 6;
int const button6 = 9;

void setup() {
  //Inits buttons
  pinMode(shiftUp, INPUT_PULLUP);
  pinMode(shiftDown, INPUT_PULLUP);
  pinMode(button3,INPUT_PULLUP);
  pinMode(button4,INPUT_PULLUP);
  pinMode(button5,INPUT_PULLUP);
  pinMode(button6,INPUT_PULLUP);
  
  Serial.begin(9600);
  //Starts Nextion
  Serial.println("Starting nextion interface");
  NextionInterface::init(); // Creates Serial Port to Display
  Serial.println("Nextion interface initialized.");

  NextionInterface::switchToDriver();

  while(NextionInterface::getCurrentPage() != page::DIAGNOSTICS){
    
  }
  //Start Can
  CanInterface::init();
  //Starts Rev Lights
  RevLights::begin(75, true, 9600);
  //Switches to the driver screen

  timer.begin(shifterCallback, 20000); // 20,000 microseconds = 20 milliseconds = 50 Hz
}

void loop() {
  //Updates Can
  CanInterface::task();


}

void buttonsCallback() {
  // deprecated function, use this to test button input on steering wheel

  // if(digitalRead(button3)){
  //   if (NextionInterface::getCurrentPage() == YIPPEE){
  //       NextionInterface::switchToDriver();
  //   }else if (NextionInterface::getCurrentPage() == DRIVER){
  //       NextionInterface::switchToYippee();
  //   }
  // }
}
void shifterCallback() { // This function will be called every 20 milliseconds (50 Hz)
  static bool shiftUpState = false;
  static bool shiftDownState = false;
  static bool button3State = false;

  if (digitalRead(shiftUp) == 1 && shiftUpState == false){
    // Serial.println("UP");
    shiftUpState = true;
  }
  else if (digitalRead(shiftDown) == 1 && shiftDownState == false){
    // Serial.println("DOWN");
    shiftDownState = true;
  }
  else if (digitalRead(shiftDown) == 0 && shiftDownState == true){
    shiftDownState = false;
  }
  else if (digitalRead(shiftUp) == 0 && shiftUpState == true){
    shiftUpState = false;
  }
  else if(digitalRead(button3) ==1 && button3State == false){
    button3State = true;
  }
  else if (digitalRead(button3) == 0 && button3State == true){
    button3State = false;
  }

  CanInterface::send_shift(shiftUpState, shiftDownState,button3State);
}