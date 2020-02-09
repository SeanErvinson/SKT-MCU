#include <SPI.h>
#include <BLEPeripheral.h>
#include <ClickButton.h>
#include <TinyGPS++.h>
#include "pitches.h"

#define BLE_REQ   10
#define BLE_RDY   2
#define BLE_RST   9

#define SPEAKER_PIN   28
#define BUTTON_PIN   21

#define RL_PIN 3
#define GL_PIN 4
#define BL_PIN 5

#define RX_PIN 6
#define TX_PIN 7
#define GPS_P_PIN 8

#define BATTERY_PIN A0

TinyGPSPlus gps;
ClickButton button1(BUTTON_PIN, LOW, CLICKBTN_PULLUP);
BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);

BLEService lSer = BLEService("1100");
BLEUnsignedIntCharacteristic lChar = BLEUnsignedIntCharacteristic("1101", BLEWrite | BLERead |   BLENotify);
BLEDescriptor lDesc = BLEDescriptor("2901", "Locate");

BLEService fmpSer = BLEService("FF00");
BLEUnsignedIntCharacteristic fmpChar = BLEUnsignedIntCharacteristic("FF01", BLERead | BLENotify);
BLEDescriptor fmpDesc = BLEDescriptor("2901", "Find My Phone");

BLEService longSer = BLEService("DD00");
BLEFloatCharacteristic longChar = BLEFloatCharacteristic("DD01", BLERead | BLENotify);
BLEDescriptor longDesc = BLEDescriptor("2901", "Long");

BLEService latSer = BLEService("DD02");
BLEFloatCharacteristic latChar = BLEFloatCharacteristic("DD03", BLERead | BLENotify);
BLEDescriptor latDesc = BLEDescriptor("2901", "Lat");

BLEService testSer = BLEService("0002");
BLEIntCharacteristic testChar = BLEIntCharacteristic("0001", BLERead |   BLENotify);
BLEDescriptor testDesc = BLEDescriptor("2901", "Test");


const int HIGH_BATTERY = 840;
const int MEDIUM_BATTERY = 760;
const int LOW_BATTERY = 650;
const int SOUND_BIT = 8;

unsigned int buttonFunction = 0;
unsigned long fmpMillis = 0;
unsigned long batIndMillis = 0;

float batteryVoltage = 0;
bool isAlarming = false;
int currentNoteDurations[8];
int currentMelody[8];

