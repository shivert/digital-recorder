#include <Adafruit_MPR121.h>
#include <Wire.h>

// the MIDI channel number to send messages
const int channel = 1;

volatile int NbTopsFan; //measuring the rising edges of the signal
int flow = 0;
int lastFlow = 0;
const int flowSensor = 2;    //The pin location of the sensor
const int buttonPin = 6;     //The pin location of joystick button
const int PIN_ANALOG_X = 23;
const int PIN_ANALOG_Y = 22;

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

const int numberOfNotes = 128;

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

const byte noteButtons[] = {255, 127, 63, 31, 15, 7, 3, 5, 4, 27};
const int noteMIDI[] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 70}; //the last value is the default recorder note
const int octaves[] = { -2, -1, 0, 1, 2};

bool notesEnabled[127] = {false};

int joystick_x_position, joystick_y_position, joystick_buttonState;
int lastButtonPosition = 0;
int currButtonPosition = 0;
int currOctave = 2;
const char* const directionMapping[] = {"normal", "down", "up", "left", "right"};

bool muted = false;

void incrementCount ()     //This is the function that the interupt calls
{
  NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
}

void setup()
{
  Serial.begin(9600);
  pinMode(flowSensor, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
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

  //usbMIDI.sendProgramChange(75, 1);
  for (int i = 0; i < 128; i++) {
    notesEnabled[i] = false;
  }

}

void readInputs()
{
  // Get the currently touched pads
  currtouched = cap.touched();

  joystick_x_position = analogRead(PIN_ANALOG_X);
  joystick_y_position = analogRead(PIN_ANALOG_Y);

  //  Serial.print("joystick_x_position");
  //  Serial.println(joystick_x_position);
  //
  //  Serial.print("joystick_y_position");
  //  Serial.println(joystick_y_position);

  joystick_buttonState = digitalRead(buttonPin);
}

void findJoystickDirection()
{
  if (joystick_x_position > 630 && 400 < joystick_y_position < 800 ) {
    currButtonPosition = 4;
  } else if (joystick_x_position < 45 && 200 < joystick_y_position < 600) {
    currButtonPosition = 3;
  } else if (joystick_y_position > 630 && 200 < joystick_x_position < 600) {
    currButtonPosition = 1;
  } else if (joystick_y_position < 45 && 300 < joystick_x_position < 800) {
    currButtonPosition = 2;
  } else {
    currButtonPosition = 0;
  }
}

int didJoystickChange()
{
  //  Serial.print("currButtonPosition");
  //  Serial.println(currButtonPosition);
  //
  //  Serial.print("lastButtonPosition");
  //  Serial.println(lastButtonPosition);

  if (currButtonPosition != lastButtonPosition) {
    lastButtonPosition = currButtonPosition;

    if (lastButtonPosition != 0) {
      return lastButtonPosition;
    }
  }
  return -1;
}

void changeOctave(int joystickPosition)
{
  if (lastButtonPosition == 2 && currOctave + 1 < (sizeof(octaves) / sizeof(octaves[0])))
  {
    updateNotesWithOctave(currOctave + 1);
    currOctave++;
  } else if (lastButtonPosition == 1 && currOctave > 0)
  {
    updateNotesWithOctave(currOctave - 1);
    currOctave--;
  }
}

void updateNotesWithOctave(int newOctave) {
  for (uint8_t i = 0; i < numberOfNotes; i++)
  {
    if (notesEnabled[i]) {
      usbMIDI.sendNoteOff(noteMIDI[i] + (12 * octaves[currOctave]), 0, channel);
      usbMIDI.sendNoteOn(noteMIDI[i] + (12 * octaves[newOctave]), 0, channel);
    }
  }
}

void killOnButtonPush() {
  if (!joystick_buttonState) {
    turnOffAllNotes();
    //muted = !muted;
  }
}

void loop()
{

  readInputs();
  killOnButtonPush();
  findJoystickDirection();

  int tempJoystickPosition = didJoystickChange();

  //  Serial.print("tempJoystickPosition");
  //  Serial.println(tempJoystickPosition);

  if (tempJoystickPosition != -1)
  {
    changeOctave(tempJoystickPosition);
  }
  //
  //  Serial.print("current octave");
  //  Serial.println(currOctave);

  //Set NbTops to 0 ready for flowulations
  NbTopsFan = 0;
  //Enables interrupts
  sei();

  if (flow > 0)
  {
    if (shouldPrint(flow, lastFlow)) {
      Serial.println("Blowing");
    }
    //if (!muted) {
      playNote(currtouched);
    //}
  }
  else
  {
    if (shouldPrint(flow, lastFlow)) {
      Serial.println("Not Blowing");
    }
    //muted = false;
    turnOffAllNotes();
  }

  //reset our state
  lasttouched = currtouched;
  lastFlow = flow;

  // put a delay so it isn't overwhelming ??? I don't know if this is needed?
  delay(100);
  //Disable interrupts
  cli();

  calculateFlow();
  //mockFlowSensor();
  //extraDebugInfoForCapSensor();
}


// Functions

void extraDebugInfoForCapSensor() {
  // debugging info, what
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
  Serial.print("Filt: ");
  for (uint8_t i = 0; i < 12; i++) {
    Serial.print(cap.filteredData(i)); Serial.print("\t");
  }
  Serial.println();
  Serial.print("Base: ");
  for (uint8_t i = 0; i < 12; i++) {
    Serial.print(cap.baselineData(i)); Serial.print("\t");
  }
  Serial.println();
}

void calculateFlow()
{
  flow = (NbTopsFan * 60 / 7.5); //(Pulse frequency x 60) / 7.5Q, = flow rate in L/hour
}

int changeInFlow()
{
  return abs(lastFlow - flow);
}

void playNote (uint16_t reading)
{
  //Get the botton 8 bits from the reading
  uint8_t temp = (reading & 0xff);
  //  Serial.println(reading);
  //  Serial.print("byte read: ");
  //  Serial.println(temp);

  for (uint8_t i = 0; i < 10; i++)
  {
    int tempNoteNumber = noteMIDI[i] + (12 * octaves[currOctave]);
//        Serial.print("note number: ");
//        Serial.println(tempNoteNumber);
//        Serial.println("conditions");
//        Serial.println(temp == noteButtons[i]);
//        Serial.println(notesEnabled[tempNoteNumber] == false);

    if (temp == noteButtons[i] && notesEnabled[tempNoteNumber] == false)
    {
      usbMIDI.sendNoteOn(tempNoteNumber, 99, channel);
      Serial.print(tempNoteNumber);
      Serial.println(" on");
      notesEnabled[tempNoteNumber] = true;
    }
    else if (temp != noteButtons[i] && notesEnabled[tempNoteNumber] == true)
    {
      Serial.print(tempNoteNumber);
      Serial.println(" off");
      usbMIDI.sendNoteOff(tempNoteNumber, 99, channel);
      notesEnabled[tempNoteNumber] = false;
    }
  }

}

void turnOffAllNotes()
{
  //  Serial.println("ALL NOTES OFF");
  for (uint8_t i = 0; i < numberOfNotes; i++)
  {
    usbMIDI.sendNoteOff(i, 0, channel);
    notesEnabled[i] = false;
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

