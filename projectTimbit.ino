#include <Adafruit_MPR121.h>
#include <Wire.h>

const int MIDI_CHANNEL = 1;        //MIDI channel number to send messages
const int FLOW_SENSOR = 2;         //Pin location of the sensor
const int JOYSTICK_BUTTON = 6;     //Pin location of joystick button
const int PIN_ANALOG_X = 23;       //Pin location for the horizontal motion of the joystick
const int PIN_ANALOG_Y = 22;       //Pin location for the vertical motion of the joystick

volatile int NbTopsFan;            //measuring the rising edges of the signal
int flow = 0, lastFlow = 0;        //variables used to store flow information

Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched so we know when buttons are 'released'
uint16_t lastTouched = 0;         //last touched fingering combination
uint16_t currTouched = 0;         //current fingering combination

// NOTE_MIDDLE_C ---> 1
// NOTE_D ---> 2
// NOTE_E ---> 3
// NOTE_F ---> 4
// NOTE_G ---> 5
// NOTE_A ---> 6
// NOTE_B ---> 7
// NOTE_C ---> 8
// NOTE_D_2 ---> 9
// NOTE_B_FLAT ---> 10

const int numberOfNotes = 128; // Total Number of MIDI notes
const byte noteButtons[] = {255, 127, 63, 31, 15, 7, 3, 5, 4, 27}; //intenger values corresponding to button confiugration
const int noteMIDI[] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 70}; //corresponding MIDI notes
bool notesEnabled[127] = {false}; //array to keep track of which notes are on

// Joystick Direction Mapping
// 0 --> normal
// 1 --> down
// 2 --> up
// 3 --> left
// 4 --> right

int joystick_x_position, joystick_y_position, joystick_buttonState;
int lastJoystickPosition = 0;
int currJoystickPosition = 0;
int currOctave = 0;

void setup()
{
  Serial.begin(9600);

  pinMode(FLOW_SENSOR, INPUT_PULLUP);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  attachInterrupt(FLOW_SENSOR, incrementCount, RISING);

  Serial.println("Group 5 Electrocorder");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A))
  {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  // Set up default value for notesEnabled array
  for (int i = 0; i < 128; i++) {
    notesEnabled[i] = false;
  }
}

void loop()
{
  readInputs();
  killOnButtonPush();
  findJoystickDirection();

  //Set NbTops to 0 ready for flowulations
  NbTopsFan = 0;

  //Enables interrupts
  sei();

  int tempJoystickPosition = didJoystickChange();

  //  Serial.print("tempJoystickPosition");
  //  Serial.println(tempJoystickPosition);

  if (tempJoystickPosition != -1)
  {
    changeOctave(tempJoystickPosition);
  }

  if (flow > 0)
  {
    if (shouldPrint(flow, lastFlow)) {
      Serial.println("Blowing");
    }
    playNote(currTouched);
  }
  else
  {
    if (shouldPrint(flow, lastFlow)) {
      Serial.println("Not Blowing");
      turnOffAllNotes();
    }
  }

  //reset our state
  lastTouched = currTouched;
  lastFlow = flow;

  // put a delay so it isn't overwhelming ??? I don't know if this is needed?
  delay(100);

  //Disable interrupts
  cli();
  calculateFlow();
}

// Functions
void readInputs()
{
  // Get the currently touched pads
  currTouched = cap.touched();

  joystick_x_position = analogRead(PIN_ANALOG_X);
  joystick_y_position = analogRead(PIN_ANALOG_Y);

  //  Serial.print("joystick_x_position");
  //  Serial.println(joystick_x_position);
  //
  //  Serial.print("joystick_y_position");
  //  Serial.println(joystick_y_position);

  joystick_buttonState = digitalRead(JOYSTICK_BUTTON);
}

void findJoystickDirection()
{
  if (joystick_x_position > 630 && 400 < joystick_y_position < 800 ) {
    currJoystickPosition = 4;
  } else if (joystick_x_position < 45 && 200 < joystick_y_position < 600) {
    currJoystickPosition = 3;
  } else if (joystick_y_position > 630 && 200 < joystick_x_position < 600) {
    currJoystickPosition = 1;
  } else if (joystick_y_position < 45 && 300 < joystick_x_position < 800) {
    currJoystickPosition = 2;
  } else {
    currJoystickPosition = 0;
  }
}

int didJoystickChange()
{
  if (currJoystickPosition != lastJoystickPosition) {
    lastJoystickPosition = currJoystickPosition;

    if (lastJoystickPosition != 0) {
      return lastJoystickPosition;
    }
  }
  return -1;
}

void changeOctave(int joystickPosition)
{
  Serial.println("Joystick Change Detected");
  if (lastJoystickPosition == 2 && currOctave < 2)
  {
    Serial.print("Octave Up: ");
    Serial.println(currOctave + 1);
    currOctave++;
    turnOffAllNotes();
  } else if (lastJoystickPosition == 1 && currOctave > -2)
  {
    Serial.print("Octave Down: ");
    Serial.println(currOctave - 1);
    currOctave--;
    turnOffAllNotes();
  }
}

void killOnButtonPush() {
  if (!joystick_buttonState) {
    turnOffAllNotes();
  }
}

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
    int tempNoteNumber = noteMIDI[i] + (12 * currOctave);
    //    Serial.print("note number: ");
    //    Serial.println(tempNoteNumber);
    //    Serial.println("conditions");
    //    Serial.println(temp == noteButtons[i]);
    //    Serial.println(notesEnabled[tempNoteNumber] == false);

    if (temp == noteButtons[i] && notesEnabled[tempNoteNumber] == false)
    {
      usbMIDI.sendNoteOn(tempNoteNumber, 99, MIDI_CHANNEL);
      Serial.print(tempNoteNumber);
      Serial.println(" on");
      notesEnabled[tempNoteNumber] = true;
    }
    else if (temp != noteButtons[i] && notesEnabled[tempNoteNumber] == true)
    {
      Serial.print(tempNoteNumber);
      Serial.println(" off");
      usbMIDI.sendNoteOff(tempNoteNumber, 99, MIDI_CHANNEL);
      notesEnabled[tempNoteNumber] = false;
    }
  }

}

void turnOffAllNotes()
{
  for (uint8_t i = 0; i < numberOfNotes; i++)
  {
    if (notesEnabled[i])
    {
      usbMIDI.sendNoteOff(i, 0, MIDI_CHANNEL);
      notesEnabled[i] = false;
    }
  }
}

// Check if value has changed
bool shouldPrint(int num, int lastNum)
{
  if (num != lastNum)
  {
    return true;
  }
  return false;
}


void incrementCount ()     //This is the function that the interupt calls
{
  NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
}

// used only for testing purposes
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