void setup() {  
  Serial.setPins(6,7);
  Serial.begin(9600);
  
  pinMode(RL_PIN, OUTPUT);
  pinMode(GL_PIN, OUTPUT);
  pinMode(BL_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
//  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BATTERY_PIN, INPUT);

  blePeripheral.setLocalName("SKT-T1");
  blePeripheral.setAdvertisedServiceUuid(lSer.uuid());
  blePeripheral.addAttribute(lSer);
  blePeripheral.addAttribute(lChar);
  blePeripheral.addAttribute(lDesc);
  
  blePeripheral.setAdvertisedServiceUuid(fmpSer.uuid());
  blePeripheral.addAttribute(fmpSer);
  blePeripheral.addAttribute(fmpChar);
  blePeripheral.addAttribute(fmpDesc);
  
  blePeripheral.setAdvertisedServiceUuid(longSer.uuid());
  blePeripheral.addAttribute(longSer);
  blePeripheral.addAttribute(longChar);
  blePeripheral.addAttribute(longDesc);

  blePeripheral.setAdvertisedServiceUuid(latSer.uuid());
  blePeripheral.addAttribute(latSer);
  blePeripheral.addAttribute(latChar);
  blePeripheral.addAttribute(latDesc);

  blePeripheral.setAdvertisedServiceUuid(testSer.uuid());
  blePeripheral.addAttribute(testSer);
  blePeripheral.addAttribute(testChar);
  blePeripheral.addAttribute(testDesc);
  
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  lChar.setValue(0);
  button1.debounceTime   = 20; 
  button1.multiclickTime = 100;
  button1.longClickTime  = 1000;
  blePeripheral.begin();
}

void loop() {
  blePeripheral.poll();
  button1.Update();
  if (button1.clicks != 0) buttonFunction = button1.clicks;
//  BLECentral central = blePeripheral.central();
//  if(connected()){

  if(button1.clicks == 1) {
    if(isAlarming){
      isAlarming = false;
      lChar.setValue(0);
    }
    else checkBattery();
  }
  
//  if(buttonFunction == 3) setfmpCharValue();
  
  if(lChar.written()){
    activateSound(lChar.value());
    isAlarming = true;
  }
  
  
// Continuous Service
  setGPSCharValue();
//  readBatteryVoltage();
  playSound();
//  }

  if(buttonFunction == -2){
//     if(connected()){
//    disconnect();
//      pairingMode();
//     }
  }
  
//  if(buttonFunction == -4){
//      Serial.println("Closing");  
//      disconnect();
//    }
  buttonFunction = 0;
}

void readBatteryVoltage(){
  batteryVoltage = analogRead(BATTERY_PIN);
}

void setfmpCharValue(){
  unsigned long currentMillis = millis();
  fmpChar.setValue(1);
  changeColor(115, 255, 0);
  if(currentMillis - fmpMillis >= 50){fmpMillis = currentMillis;}
  fmpChar.setValue(0); 
  changeColor(0, 0, 0);
}

void setGPSCharValue() {
  if (Serial.available() > 0) {
    while(gps.encode(Serial.read())) {
      if (gps.location.isValid()) {
        longChar.setValue(gps.location.lng());
        latChar.setValue(gps.location.lat());
      }
    }
  }
}


void activateSound(int sound){
  if(sound == 1){
    int noteDurations[] = { 4, 8, 8, 4, 4, 4, 4, 4 };
    int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_C4, NOTE_B3, 0, NOTE_G2, NOTE_C4};
    memcpy(currentNoteDurations, noteDurations, SOUND_BIT);
    memcpy(currentMelody, melody, SOUND_BIT);
  }
  else if(sound == 2){
    int noteDurations[] = { 4, 8, 8, 4, 4, 4, 4, 4 };
    int melody[] = {NOTE_B3, NOTE_G2, NOTE_G1, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    memcpy(currentNoteDurations, noteDurations, SOUND_BIT);
    memcpy(currentMelody, melody, SOUND_BIT);
  }
  else{
    int noteDurations[] = { 4, 8, 8, 4, 4, 4, 4, 4 };
    int melody[] = {NOTE_B2, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    memcpy(currentNoteDurations, noteDurations, SOUND_BIT);
    memcpy(currentMelody, melody, SOUND_BIT);
  }
}


void playSound() {
  if(isAlarming){
    for (int index = 0; index < SOUND_BIT; index++) {
      analogWrite(SPEAKER_PIN, 255);
    };
//    for (int index = 0; index < SOUND_BIT; index++) {
//      int noteDuration = 1000 / currentNoteDurations[index];
//      //    if(millis() >= noteDuration)
//      analogWrite(SPEAKER_PIN, currentMelody[index]);
//      int pauseBetweenNotes = noteDuration * 1.30;
//      delay(pauseBetweenNotes);
//      analogWrite(SPEAKER_PIN, 0);
//    }
  }else{
    analogWrite(SPEAKER_PIN, 0);
  }
}

void pairingMode() {
  for (int i = 0; i < 10; i++) {
    changeColor(240, 0, 0);
    delay(200);
    changeColor(0, 0, 0);
    delay(200);
  }
}

void checkBattery() {
  unsigned long currentMillis = millis();
  if (batteryVoltage <= HIGH_BATTERY && batteryVoltage > MEDIUM_BATTERY) {
    changeColor(0, 255, 0); //Green
  } else if (batteryVoltage < MEDIUM_BATTERY && batteryVoltage > LOW_BATTERY) {
    changeColor(254, 203, 0); //Yellow
  } else if (batteryVoltage < LOW_BATTERY) {
    changeColor(255, 0, 0); //Red
  }
  if (currentMillis - batIndMillis >= 2000) {
    changeColor(0, 0, 0);
    batIndMillis = currentMillis;
  }
}

void changeColor(int red, int green, int blue) {
  analogWrite(RL_PIN, red);
  analogWrite(GL_PIN, green);
  analogWrite(BL_PIN, blue);
}

void blePeripheralConnectHandler(BLECentral & central) {
  digitalWrite(GPS_P_PIN, HIGH);
}

void blePeripheralDisconnectHandler(BLECentral & central) {
  digitalWrite(GPS_P_PIN, LOW);
  pairingMode();
}
