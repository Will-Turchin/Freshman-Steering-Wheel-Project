#include "main.h"
#include "neopixel.h"

#include <IntervalTimer.h>

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
}

void loop() {
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