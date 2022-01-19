#include <LiquidCrystal_I2C.h>
#include "bacNames.h"

#define pinOutSig 3
#define pinPot A0
#define pinInV A1
#define pinIncBtn 2
#define pinDecBtn 12


LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

long period = 100000;
double freq = 10;
int lastValuePot = 0;
int prec = 10;
int btnValue = 5;
const int MAX_BTN_VALUE = 6;

int buttonStateInc, buttonStateDec;             // the current reading from the input pin
int lastBtnStateInc = LOW, lastBtnStateDec = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTimeInc = 0, lastDebounceTimeDec = 0;  // the last time the output pin was toggled
const unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

bool LCDtoUpdate = true;
bool presetMode = true;
bool canToggle = true;

int currMode = 0;

void setup() {

  Serial.begin(9600); 
  lcd.init(); // initialize the lcd
  lcd.backlight();

  //count = sizeof(freqsTOP)/sizeof(freqsTOP[0]);
  //pinMode(pinOutSig, OUTPUT);
  pinMode(pinPot, INPUT);
  pinMode(pinIncBtn, INPUT);
  pinMode(pinDecBtn, INPUT);
  pinMode(pinInV, INPUT);
  
  pinMode(pinOutSig, OUTPUT);  
  // non-inverted fast PWM on OC1B with prescalar of 1
  TCCR1A = (1<<COM1A1) | (1<<WGM11); 
  TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS10); 
  ICR1 = getfreqTopAt(currMode);
  OCR1A = ICR1/2 - 1;

  lastValuePot = analogRead(pinPot);

}

String getbackNamesAt(int index){
  char buffer[16];
  strcpy_P(buffer, (char *)pgm_read_word(&(bacNames[index])));
  return buffer;
}

int getfreqTopAt(int index){
  return pgm_read_word_near(freqsTOP + index);
}

void PrintModeToLCD(){
  LCDtoUpdate = false;
  lcd.clear();
  lcd.setCursor(0, 0);

  Serial.println("name:");
  Serial.print(getbackNamesAt(currMode));
  lcd.print(getbackNamesAt(currMode));
  
  lcd.setCursor(0, 1);
  lcd.print(currMode + 1);
  lcd.print("/");
  lcd.print(count);
  lcd.print("  ");
  double f = F_CPU/(ICR1+1);
  lcd.print(f/1000,2);
  lcd.print("kHz");
}

void SetFrequency(){
  if(presetMode){
     ICR1 = getfreqTopAt(currMode);
  }
  else{
    freq = pow(10,btnValue) + lastValuePot / 1024.0 * pow(10,btnValue + 1);
    ICR1 = F_CPU/freq - 1;
  }
    OCR1A = ICR1/2 - 1;
    LCDtoUpdate = true;
}

bool readButtonStateChange(int pin, unsigned long* lastDebounceTime, int* buttonState, int* lastBtnState ){
  bool ret = false;
  int reading = digitalRead(pin);
  if (reading != *lastBtnState) {
    *lastDebounceTime = millis();
  }

  if ((millis() - *lastDebounceTime) > debounceDelay) {
    if (reading != *buttonState) {
      *buttonState = reading;
      ret = true;
    }
  }
  *lastBtnState = reading;
  return ret;
}

void loop() {


bool incButtonChanged = readButtonStateChange(pinIncBtn,&lastDebounceTimeInc,&buttonStateInc,&lastBtnStateInc);
bool decButtonChanged = readButtonStateChange(pinDecBtn,&lastDebounceTimeDec,&buttonStateDec,&lastBtnStateDec);

  if(buttonStateInc == HIGH && buttonStateDec == HIGH ){
    if(canToggle){
      canToggle = false;
      presetMode = !presetMode;
    }
}
else{
  canToggle = true;
  //==================increase button=============
  if(incButtonChanged && buttonStateInc == HIGH && btnValue < MAX_BTN_VALUE){
    btnValue++;
    SetFrequency();
  }
  //==============decrease button=================
  if(decButtonChanged && buttonStateDec == HIGH && btnValue > 0){
    btnValue--;
    SetFrequency();
  }
}

  
  int curr = analogRead(pinPot); 
  if(presetMode){
    int v = map(curr, 0, 1024, 0, count);
    if(v != currMode){
      currMode = v;
      SetFrequency();
    }
  }
  else if(abs(lastValuePot - curr) > prec){
    lastValuePot = curr;
    SetFrequency();
  }
  

  if(LCDtoUpdate) PrintModeToLCD();
  
  //Serial.println(digitalRead(pinPot));
//  Serial.print(analogRead(pinPot)/1024.0 * 5.0);
//  Serial.print(",");
 // Serial.println(analogRead(pinInV)/1024.0 * 5.0);
  delay(100);
}
