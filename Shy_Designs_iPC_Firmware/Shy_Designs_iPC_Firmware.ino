/*
*   Created By: Andrew O'Shei
*   Date: March 11, 2021
*   Teensy 3.6 Firmware for the Shy Designs iPC
*
*   IMPORTANT:  This code is meant for a proof of concept prototype and not for distribution
*               The creator reserves all rights for the code unique to this project (this does not including any third party libraries used)
*
*   Attention Teensy 3.6 Overheats at 180Mhz with this code. Reduce processor clock speed for more stable performance.
*
*/

#include <SPI.h>
#include <Keypad.h>
#include <Encoder.h>
#include <ADC.h>
#include <ButtonDebounce.h>
#include <MIDI.h>


//PIN ASSIGNMENTS

const byte ROWSA = 8; //rows
const byte COLS = 8; //columns
char keysA[ROWSA][COLS] = {
    {  50,  51,  52,  53,  54,  55,  56,  57 },
    {   4,   2,   1,   0,  29,  27,  26,  59 },
    {   9,   7,   6,   5,  34,  32,  31,  25 },
    {  14,  12,  11,  10,  39,  37,  36,  30 },
    {  19,  17,  16,  15,  44,  42,  41,  35 },
    {  24,  22,  21,  20,  49,  47,  46,  40 },
    { 255,  55, 126, 129, 132, 135, 138,  45 },
    { 255, 124, 127, 130, 133, 136, 139, 240 }
};

byte rowAPins[ROWSA] = {12, 13, 14, 15, 24, 25, 39, 38}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {47, 48, 2, 3, 4, 5, 6, 7}; //connect to the column pinouts of the kpd

Keypad KeyWing1 = Keypad(makeKeymap(keysA), rowAPins, colPins, ROWSA, COLS );

const byte ROWSB = 8; //rows
char keysB[ROWSB][COLS] = {
    {  255, 25, 28, 31, 34, 37, 40, 241 },
    {    0,  1,  4,  7, 11, 45, 41, 242 },
    {   10,  2,  5,  8, 12, 46, 42, 243 },
    {   18,  3,  6,  9, 13, 47, 43, 244 },
    {  255, 17, 16, 15, 14, 48, 44, 245 },
    {  255, 19, 58, 55, 52, 49, 61,  68 },
    {   21, 22, 59, 56, 53, 50, 62,  69 },
    {  255, 20, 60, 57, 54, 51, 63,  70 }
};

byte rowBPins[ROWSB] = {37, 36, 35, 34, 11, 10, 9, 8};

Keypad KeyWing2 = Keypad(makeKeymap(keysB), rowBPins, colPins, ROWSB, COLS);

// Capture Fader with choose
bool chooseFader = false;
bool moveFader = false;
int stateFader = 0;

int moveMin = 120;
int moveMax = 240;

