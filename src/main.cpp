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

int dial1, dial2, dial3;
void dialCallBack(){
  dial1 = analogRead(A10);
  dial1 = dial1*3.0/1023.0;
  //Serial.printf("A10 %d\n", dial1); //this one is good
  //Serial.printf("A11 %d\n", digitalRead(A11));
  dial2 = analogRead(A11);
  dial2 = dial2%450;
  //Serial.printf("A12 %d\n", digitalRead(A12));
  dial3 = digitalRead(A12);
}

void setup() {
  pinMode(shiftUp, INPUT_PULLUP);
  pinMode(shiftDown, INPUT_PULLUP);

  RevLights lights{};

  delay(8000);
  Serial.begin(9600);

  NextionInterface::init(); // Creates Serial Port to Display

  CanInterface::init();

  //timer.begin(shifterCallback, 20000); // 20,000 microseconds = 20 milliseconds = 50 Hz
}

void loop() {
  CanInterface::task();
  dialCallBack();
  if(dial1 == 1 && NextionInterface::getCurrentPage() != DRIVER){
    NextionInterface::switchToDriver();
  } else if(dial1 == 2 && NextionInterface::getCurrentPage() != WARNING) {
    NextionInterface::switchToWarning();
  } else if(dial1 == 3 && NextionInterface::getCurrentPage() != LOADING){
    NextionInterface::switchToLoading();
  }

  NextionInterface::setWaterTemp(dial2);
  delay(100);
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