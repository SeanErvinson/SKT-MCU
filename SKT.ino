#include <SPI.h>
#include <BLEPeripheral.h>
#include <TinyGPS++.h>

#define BLE_REQ   10
#define BLE_RDY   2
#define BLE_RST   9

#define SPEAKER_PIN   3
#define BUTTON_PIN   21

#define RL_PIN 22
#define GL_PIN 23
#define BL_PIN 24

#define GPS_P_PIN 6
#define RX_PIN 1
#define TX_PIN 8

#define BATTERY_PIN 4

TinyGPSPlus gps;
BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);

BLEService aSer = BLEService("AA00");
BLEUnsignedIntCharacteristic aChar = BLEUnsignedIntCharacteristic("AA01", BLEWrite | BLERead | BLENotify);
BLEDescriptor aDesc = BLEDescriptor("2901", "Alert");

BLEService fmpSer = BLEService("FF00");
BLEUnsignedIntCharacteristic fmpChar = BLEUnsignedIntCharacteristic("FF01", BLERead | BLENotify);
BLEDescriptor fmpDesc = BLEDescriptor("2901", "Find My Phone");

BLEService gpsSer = BLEService("DD00");
BLEFloatCharacteristic longChar = BLEFloatCharacteristic("DD01", BLERead | BLENotify);
BLEFloatCharacteristic latChar = BLEFloatCharacteristic("DD02", BLERead | BLENotify);
BLEDescriptor gpsDesc = BLEDescriptor("2901", "GPS");

const unsigned int HIGH_BATTERY = 840;
const unsigned int MEDIUM_BATTERY = 760;
const unsigned int LOW_BATTERY = 650;
const unsigned int speakerInterval = 500;
const unsigned int pairingInterval = 300;
const unsigned int batIndInterval = 300;
const unsigned int batCheckInterval = 3000;

unsigned long currentMillis;
unsigned long fmpMillis = 0;
unsigned long batIndMillis = 0;
unsigned long speakerMillis = 0;
unsigned long pairingMillis = 0;
unsigned long batCheckMillis = 0;

float batteryVoltage = 0;
boolean isAlarming = false;
unsigned int loudness = 255;
boolean speakerState = false;
boolean pairingState = true;
boolean batteryState = false;

void setup() {  
  Serial.setPins(RX_PIN, TX_PIN);
  Serial.begin(9600);
  
  pinMode(RL_PIN, OUTPUT);
  pinMode(GL_PIN, OUTPUT);
  pinMode(BL_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(BATTERY_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  blePeripheral.setLocalName("SKT-T1");
  blePeripheral.setDeviceName("SKT-T1");
  blePeripheral.setAdvertisedServiceUuid(aSer.uuid());
  blePeripheral.addAttribute(aSer);
  blePeripheral.addAttribute(aChar);
  blePeripheral.addAttribute(aDesc);
  
  blePeripheral.setAdvertisedServiceUuid(fmpSer.uuid());
  blePeripheral.addAttribute(fmpSer);
  blePeripheral.addAttribute(fmpChar);
  blePeripheral.addAttribute(fmpDesc);
  
  blePeripheral.setAdvertisedServiceUuid(gpsSer.uuid());
  blePeripheral.addAttribute(gpsSer);
  blePeripheral.addAttribute(latChar);
  blePeripheral.addAttribute(longChar);
  blePeripheral.addAttribute(gpsDesc);
  
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  aChar.setValue(0);
  blePeripheral.begin();
}

void loop() {
  currentMillis = millis();
  blePeripheral.poll();
  BLECentral central = blePeripheral.central();
  if (central) {
    if(digitalRead(BUTTON_PIN) == HIGH && isAlarming) {
      isAlarming = false;
      aChar.setValue(0);
    }
//    if(buttonFunction == 3) setfmpCharValue();
    if(aChar.written()){
      changeColor(100, 100, 50);
      activateSound(aChar.value());
      isAlarming = true;
    }
    // Services
    setGPSCharValue();
  }  
  if (digitalRead(BUTTON_PIN) == HIGH){
    batteryState = true;
  }
// Continuous Service
  readBatteryVoltage();
  checkBattery();
  playSound();
//  if (buttonFunction == -2) { // 1 Click then hold
//    if (central.connected()) {
//      central.disconnect();
//    }
//  }
}

void readBatteryVoltage(){
  batteryVoltage = analogRead(BATTERY_PIN);
}

void setfmpCharValue(){
  fmpChar.setValue(1);
  if(currentMillis - fmpMillis >= 50){fmpMillis = currentMillis;}
  fmpChar.setValue(0); 
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
  if(sound == 1) loudness = 85;
  else if(sound == 2) loudness = 170;
  else loudness = 255;
}


void playSound() {
  if(isAlarming){
    if (speakerState) {
      if (currentMillis - speakerMillis >= speakerInterval) {
        speakerMillis = currentMillis;
        analogWrite(SPEAKER_PIN, 0);  
        speakerState = false;
      }
    } else {
      if (currentMillis - speakerMillis >= speakerInterval) {
        speakerMillis = currentMillis;
        analogWrite(SPEAKER_PIN, loudness);
        speakerState = true;
      }
    }
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
  if (batteryState) {
    if (batteryVoltage <= HIGH_BATTERY && batteryVoltage > MEDIUM_BATTERY) {
      changeColor(0, 255, 0);
    } else if (batteryVoltage < MEDIUM_BATTERY && batteryVoltage > LOW_BATTERY) {
      changeColor(254, 203, 0); 
    } else {
      changeColor(255, 0, 0);
    }
    delay(250);
    changeColor(0, 0, 0);
    batteryState = false;
  }
}

void changeColor(int red, int green, int blue) {
  analogWrite(RL_PIN, red);
  analogWrite(GL_PIN, green);
  analogWrite(BL_PIN, blue);
}

void blePeripheralConnectHandler(BLECentral & central) {
  digitalWrite(GPS_P_PIN, HIGH);
  changeColor(0,0,150);
}

void blePeripheralDisconnectHandler(BLECentral & central) {
  digitalWrite(GPS_P_PIN, LOW);
  pairingMode();
}
