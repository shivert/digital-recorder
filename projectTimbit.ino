
#include <Wire.h>
#include "Adafruit_MPR121.h"

// the MIDI channel number to send messages
const int channel = 1;

volatile int NbTopsFan; //measuring the rising edges of the signal
int flow = 0;
int lastflow = 0;
int flowSensor = 2;    //The pin location of the sensor

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

const int numberOfNotes = 10;

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

void incrementCount ()     //This is the function that the interupt calls
{
  NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
}

void setup()
{
  Serial.begin(9600);
  pinMode(flowSensor, INPUT_PULLUP);
  attachInterrupt(flowSensor, incrementCount, RISING);

  Serial.println("Group 5 LFP Test");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A))
  {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
}

void loop()
{
  // Get the currently touched pads
  currtouched = cap.touched();
  //Set NbTops to 0 ready for flowulations
  NbTopsFan = 0;
  //Enables interrupts
  sei();

  if (flow > 0)
  {
    if (shouldPrint(flow, lastflow)) {
      Serial.println("Blowing");
    }
    playNote(currtouched);
  }
  else
  {
    if (shouldPrint(flow, lastflow)) {
      Serial.println("Not Blowing");
    }
    turnOffAllNotes();
  }

  //reset our state
  lasttouched = currtouched;
  lastflow = flow;

  // put a delay so it isn't overwhelming ??? I don't know if this is needed?
  delay(100);
  //Disable interrupts
  cli();

  //calculateFlow();
  mockFlowSensor();
}


// Functions

void calculateFlow()
{
  flow = (NbTopsFan * 60 / 7.5); //(Pulse frequency x 60) / 7.5Q, = flow rate in L/hour
}

int changeInFlow()
{
  return abs(lastFlow - flow)
}

void playNote (uint16_t reading)
{
  //Get the botton 8 bits from the reading
  uint8_t temp = (reading & 0xff);
  Serial.println(reading);
  Serial.println(temp);

  for (uint8_t i = 0; i < numberOfNotes; i++)
  {
    if (temp == noteButtons[i])
    {
      usbMIDI.sendNoteOn(noteMIDI[i], 99, channel);
    }
    else
    {
      usbMIDI.sendNoteOff(noteMIDI[i], 99, channel);
    }
  }
  //check to see if that exists in noteButtons
}

void turnOffAllNotes()
{
  for (uint8_t i = 0; i < numberOfNotes; i++)
  {
    usbMIDI.sendNoteOff(noteMIDI[i], 0, channel);
  }
}


void mockFlowSensor()
{
  if (Serial.available() > 0)
  {
    String temp = Serial.readString();
    if (temp == "on")
    {
      flow = 50;
    }
    else
    {
      flow = 0;
    }
  }
}


bool shouldPrint(int num, int lastNum)
{
  if (num != lastNum)
  {
    return true;
  }
  return false;
}

