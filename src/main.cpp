#include "main.h"
#include "neopixel.h"

#include <IntervalTimer.h>

/* Start of V2 of code */

// Define the callback function
void shifterCallback(); 
void dialCallBack();

// Create an IntervalTimer object
IntervalTimer timer;

int const shiftUp = 37;
int const shiftDown = 36;

void setup() {
  pinMode(shiftUp, INPUT_PULLUP);
  pinMode(shiftDown, INPUT_PULLUP);
  
  delay(8000);
  Serial.begin(9600);
  Serial.println("prong1");


  Serial.println("Starting nextion interface");
  NextionInterface::init(); // Creates Serial Port to Display
  Serial.println("Nextion interface initialized.");

  CanInterface::init();

  RevLights::init();//<><><>WARNING VOLATILE<><><>

  //timer.begin(shifterCallback, 20000); // 20,000 microseconds = 20 milliseconds = 50 Hz
}

void loop() {
  CanInterface::task();
  // NextionInterface::setRPM(peen);
  // peen = (peen >= 14000) ? 8000 : peen + 500;
  //lights->rpmBased(peen);
  //Serial.println(peen);
  // delay(250);
}

void shifterCallback() { // This function will be called every 20 milliseconds (50 Hz)
  static bool shiftUpState = false;
  static bool shiftDownState = false;

  if (digitalRead(shiftUp) == 0 && shiftUpState == false)
    shiftUpState = true;
  else if (digitalRead(shiftDown) == 0 && shiftDownState == false)
    shiftDownState = true;
  else if (digitalRead(shiftDown) == 1 && shiftDownState == true)
    shiftDownState = false;
  else if (digitalRead(shiftUp) == 1 && shiftUpState == true)
    shiftUpState = false;

  CanInterface::send_shift(shiftUpState, shiftDownState);
}