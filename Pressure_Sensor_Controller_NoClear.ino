/*
// ATtiny85 Pressure Sensor Controller
// Author: Chip McClelland
// Date: 7-26-18
// License - GPL3

// ATMEL ATTINY 25/45/85 / ARDUINO
//
//                           +-\/-+
//  Reset - Ain0 (D 5) PB5  1|    |8  Vcc
//  Clear - Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1 - Pressure Sensor
//  Led -   Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1 - IntPin to uC
//                     GND  4|    |5  PB0 (D 0) pwm0 - Not used
//                           +----+
// Interrupt code: https://gammon.com.au/forum/?id=11488&reply=9#reply9

/*
This is the sketch which will be put onto the embedded ATTINY 
It will manage the reading of the sensor and signalling to the Electron
Version 1.0 - Minimally functional 
Version 1.1 - Moved to interrupts for the clear pin
Version 1.2 - Cleaned it up and increased the sampling rate
Version 1.3 - Eliminated ERROR_STATE and added debounce to Clear state
Version 1.4 - Added a blink routine to setup to confirm functionality
Version 1.5 - Reduced startup time 
*/

enum State { INITIALIZATION_STATE, IDLE_STATE, SIGNALING_STATE, CLEAR_WAIT_STATE };
State state = INITIALIZATION_STATE;

// Pin assignments will for the ATTINY85
const int pressureIn =   A1;              // Analog Input
const int intPin     =   1;               // Interrupt pin will go high when an event is triggered
const int clearPin   =   3;               // Can be used to require a positive "clear" from the uC before normal operations
const int ledPin     =   4;               // Powered by the Electron - ATTINY sinks current (LOW=ON)


const int sampleRate = 3;                 // This will sample at 200 times a second
const int pressureThreshold = 100;        // Value between 0-1024 which will count as an event
const int interruptLength = 100;          // time in mSec we will wait for the Electron to respond
unsigned long lastSample = 0;

const int numReadings = 3;                // How Many numbers to average for smoothing
int pressureBuffer[numReadings];          // the readings from the pressure sensor


void setup() {
  pinMode(pressureIn,INPUT);              // Analog pressure sensor input
  pinMode(intPin,OUTPUT);                 // How we signal the Elexctron that there is an interrupt
  pinMode(ledPin,OUTPUT);                 // Initialize the LED and turn it off
  digitalWrite(ledPin,LOW);               // Ensures the LED is on
  delay(500);
  digitalWrite(ledPin,HIGH);              // Turn off the LED

  state = IDLE_STATE;
}

void loop() {
  switch (state) {
    case IDLE_STATE:
      if (millis() >= lastSample + sampleRate) {
        if (readPressure()) state = SIGNALING_STATE;
        lastSample = millis();
      }
      break;
    
    case SIGNALING_STATE:
      digitalWrite(intPin,HIGH);          // Raise the Interrupt Flag
      digitalWrite(ledPin,LOW);           // Turn on the LED
      delay(interruptLength);             // This is to debounce the signal
      digitalWrite(intPin,LOW);           // Lower the Interrupt flag
      digitalWrite(ledPin,HIGH);          // Turn off the LED
      state = CLEAR_WAIT_STATE;
      break;
    
    case CLEAR_WAIT_STATE:
      if (millis() >= lastSample + sampleRate) {
        if (!readPressure()) {
          delay(interruptLength);         // Debounce the return
          state = IDLE_STATE;             // So, the pressure is below the target and we have debounced 
        }
        lastSample = millis();
      }
      break;
  }
}

bool readPressure() {
  int runningTotal = 0;
  static int index = 0;                            // the index of the current reading

  
  //if (random(100) >= 98) return 1;                 // Simulate having a pressure sensor attached.

  pressureBuffer[index] = analogRead(pressureIn);    // Actually take the pressure
  for (int i=0; i<= numReadings; i++) runningTotal += pressureBuffer[i];
  index = (index + 1) % numReadings;
  
  if ((runningTotal/numReadings) >= pressureThreshold) return 1;  // Should trigger for a car
  else return 0;
}
