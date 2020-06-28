#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include "RTClib.h"
#include <EEPROM.h>

#define btnNONE 0
#define btnRIGHT 1
#define btnUP 2
#define btnDOWN 3
#define btnLEFT 4
#define btnSELECT 5

DateTime now;

char buffer[30];

//const char MENU1[] PROGMEM = "SET DATE & TIME";
const char MENU1[] PROGMEM = "SET SENSOR DLY";
const char MENU2[] PROGMEM = "SET MOTOR TIME";
const char MENU3[] PROGMEM = "SET UV HLD TIME";
const char MENU4[] PROGMEM = "SET UV ON TIME";
//const char MENU6[] PROGMEM = "SET MOTOR MODE";

const char *const mainMenu[] PROGMEM = {MENU1, MENU2, MENU3, MENU4};

//RTC_DS3231 rtc;
//TEST
RTC_DS1307 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void showDateTime(void);
byte key_press(void);
byte read_LCD_buttons(void);
void check_button_input(void);
void displayMainMenu(char);
void scrollMainMenu(void);
void selectMenuOption(void);
void printDataUpdate(void);
void changeModeScreen(void);
void changeMode(void);
void checkInitTrig(void);
void checkUVTrig(void);
void changeUvOn(void);
void changeUvOnScreen(void);
void changeUVTrig(void);
void changeUVTrigScreen(void);
void changeMotorTime(void);
void changeMotorTimeScreen(void);

boolean mainMenuFlag = false;
boolean manualMenuFlag = false;
boolean adjDtTimeFlag = false;
boolean editSumMFlag = false;
boolean editWinFlag = false;
boolean editExmMFlag = false;
boolean changeModeFlag = false;

int mainMenupos = 0;
int tempHour;
int tempMin;
int tempSec;
int tempHourUV;
int tempMinUV;
int tempSecUV;
int prevHour;
int prevMin;
int uvHour;
int uvMin;
int uvSec;
int prevSec;
int cursor = 0;

int currMotor = 1;
#define trigPin 0
#define sensorPin 13
#define motor1Pin 11
#define buzzerPin 12
#define uvTrigPin A1
#define resetPin A4

#define countAddr 0
#define sensorDelayAddr 3
#define motorTimeAddr 5
#define uvHoldAddr 7
#define uvOnAddr 9
#define motorModeAddr 11

boolean uvFlag = false;
boolean uvCleanFlag = false;
boolean motorFlag = false;
int sensorDelay;
int motorTime;
int uvHold;
int uvOn;
int motorMode;
int motorMode1;
unsigned long uvStartTime = 0;

char currMode;
int nxtBellHour;
int nxtBellMin;
int nxtBellFile;
int currBellDur;

//////////////////////////////////////////////////////////////////////////
// debounce a button
//////////////////////////////////////////////////////////////////////////
int counter = 0;          // how many times we have seen new value
long previous_time = 0;   // the last time the output pin was sampled
byte debounce_count = 50; // number of millis/samples to consider before declaring a debounced input
byte current_state = 0;   // the debounced input value