char faderVals[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//HARDWARE MIDI Setup ------
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

//Desk Lamp PWM-----
const int dLamp = 20;

// Encoder Setup----------
Encoder EnR(19, 18);
Encoder En1(17, 16);
Encoder En2(27, 26);
Encoder En3(29, 28);
Encoder En4(31, 30);
Encoder En5(33, 32);

long old_EnR = -999;
long old_En1 = -999;
long old_En2 = -999;
long old_En3 = -999;
long old_En4 = -999;
long old_En5 = -999;

int EnR_VAL = 0;
int En1_VAL = 0;
int En2_VAL = 0;
int En3_VAL = 0;
int En4_VAL = 0;
int En5_VAL = 0;

//---------------------------

//Rate Buttons-------------
const int redBut = 49;
const int grnBut = 50;

ButtonDebounce redButton(redBut, 100);
ButtonDebounce grnButton(grnBut, 100);

//Setup Fader--------------
#define FADE_PIN A10
ADC *adc = new ADC();
int oldFade = 0;
int curFade;
int scaFade;
int tarFade = 127;

unsigned long loopCount;
unsigned long startTime;
String msg;

const int fchan = 1; // Sets MIDI Output Channel
const int pchan = 2;
const int echan = 3;
const int schan = 4;

//Setup Fader Touch
#define TOUCH_PIN A9
int touchThresh = 7500;
int touchState = 0;
unsigned long touchVal;

//Setup H-Bridge
#define MOTOR_A  A7
#define MOTOR_B  A8
//----------------------------

//LED Setup
int SPI_CS = 43; // using digital pin 10 for SPI slave select
int TLC_OE = 42; // output enable

int TLC1 = 0;
int TLC2 = 0;
int TLC3 = 0;
int TLC4 = 0;
int TLC5 = 0;
int TLC6 = 0;
int TLC7 = 0;

byte intX;

char ledStateMap[56];
short ledMap[56] = {  0x0204, 0x0208, 0x0110, 0x0108, 0x0120, 0x0540, 0x0508, 0x0502, 0x0610, 0x0640, // Choose 1 - 10
                      0x0302, 0x0308, 0x0340, 0x0310, 0x0202, 0x0140, 0x0180, 0x0210, 0x0240, 0x0510, // Flash Green 1 - 10
                      0x0301, 0x0304, 0x0380, 0x0320, 0x0201, 0x0104, 0x0102, 0x0101, 0x0220, 0x0280, // Flash Red 1 - 10
                      0x0608, 0x0602,                         // Rate, Restore
                      0x0410,                                 // Macro
                      0x0710, 0x0601, 0x0604, 0x0680,         // Intensity, Position, Colour, Beam
                      0x0708, 0x0704, 0x0701, 0x0702,         // Effect, Time, Group, Fixture
                      0x0720, 0x0740,                         // Highlight, Blind
                      0x0780,                                 // Clear
                      0x0620, 0x0501, 0x0504, 0x0580, 0x0520, 0x0401, 0x0402, 0x0404, 0x0408, 0x0480, 0x0440, 0x0420 // F Keys
};

bool is_on = false;

void redPress(int state) {
  if (state) {
    usbMIDI.sendNoteOff(61, 0, fchan);
  }
  else {
    usbMIDI.sendNoteOn(61, 127, fchan);
  }
}

void grnPress(int state) {
  if (state) {
    usbMIDI.sendNoteOff(62, 0, fchan);
  }
  else {
    usbMIDI.sendNoteOn(62, 127, fchan);
  }
}

void setup() {
  Serial.begin(115200);
  loopCount = 0;
  startTime = millis();
  msg = "";

  MIDI.begin();

  //Setup ADC (Fader)
  pinMode(FADE_PIN, INPUT);
  adc->setResolution(8); // set bits of ADC resolution
  pinMode(TOUCH_PIN, INPUT_PULLUP);

  pinMode(MOTOR_A, OUTPUT); // H-Bridge Control
  pinMode(MOTOR_B, OUTPUT);

  pinMode(dLamp, OUTPUT); //Desk Lamp PWM
  analogWrite(dLamp, 0);

  pinMode(redBut, INPUT_PULLUP);
  pinMode(grnBut, INPUT_PULLUP);

  redButton.setCallback(redPress);
  grnButton.setCallback(grnPress);

  delay(500);
  
  pinMode(SPI_CS, OUTPUT); // this for SPI_CS pin
  pinMode(TLC_OE, OUTPUT);
  SPI2.setMOSI(44);
  SPI2.setMISO(45);
  SPI2.setSCK(46);
  SPI2.begin(); // wake up the SPI bus.
  SPI2.setBitOrder(LSBFIRST);
  setLED();

  // Initialize LED State Map
  for(int pos = 0; pos < 56; pos++) {
    ledStateMap[pos] = 0;
  }

  delay(500);
}


void loop() {

  redButton.update();
  grnButton.update();
  
  long pos_EnR = EnR.read();
  long pos_En1 = En1.read();
  long pos_En2 = En2.read();
  long pos_En3 = En3.read();
  long pos_En4 = En4.read();
  long pos_En5 = En5.read();
  
  loopCount++;
  if ( (millis() - startTime) > 5000 ) {
    Serial.print("Average loops per second = ");
    Serial.println(loopCount / 5);
    startTime = millis();
    loopCount = 0;
  }



//UART SERIAL SYNC-------------------------
  String message;
  if(Serial.available() > 0){
    message = Serial.readStringUntil('\n');
      if(message[1] == '1'){
        pushLEDs(message[2], message[3], message[5]);
      }
      else if(message[1] == '2') {
        pushFader(message[3], message[4], message[5]);
      }
  }



//FADER CONTROL-------------------------

  touchVal = touchRead(TOUCH_PIN);
  //int t = touchVal;
  //Serial.println(t);
  if(touchState == 0) {
    if (touchVal > touchThresh) {
      Serial.println("The fader is touched");
      touchState = 1;
    }
  }

  if(touchState == 1) {
    if (touchVal < touchThresh) {
      Serial.println("The fader is lonely");
      touchState = 0;
    }
  }
  

  curFade = adc->analogRead(FADE_PIN);
  //Serial.println(curFade);
  if(touchState == 1) {
    if ( curFade > oldFade + 1 || curFade < oldFade - 1 ) {
      // Serial.println(curFade >> 1);
      usbMIDI.sendControlChange(stateFader, curFade >> 1, echan);
      oldFade = curFade;
      faderVals[stateFader] = curFade;
    }
  }

  if(touchState == 0) {
    if(curFade > faderVals[stateFader] + 3 || curFade < faderVals[stateFader] - 3){
      int distance = curFade - faderVals[stateFader];
      if(distance < 0){
        // Move up
        distance = abs(distance);
        int mov = map(distance, 0, 255, moveMin, moveMax);
        analogWrite(MOTOR_A, 0);
        analogWrite(MOTOR_B, mov);
      } else {
        //Move Down
        int mov = map(distance, 0, 255, moveMin, moveMax);
        analogWrite(MOTOR_A, mov);
        analogWrite(MOTOR_B, 0);
      }
      delay(5);
      analogWrite(MOTOR_A, 0);
      analogWrite(MOTOR_B, 0);
    }
  }
  

  //Serial.println(curFade);
//END FADER CONTROL---------------------


//KEYPAD-------------------------------------------------------
    // Fills kpd.key[ ] array with up-to 10 active keys.
    // Returns true if there are ANY active keys.
    if (KeyWing1.getKeys()) {
        for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {
            if ( KeyWing1.key[i].stateChanged )   // Only find keys that have changed state.
            {
                switch (KeyWing1.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                    case PRESSED:
                    msg = " PRESSED.";
                    if (KeyWing1.key[i].kchar != 255) {
                      if (KeyWing1.key[i].kchar > 200) {
                          usbMIDI.sendNoteOn( KeyWing1.key[i].kchar - 200, 127, schan);
                      }
                      if (KeyWing1.key[i].kchar > 100 and KeyWing1.key[i].kchar < 200){
                          usbMIDI.sendNoteOn(KeyWing1.key[i].kchar - 100, 127, pchan);
                      
                      }
                      if (KeyWing1.key[i].kchar < 100) {
                          usbMIDI.sendNoteOn(KeyWing1.key[i].kchar, 127, fchan);
                      }
                      Serial.println((int)KeyWing1.key[i].kchar);
                    }
                break;
                
                    case HOLD:
                    msg = " HOLD.";
                    if(KeyWing1.key[i].kchar < 46) {
                      if(!KeyWing1.key[i].kchar || KeyWing1.key[i].kchar % 5 == 0){
                        usbMIDI.sendNoteOff(KeyWing1.key[i].kchar, 0, fchan);
                        stateFader = KeyWing1.key[i].kchar / 5 + 1;
                        chooseFader = true;
                      }
                    }
                break;
                
                    case RELEASED:
                    msg = " RELEASED.";
                    if (KeyWing1.key[i].kchar != 255) {
                      if (KeyWing1.key[i].kchar > 200) {
                          usbMIDI.sendNoteOff( KeyWing1.key[i].kchar, 0, schan);
                        }
                      if (KeyWing1.key[i].kchar > 100 and KeyWing1.key[i].kchar < 200){
                            usbMIDI.sendNoteOff( KeyWing1.key[i].kchar - 100, 0, pchan);
                        }
                      
                      if (KeyWing1.key[i].kchar < 100) {
                          if(chooseFader and KeyWing1.key[i].kchar == ((stateFader - 1) * 5)) {
                            stateFader = 0;
                            chooseFader = false;
                          } else {
                            usbMIDI.sendNoteOff(KeyWing1.key[i].kchar, 0, fchan);
                          }
                      }
                    }
                break;
                
                    case IDLE:
                    msg = " IDLE.";
                }
                Serial.print("Key ");
                Serial.print(KeyWing1.key[i].kchar);
                Serial.println(msg);
            }
        }
    }

    if (KeyWing2.getKeys()) {
        for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {
            if ( KeyWing2.key[i].stateChanged )   // Only find keys that have changed state.
            {
                switch (KeyWing2.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                    case PRESSED:
                    msg = " PRESSED.";
                    if (KeyWing2.key[i].kchar != 255) {
                      if (KeyWing2.key[i].kchar > 200) {
                          usbMIDI.sendNoteOn( KeyWing2.key[i].kchar - 200, 127, schan);
                      }
                      else {
                          usbMIDI.sendNoteOn(KeyWing2.key[i].kchar, 127, pchan);
                      }
                    }
                break;
                
                    case HOLD:
                    msg = " HOLD.";

                break;
                
                    case RELEASED:
                    msg = " RELEASED.";
                    if (KeyWing2.key[i].kchar != 255) {
                      if (KeyWing2.key[i].kchar > 200) {
                          usbMIDI.sendNoteOff( KeyWing2.key[i].kchar, 0, schan);
                        }
                      else {
                          usbMIDI.sendNoteOff(KeyWing2.key[i].kchar, 0, pchan);                                              
                        }
                    }
                    
                break;
                    case IDLE:
                    msg = " IDLE.";
                }
                Serial.print("Key ");
                Serial.print(KeyWing2.key[i].kchar);
                Serial.println(msg);
            }
        }
    }

//END KEYPAD----------------------------------------------------------------
    
//ENCODERS-------------------------------------------
  //Rate Encoder
  if (pos_EnR != old_EnR) {
    if (pos_EnR > old_EnR) {
      Serial.println("Rate Up");
      old_EnR = pos_EnR;
      EnR_VAL += 1;
      if(EnR_VAL >= 128) {
        EnR_VAL = 0;
      }
      usbMIDI.sendControlChange(16, EnR_VAL, echan);
    }
    if (pos_EnR < old_EnR) {
      Serial.println("Down Rate");
      old_EnR = pos_EnR;
      EnR_VAL -= 1;
      if(EnR_VAL <= -1) {
        EnR_VAL = 127;
      }
      usbMIDI.sendControlChange(16, EnR_VAL, echan);
    }
  }

  //Encoder 1
  if (pos_En1 != old_En1) {
    if (pos_En1 > old_En1) {
      Serial.println("Enc1 Up");
      old_En1 = pos_En1;
      En1_VAL += 1;
      if(En1_VAL >= 128) {
        En1_VAL = 0;
      }
      usbMIDI.sendControlChange(11, En1_VAL, echan);
    }
    if (pos_En1 < old_En1) {
      Serial.println("Enc1 Down");
      old_En1 = pos_En1;
      En1_VAL -= 1;
      if(En1_VAL <= -1) {
        En1_VAL = 127;
      }
      usbMIDI.sendControlChange(11, En1_VAL, echan);
    }
  }

  //Encoder 2
  if (pos_En2 != old_En2) {
    if (pos_En2 > old_En2) {
      Serial.println("Enc2 Up");
      old_En2 = pos_En2;
      En2_VAL += 1;
      if(En2_VAL >= 128) {
        En2_VAL = 0;
      }
      usbMIDI.sendControlChange(12, En2_VAL, echan);
    }
    if (pos_En2 < old_En2) {
      Serial.println("Enc2 Down");
      old_En2 = pos_En2;
      En2_VAL -= 1;
      if(En2_VAL <= -1) {
        En2_VAL = 127;
      }
      usbMIDI.sendControlChange(12, En2_VAL, echan);
    }
  }

  //Encoder 3
  if (pos_En3 != old_En3) {
    if (pos_En3 > old_En3) {
      Serial.println("Enc3 Up");
      old_En3 = pos_En3;
      En3_VAL += 1;
      if(En3_VAL >= 128) {
        En3_VAL = 0;
      }
      usbMIDI.sendControlChange(13, En3_VAL, echan);
    }
    if (pos_En3 < old_En3) {
      Serial.println("Enc3 Down");
      old_En3 = pos_En3;
      En3_VAL -= 1;
      if(En3_VAL <= -1) {
        En3_VAL = 127;
      }
      usbMIDI.sendControlChange(13, En3_VAL, echan);
    }
  }

  //Encoder 4
  if (pos_En4 != old_En4) {
    if (pos_En4 > old_En4) {
      Serial.println("Enc4 Up");
      old_En4 = pos_En4;
      En4_VAL += 1;
      if(En4_VAL >= 128) {
        En4_VAL = 0;
      }
      usbMIDI.sendControlChange(14, En4_VAL, echan);
    }
    if (pos_En4 < old_En4) {
      Serial.println("Enc4 Down");
      old_En4 = pos_En4;
      En4_VAL -= 1;
      if(En4_VAL <= -1) {
        En4_VAL = 127;
      }
      usbMIDI.sendControlChange(14, En4_VAL, echan);
    }
  }

  //Encoder 5
  if (pos_En5 != old_En5) {
    if (pos_En5 > old_En5) {
      Serial.println("Enc5 Up");
      old_En5 = pos_En5;
      En5_VAL += 1;
      if(En5_VAL >= 128) {
        En5_VAL = 0;
      }
      usbMIDI.sendControlChange(15, En5_VAL, echan);
    }
    if (pos_En5 < old_En5) {
      Serial.println("Enc5 Down");
      old_En5 = pos_En5;
      En5_VAL -= 1;
      if(En5_VAL <= -1) {
        En5_VAL = 127;
      }
      usbMIDI.sendControlChange(15, En5_VAL, echan);
    }
  }
//END ENCODERS--------------------------------------------

//HARDWARE MIDI-----------------------------------------
/*NOTE: This is only a test function to receive midi noteOn and noteOff values
        I need to build a seperate void function for handling full midi protocol*/
  int note, velocity, channel, d1, d2;
  if (MIDI.read()) {                    // Is there a MIDI message incoming ?
    byte type = MIDI.getType();
    switch (type) {
      case midi::NoteOn:
        note = MIDI.getData1();
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        if (velocity > 0) {
          Serial.println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
        } else {
          Serial.println(String("Note Off: ch=") + channel + ", note=" + note);
        }
        break;
      case midi::NoteOff:
        note = MIDI.getData1();
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        Serial.println(String("Note Off: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
        break;
      default:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2();
        Serial.println(String("Message, type=") + type + ", data = " + d1 + " " + d2);
    }
  }
//END HARDWARE MIDI-------------------------------------
}

// Set fader states and values
void pushFader(char pos, char valHi, char valLo) {
  int fader = pos - 48;
  faderVals[fader] = processHEX(valHi, valLo);
}

// Get hex value sent as char
char processHEX(char hi, char lo) {
  int x = hi - 48;
  int y = lo - 48;
  if(x > 9) {
    x -= 7;
  }
  if(y > 9) {
    y -= 7;
  }
  return ((x * 16) + y);
}

void setLED() {
  digitalWrite(TLC_OE, LOW);
  digitalWrite(SPI_CS, LOW);
  SPI2.transfer(0);     // send command byte
  SPI2.transfer(TLC7);  // TLC 7
  SPI2.transfer(TLC6);  // TLC 6
  SPI2.transfer(TLC5);  // TLC 5
  SPI2.transfer(TLC4);  // TLC 4
  SPI2.transfer(TLC3);  // TLC 3
  SPI2.transfer(TLC2);  // TLC 2
  SPI2.transfer(TLC1);  // TLC 1
  digitalWrite(SPI_CS, HIGH);
  digitalWrite(TLC_OE, HIGH);
  digitalWrite(SPI_CS, LOW);
  digitalWrite(TLC_OE, LOW);
}


void RESET() {
  TLC1 = 0;
  TLC2 = 0;
  TLC3 = 0;
  TLC4 = 0;
  TLC5 = 0;
  TLC6 = 0;
  TLC7 = 0;
  setLED();
}


void pushLEDs(char pos1, char pos2, char onoff){
  int posit = ((pos1 - 48) * 10) + pos2 - 48;
  int onState = onoff - 48;
  if(ledStateMap[posit] != onState) {
    Serial.println("State Changed");
    ledStateMap[posit] = onState;
    if (onState) {
      ledON(ledMap[posit]);
    } else {
      ledOFF(ledMap[posit]);
    }
  }
}

void ledON(short ledVal) {
  int target = ledVal >> 8;
  switch(target) {
    case 1:
      TLC1 = TLC1 + (ledVal - 0x0100);
      break;
    case 2:
      TLC2 = TLC2 + (ledVal - 0x0200);
      break;
    case 3:
      TLC3 = TLC3 + (ledVal - 0x0300);
      break;
    case 4:
      TLC4 = TLC4 + (ledVal - 0x0400);
      break;
    case 5:
      TLC5 = TLC5 + (ledVal - 0x0500);
      break;
    case 6:
      TLC6 = TLC6 + (ledVal - 0x0600);
      break;
    case 7:
      TLC7 = TLC7 + (ledVal - 0x0700);
      break;
    default:
      break;
  }
  setLED();
}


void ledOFF(short ledVal) {
  int target = ledVal >> 8;
  switch(target) {
    case 1:
      TLC1 = TLC1 - (ledVal - 0x0100);
      break;
    case 2:
      TLC2 = TLC2 - (ledVal - 0x0200);
      break;
    case 3:
      TLC3 = TLC3 - (ledVal - 0x0300);
      break;
    case 4:
      TLC4 = TLC4 - (ledVal - 0x0400);
      break;
    case 5:
      TLC5 = TLC5 - (ledVal - 0x0500);
      break;
    case 6:
      TLC6 = TLC6 - (ledVal - 0x0600);
      break;
    case 7:
      TLC7 = TLC7 - (ledVal - 0x0700);
      break;
    default:
      break;
  }
  setLED();
}
