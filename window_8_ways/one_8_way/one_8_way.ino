void setup();

//#include <TimerOne.h>
#include <CapacitiveSensor.h>
#include "CapacitiveTouchSwitch.h"

//#define USE_SERIAL


long threshold=60;


const uint8_t numSensors=8;

CapacitiveTouchSwitch sns0(0,threshold);
CapacitiveTouchSwitch sns1(1,threshold);
CapacitiveTouchSwitch sns2(23,threshold);
CapacitiveTouchSwitch sns3(22,threshold);
CapacitiveTouchSwitch sns4(19,threshold);
CapacitiveTouchSwitch sns5(18,threshold);
CapacitiveTouchSwitch sns6(17,threshold);
CapacitiveTouchSwitch sns7(16,threshold);
CapacitiveTouchSwitch sns8(15,threshold);

struct KeyboardTouch {
  CapacitiveTouchSwitch *key;
  uint16_t keys[2];
};

const int lastTouchPins[]={9,10,11,12};
const int handshakeOut=8; //master handshake
const int handshakeIn=7; //slave handshake

struct KeyboardTouch keys[numSensors]= {
  {&sns6,  {KEY_I,     0}}, //up
  {&sns5,  {KEY_I, KEY_J}}, //up, left
  {&sns4,  {    0, KEY_J}}, //left
  {&sns3,  {KEY_K, KEY_J}}, //left, down
  {&sns2,  {KEY_K,     0}}, //down
  {&sns1,  {KEY_K, KEY_L}}, //down, right
  {&sns0,  {    0, KEY_L}}, //right
  {&sns7,  {KEY_I, KEY_L}}, //right, up
};
struct KeyboardTouch startKey={&sns8,  {KEY_1,     0}}; //start

struct KeyboardTouch moveKeys[numSensors]= {
  {&sns6,  {KEY_E,     0}}, //up
  {&sns7,  {KEY_E, KEY_S}}, //up, left
  {&sns0,  {    0, KEY_S}}, //left
  {&sns1,  {KEY_D, KEY_S}}, //left, down
  {&sns2,  {KEY_D,     0}}, //down
  {&sns3,  {KEY_D, KEY_F}}, //down, right
  {&sns4, {    0, KEY_F}}, //right
  {&sns5, {KEY_E, KEY_F}} //right, up
};


//touched key order
int touchKeys[numSensors];
int lastKeyIdx=0;
int moveKey=0;

CapacitiveTouchSwitch *sensors[numSensors]={&sns0, &sns1, &sns2, &sns3, &sns4, &sns5, &sns6, &sns7};

int last_key=-1;

int last_on() {
  int rVal=-1;
  int least=0x7FFF;
  for (int i=0; i<numSensors; i++) {
    int elapsed=lastKeyIdx-touchKeys[i];
    if (elapsed<least && keys[i].key->State()) {
      least=elapsed;
      rVal=i;
    }
  }
  return rVal;
}

void setKeys(struct KeyboardTouch *leftKey, struct KeyboardTouch *rightKey, struct KeyboardTouch *startKey) {
  Keyboard.set_key1(0);
  Keyboard.set_key2(0);
  Keyboard.set_key3(0);
  Keyboard.set_key4(0);
  if (leftKey) {
    Keyboard.set_key1(leftKey->keys[0]);
    Keyboard.set_key2(leftKey->keys[1]);
  }
  if (rightKey) {
    Keyboard.set_key3(rightKey->keys[0]);
    Keyboard.set_key4(rightKey->keys[1]);
  }
  Keyboard.set_key5(startKey && startKey->key->State() ? startKey->keys[0] : 0);
  Keyboard.send_now();
}

//insert point above
void setup () {
  //Start serial. Baud rate is ignored with USB.
#ifdef USE_SERIAL
  Serial.begin(9600);
  Serial.println("1 8 Way");
#endif
  //set led pins as output low
  for (int i=0; i<numSensors; i++) {
    //configure sensor Hysteresis and timeout
    CapacitiveTouchSwitch *key=keys[i].key;
    key->SetHysteresis(threshold/2); 
    key->SetTimeoutMs(60000); //5 seconds, for now
    key->SetBaselineAlways(false); //don't adjust baseline while on 
    touchKeys[i]=0;
  }
  //set up the last out pins
  for (int i=0; i<4; i++) {
    pinMode(lastTouchPins[i], INPUT);
  }
  pinMode(handshakeOut, OUTPUT);
  pinMode(handshakeIn, INPUT);
  
}

void print_stats () {
  for (int i=0; i<numSensors; i++) {
    Serial.print(keys[i].key->Diff());
    Serial.print(" ");
  }
  Serial.println();
}

int readLastOutPins() {
  int out=0;
  for (int i=0; i<4; i++) {
    out >>= 1;
    if (digitalRead(lastTouchPins[i])==HIGH) {
      out|=8;
    }
  }
  if (out>=numSensors) {
    out=-1;
  }
  return out;
}

void waitHandshakeHigh() {
  while (digitalRead(handshakeIn)==LOW) {
  }
}

void waitHandshakeLow() {
  while (digitalRead(handshakeIn)==HIGH) {
  }
}

void handshakeHigh() {
  digitalWrite(handshakeOut, HIGH);
}

void handshakeLow() {
  digitalWrite(handshakeOut, LOW);
}


void loop () {
  bool startChanged=false;
  startKey.key->Update();
  if (startKey.key->Changed()) {
    startChanged=true;
#ifdef USE_SERIAL
    if (startKey.key->State()) {
      Serial.println("start on");
    } else {
      Serial.println("start off");
    }
#endif
  }
  for (int i=0; i<numSensors; i++) {
    CapacitiveTouchSwitch *key=keys[i].key;
    key->Update();
    //look for a change in switch state
    if (key->Changed()) {
      if (key->State()) {
#ifdef USE_SERIAL
        Serial.print(i);
        Serial.println(" on");
#endif
        //save the key
        touchKeys[i]=lastKeyIdx;
        lastKeyIdx++;
      } else {
#ifdef USE_SERIAL
        Serial.print(i);
        Serial.println(" off");
#endif
      }
    }
  }
  int new_last_key=last_on();
  //wait until we see data ready from slave
#ifdef USE_SERIAL
  //Serial.println("waiting high");
#endif
  waitHandshakeHigh();
  int newMoveKey=readLastOutPins();
  //indicate OK to move on
  handshakeHigh();
#ifdef USE_SERIAL
  //Serial.println("waiting low");
#endif

  waitHandshakeLow();
  handshakeLow();

  if (new_last_key!=last_key || newMoveKey!= moveKey || startChanged) {
    last_key=new_last_key;
    moveKey=newMoveKey;
#ifdef USE_SERIAL
    Serial.print(last_key);
    Serial.print(" ");
    Serial.print(moveKey);
    Serial.print(" ");
    Serial.println(startChanged);
#endif
    setKeys(last_key>=0 ? keys+last_key : 0, moveKey>=0 ? moveKeys+moveKey : 0 , &startKey);
  }
}

