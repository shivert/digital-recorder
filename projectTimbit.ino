/*********************************************************
  This is a library for the MPR121 12-channel Capacitive touch sensor

  Designed specifically to work with the MPR121 Breakout in the Adafruit shop
  ----> https://www.adafruit.com/products/

  These sensors use I2C communicate, at least 2 pins are required
  to interface

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
**********************************************************/

#include <Wire.h>
#include "Adafruit_MPR121.h"

// the MIDI channel number to send messages
const int channel = 1;

volatile int NbTopsFan; //measuring the rising edges of the signal
int calc;
int hallsensor = 2;    //The pin location of the sensor

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

int lastCalc = 0;

const int NOTE_MIDDLE_C = 1;
const int NOTE_D = 2;
const int NOTE_E = 3;
const int NOTE_F = 4;
const int NOTE_G = 5;
const int NOTE_A = 6;
const int NOTE_B = 7;
const int NOTE_C = 8;
const int NOTE_D_2 = 9;
const int NOTE_B_FLAT = 10;

byte noteButtons[] = {255, 254, 252, 248, 240, 224, 200, 160, 32, 216};
int noteMIDI[] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 70};
const int numberOfNotes = 10;

void rpm ()     //This is the function that the interupt calls
{
  NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
}

void playNote (uint16_t reading)     //This is the function that the interupt calls
{
  //Get the botton 8 bits from the reading
  uint8_t temp = (reading & 0xff);
  Serial.println(reading);
  Serial.println(temp);

  for (uint8_t i = 0; i < numberOfNotes; i++) {
    if (temp == noteButtons[i]) {
      usbMIDI.sendNoteOn(noteMIDI[i], 99, channel);
    }
    else {
      usbMIDI.sendNoteOff(noteMIDI[i], 99, channel);
    }
  }
  //check to see if that exists in noteButtons
}


void setup() {
  Serial.begin(9600);
  pinMode(hallsensor, INPUT_PULLUP); //initializes digital pin 2 as an input
  attachInterrupt(hallsensor, rpm, RISING); //and the interrupt is attached

  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

}

bool printState(int num, int lastNum) {
  if (num != lastNum) {
    return true;
  }
  return false;
}

void loop() {


  // Get the currently touched pads
  currtouched = cap.touched();
  NbTopsFan = 0;      //Set NbTops to 0 ready for calculations

  sei();            //Enables interrupts

  if (printState(calc, lastCalc)) {
    if (calc > 0) {
      Serial.println("Blowing");
    }
    else {
      Serial.println(" Not Blowing");
    }
  }

  if (calc > 0) {
    playNote(currtouched);
    //    for (uint8_t i = 0; i < 12; i++) {
    //      // it if *is* touched and *wasnt* touched before, alert!
    //      if ((currtouched & _BV(i))) {
    //        Serial.print(i); Serial.println(" touched");
    //        usbMIDI.sendNoteOn(60 + i, 99, channel); // 60 = C4
    //      }
    //      if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
    //        Serial.print(i); Serial.println(" released");
    //        usbMIDI.sendNoteOff(60 + i, 0, channel);  // 60 = C4
    //      }
    //
    //    }

  }
  else {
    for (uint8_t i = 0; i < numberOfNotes; i++) {
      usbMIDI.sendNoteOff(noteMIDI[i], 0, channel);
    }
  }


  //Currently active notes

  //if airflow
  //check for pressed buttons and make note
  //no airflow
  //we stop all notes


  // reset our state
  lasttouched = currtouched;
  lastCalc = calc;



  // put a delay so it isn't overwhelming
  delay(100);
  cli();            //Disable interrupts
  //  calc = (NbTopsFan * 60 / 7.5); //(Pulse frequency x 60) / 7.5Q, = flow rate in L/hour
  if (Serial.available() > 0) {
    String temp = Serial.readString();
    if (temp == "on") {
      calc = 50;
    }
    else {
      calc = 0;
    }
  }

}