void setup()
{
  pinMode(trigPin, INPUT);
  pinMode(sensorPin, INPUT);
  pinMode(resetPin, INPUT);
  pinMode(motor1Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(uvTrigPin, OUTPUT);

  digitalWrite(sensorPin, HIGH);
  digitalWrite(trigPin, HIGH);
  digitalWrite(resetPin, LOW);
  digitalWrite(motor1Pin, HIGH);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(uvTrigPin, HIGH);

  lcd.begin(16, 2);

  /* {
    EEPROM.update(i, 0);
  }
   */

  if (!rtc.begin())
  {
    while (1)
      ;
  }

  if (!rtc.isrunning())
  //TEST
  //if (!rtc.lostPower())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  lcd.setCursor(0, 0);
  lcd.print("  CAMSOL ENGG  ");
  lcd.setCursor(0, 1);
  lcd.print("   SOLUTIONS   ");
  delay(1000);
  lcd.clear();

  DateTime now = rtc.now();

  now = rtc.now();

  tempMinUV = 0;
  tempHourUV = 0;
  tempSecUV = 0;
  prevMin = now.minute();
  prevSec = now.second();
  uvHour = 0;
  uvMin = 0;
  uvFlag = true;
}

void loop()
{
  checkInitTrig();
  checkUVTrig();
  if (!mainMenuFlag && !adjDtTimeFlag && !editSumMFlag && !editWinFlag && !editExmMFlag && !changeModeFlag && !manualMenuFlag)
  {
    showDateTime();
  }
  check_button_input();
}

void checkInitTrig()
{
  int wait_time = EEPROM.read(sensorDelayAddr);
  int motorTime = EEPROM.read(motorTimeAddr);
  currMotor = 1;
  boolean sensorTrig = false;
  if (digitalRead(trigPin) == LOW)
  {
    if (uvCleanFlag)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("UV Cleansing");
      lcd.setCursor(0, 1);
      lcd.write("Interrupt!!");
      uvCleanFlag = false;
      digitalWrite(uvTrigPin, HIGH);
      now = rtc.now();
      prevMin = now.minute();
      prevSec = now.second();
      delay(1000);
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write("Person Entry");
    lcd.setCursor(0, 1);
    lcd.write("Detected");
    delay(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write("Waiting For");
    lcd.setCursor(0, 1);
    lcd.write("Person");

    digitalWrite(buzzerPin, HIGH);
    delay(wait_time * 1000);
    digitalWrite(buzzerPin, LOW);
    motorFlag = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write("MOTOR ");
    lcd.print(currMotor);
    lcd.setCursor(0, 1);
    lcd.write("Running");
    digitalWrite(motor1Pin, LOW);
    delay(motorTime * 1000);
    digitalWrite(motor1Pin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write("Person");
    lcd.setCursor(0, 1);
    lcd.write("Exiting");
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
    now = rtc.now();
    prevMin = now.minute();
    prevSec = now.second();
    uvCleanFlag = false;
    uvFlag = true;
  }
}

void checkUVTrig()
{
  now = rtc.now();
  int uvTime = EEPROM.read(uvHoldAddr);
  int uvONTime = EEPROM.read(uvOnAddr);
  tempHourUV = now.hour();
  tempMinUV = now.minute();
  tempSecUV = now.second();

  int prevTime = prevMin * 60 + prevSec;
  int currTime = tempMinUV * 60 + tempSecUV;
  unsigned long uvStopTime = (tempHourUV * 3600) + (tempMinUV * 60) + tempSecUV;

  // uvFlag = false;

  if (currTime - prevTime >= uvTime && !uvCleanFlag && uvFlag)
  {

    digitalWrite(uvTrigPin, LOW);
    uvHour = now.hour();
    uvMin = now.minute();
    uvSec = now.second();
    uvStartTime = (uvHour * 3600) + (uvMin * 60) + uvSec;
    uvCleanFlag = true;
  }

  if (uvCleanFlag)
  {
    if (uvStopTime - uvStartTime >= uvONTime * 60)
    {
      digitalWrite(uvTrigPin, HIGH);
      uvCleanFlag = false;
      uvFlag = false;
    }
  }
}

void showDateTime()
{

  if (uvCleanFlag)
  {
    lcd.setCursor(0, 0);
    lcd.write("  UV Cleansing  ");
    lcd.setCursor(0, 1);
    lcd.write("     Process    ");
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.write("     WAITING     ");
    lcd.setCursor(0, 1);
    lcd.write("   FOR PERSON  ");
  }
}

byte read_LCD_buttons()
{                                 // read the buttons
  int adc_key_in = analogRead(0); // read the value from the sensor

  //value read: 0(0V), 130(0.64V), 306(1.49V), 479(2.33V), 722(3.5V), 1023(4.97V)
  /* if (adc_key_in > 1000)
    return btnNONE;
  if (adc_key_in < 60)
    return btnRIGHT;
  if (adc_key_in < 200)
    return btnDOWN;
  if (adc_key_in < 400)
    return btnUP;
  if (adc_key_in < 600)
    return btnLEFT;
  if (adc_key_in < 800)
    return btnSELECT;
  return btnNONE; */

  //TEST
  if (adc_key_in > 1000)
    return btnNONE;
  if (adc_key_in < 580 && adc_key_in > 480)
    return btnRIGHT;
  if (adc_key_in < 280 && adc_key_in > 150)
    return btnDOWN;
  if (adc_key_in < 380 && adc_key_in > 280)
    return btnUP;
  if (adc_key_in < 640 && adc_key_in > 580)
    return btnLEFT;
  if (adc_key_in < 1000 && adc_key_in > 900)
    return btnSELECT;
  return btnNONE;
}

byte key_press()
{
  // If we have gone on to the next millisecond
  if (millis() != previous_time)
  {
    byte this_button = read_LCD_buttons();

    if (this_button == current_state && counter > 0)
      counter--;

    if (this_button != current_state)
      counter++;

    // If the Input has shown the same value for long enough let's switch it
    if (counter >= debounce_count)
    {
      counter = 0;
      current_state = this_button;
      return this_button;
    }
    previous_time = millis();
  }
  return 0;
}

void check_button_input()
{

  if (!mainMenuFlag && !adjDtTimeFlag && !editSumMFlag && !editWinFlag && !editExmMFlag && !changeModeFlag && !manualMenuFlag)
  {
    byte lcd_key = key_press();
    switch (lcd_key)
    {
    case btnLEFT:
    {
      mainMenuFlag = true;
      displayMainMenu('H');
      break;
    }
    case btnRIGHT:
    {
      break;
    }

    default:
      break;
    }
  }
  else if (mainMenuFlag)
  {
    scrollMainMenu();
  }

  else if (editSumMFlag)
  {
    changeMotorTime();
  }
  else if (editWinFlag)
  {
    //setSchedBell('W');
    changeUVTrig();
  }
  else if (editExmMFlag)
  {
    //etSchedBell('E');
    changeUvOn();
  }

  else if (changeModeFlag)
  {
    changeMode();
  }
  return;
}

void scrollMainMenu()
{
  byte lcd_key = key_press(); // read the buttons
  switch (lcd_key)
  {
  case btnLEFT:
  {
    selectMenuOption();
    break;
  }
  case btnDOWN:
  {
    displayMainMenu('D');
    break;
  }
  case btnUP:
  {
    displayMainMenu('U');
    break;
  }
  case btnRIGHT:
  {
    mainMenuFlag = false;
    mainMenupos = 0;
    lcd.clear();
    break;
  }
  }
  return;
}

void selectMenuOption()
{
  if (mainMenupos == 0)
  {
    changeModeFlag = true;
    mainMenuFlag = false;
    cursor = 0;
    sensorDelay = EEPROM.read(sensorDelayAddr);
    changeModeScreen();
  }
  else if (mainMenupos == 1)
  {
    editSumMFlag = true;
    mainMenuFlag = false;
    motorTime = EEPROM.read(motorTimeAddr);
    changeMotorTimeScreen();
  }
  else if (mainMenupos == 2)
  {
    editWinFlag = true;
    mainMenuFlag = false;
    uvHold = EEPROM.read(uvHoldAddr);
    changeUVTrigScreen();
  }
  else if (mainMenupos == 3)
  {
    editExmMFlag = true;
    mainMenuFlag = false;
    uvOn = EEPROM.read(uvOnAddr);
    changeUvOnScreen();
  }

  return;
}

void displayMainMenu(char c)
{
  int fisrtLinePos;
  int scndLinePos;
  if (c == 'H')
  {
    fisrtLinePos = mainMenupos;
    scndLinePos = fisrtLinePos + 1;
  }
  else if (c == 'D')
  {
    mainMenupos++;
    if (mainMenupos <= 3)
    {
      fisrtLinePos = mainMenupos;
      scndLinePos = fisrtLinePos + 1;
    }
    else if (mainMenupos > 3)
    {
      mainMenupos = 0;
      fisrtLinePos = mainMenupos;
      scndLinePos = fisrtLinePos + 1;
    }
  }
  else if (c == 'U')
  {
    mainMenupos--;
    if (mainMenupos >= 0)
    {
      fisrtLinePos = mainMenupos;
      scndLinePos = fisrtLinePos + 1;
    }
    else if (mainMenupos < 0)
    {
      mainMenupos = 3;
      fisrtLinePos = mainMenupos;
      scndLinePos = fisrtLinePos + 1;
    }
  }

  lcd.clear();
  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print(">");
  lcd.setCursor(1, 0);
  strcpy_P(buffer, (char *)pgm_read_word(&(mainMenu[fisrtLinePos])));
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(" ");
  lcd.setCursor(1, 1);
  if (scndLinePos == 4)
  {
    lcd.print("                ");
  }
  else
  {
    strcpy_P(buffer, (char *)pgm_read_word(&(mainMenu[scndLinePos])));
    lcd.print(buffer);
  }
}

void changeModeScreen()
{
  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print("SET SENSOR DLY");
  lcd.setCursor(0, 1);
  lcd.print(sensorDelay);
  lcd.print(" SEC          ");
  lcd.setCursor(cursor, 1);
  lcd.blink();
}

void changeMode()
{
  byte lcd_key = key_press(); // read the buttons
  switch (lcd_key)
  {
  case btnUP:
  {
    sensorDelay++;
    changeModeScreen();
    break;
  }
  case btnDOWN:
  {
    if (sensorDelay > 0)
    {
      sensorDelay--;
    }
    changeModeScreen();
    break;
  }

  case btnRIGHT:
  {
    changeModeFlag = false;
    mainMenuFlag = true;
    displayMainMenu('H');
    //changeModeScreen();
    break;
  }
  case btnLEFT:
  {

    EEPROM.update(sensorDelayAddr, sensorDelay);

    changeModeFlag = false;
    mainMenuFlag = true;
    printDataUpdate();
  }

  default:
    break;
  }
}

void changeMotorTimeScreen()
{
  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print("SET MOTOR TIME");
  lcd.setCursor(0, 1);
  lcd.print(motorTime);
  lcd.print(" SEC            ");
  lcd.setCursor(cursor, 1);
  lcd.blink();
}

void changeMotorTime()
{
  byte lcd_key = key_press(); // read the buttons
  switch (lcd_key)
  {
  case btnUP:
  {
    motorTime++;
    changeMotorTimeScreen();
    break;
  }
  case btnDOWN:
  {
    if (motorTime > 0)
    {
      motorTime--;
    }
    changeMotorTimeScreen();
    break;
  }

  case btnRIGHT:
  {
    editSumMFlag = false;
    mainMenuFlag = true;
    displayMainMenu('H');
    changeMotorTimeScreen();
    break;
  }
  case btnLEFT:
  {

    EEPROM.update(motorTimeAddr, motorTime);

    editSumMFlag = false;
    mainMenuFlag = true;
    printDataUpdate();
  }

  default:
    break;
  }
}
void changeUVTrigScreen()
{
  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print("SET UV DELAY");
  lcd.setCursor(0, 1);
  lcd.print(uvHold);
  lcd.print(" SEC           ");
  lcd.setCursor(cursor, 1);
  lcd.blink();
}

void changeUVTrig()
{
  byte lcd_key = key_press(); // read the buttons
  switch (lcd_key)
  {
  case btnUP:
  {
    uvHold++;
    changeUVTrigScreen();
    break;
  }
  case btnDOWN:
  {
    if (uvHold > 0)
    {
      uvHold--;
    }
    changeUVTrigScreen();
    break;
  }

  case btnRIGHT:
  {
    editWinFlag = false;
    mainMenuFlag = true;
    displayMainMenu('H');
    changeUVTrigScreen();
    break;
  }
  case btnLEFT:
  {

    EEPROM.update(uvHoldAddr, uvHold);

    editWinFlag = false;
    mainMenuFlag = true;
    printDataUpdate();
  }

  default:
    break;
  }
}

void changeUvOnScreen()
{
  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print("SET UV DUR");
  lcd.setCursor(0, 1);
  lcd.print(uvOn);
  lcd.print(" MIN            ");
  lcd.setCursor(cursor, 1);
  lcd.blink();
}

void changeUvOn()
{
  byte lcd_key = key_press(); // read the buttons
  switch (lcd_key)
  {
  case btnUP:
  {
    uvOn++;
    changeUvOnScreen();
    break;
  }
  case btnDOWN:
  {
    if (uvOn > 0)
    {
      uvOn--;
    }
    changeUvOnScreen();
    break;
  }

  case btnRIGHT:
  {
    editExmMFlag = false;
    mainMenuFlag = true;
    displayMainMenu('H');
    changeUvOnScreen();
    break;
  }
  case btnLEFT:
  {

    EEPROM.update(uvOnAddr, uvOn);

    editExmMFlag = false;
    mainMenuFlag = true;
    printDataUpdate();
  }

  default:
    break;
  }
}
void printDataUpdate()
{
  lcd.clear();
  lcd.noBlink();
  lcd.noCursor();
  lcd.setCursor(0, 0);
  lcd.print("******DATA******");
  lcd.setCursor(0, 1);
  lcd.print("****UPDATED*****");
  delay(1000);
  displayMainMenu('H');
}