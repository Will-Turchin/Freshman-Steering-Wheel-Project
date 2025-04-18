#include "main.h"
#include "neopixel.h"

#include <IntervalTimer.h>

/* Start of V2 of code */

// Define the callback function
void shifterCallback(); 
void buttonsCallback();

// Create an IntervalTimer object
IntervalTimer timer;

int const shiftUp = 43;
int const shiftDown = 42;
int const button3 = 44;
int const button4 = 45;
int const button5 = 6;
int const button6 = 9;

void setup() {
  pinMode(shiftUp, INPUT_PULLUP);
  pinMode(shiftDown, INPUT_PULLUP);
  pinMode(button3,INPUT_PULLUP);
  pinMode(button4,INPUT_PULLUP);
  pinMode(button5,INPUT_PULLUP);
  pinMode(button6,INPUT_PULLUP);
  
  delay(8000);
  Serial.begin(9600);


  Serial.println("Starting nextion interface");
  NextionInterface::init(); // Creates Serial Port to Display
  Serial.println("Nextion interface initialized.");

  CanInterface::init();

  RevLights::init();
  NextionInterface::switchToDriver();

  timer.begin(shifterCallback, 20000); // 20,000 microseconds = 20 milliseconds = 50 Hz
  // timer.begin(buttonsCallback,50000);
}

void loop() {
  CanInterface::task();
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

void buttonsCallback(){
  // if(digitalRead(button3)){
  //   if (NextionInterface::getCurrentPage() == YIPPEE){
  //       NextionInterface::switchToDriver();
  //   }else if (NextionInterface::getCurrentPage() == DRIVER){
  //       NextionInterface::switchToYippee();
  //   }
  // }
}