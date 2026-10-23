#pragma once
#if defined(MESHIT_TDECK)
#include <Arduino.h>
#include <Wire.h>
#include "../firmware/esp32/Pins.h"
inline int deckReadRegister(uint8_t address,uint8_t reg){
  Wire.beginTransmission(address);Wire.write(reg);if(Wire.endTransmission(false))return -1;
  if(Wire.requestFrom(address,uint8_t(1))!=1)return -1;return Wire.read();
}
inline bool deckWriteRegister(uint8_t address,uint8_t reg,uint8_t value){
  Wire.beginTransmission(address);Wire.write(reg);Wire.write(value);return Wire.endTransmission()==0;
}
inline bool beginDeckHardware(){
#if !defined(MESHIT_TDECK_PRO)
  pinMode(10,OUTPUT);digitalWrite(10,HIGH); // Display, radio and keyboard power.
#endif
  for(int pin:{RADIO_CS,DECK_SD_CS,DECK_DISPLAY_CS}){digitalWrite(pin,HIGH);pinMode(pin,OUTPUT);}
  digitalWrite(RADIO_RESET,HIGH);pinMode(RADIO_RESET,OUTPUT);
  Wire.begin(DECK_SDA,DECK_SCL);Wire.setTimeOut(30);
#if defined(MESHIT_TDECK_PRO_MAX)
  // XL9555: LoRa on, internal antenna selected, touch/keyboard out of reset.
  // Keep the unused cellular, GPS, audio and sensor rails off.
  if(!deckWriteRegister(0x20,2,0x92)||!deckWriteRegister(0x20,3,0x02)||
     !deckWriteRegister(0x20,6,0x00)||!deckWriteRegister(0x20,7,0xf8))return false;
#elif defined(MESHIT_TDECK_PRO)
  pinMode(46,OUTPUT);digitalWrite(46,HIGH);
  for(int pin:{39,41,40}){pinMode(pin,OUTPUT);digitalWrite(pin,LOW);}
#endif
#if defined(MESHIT_TDECK_PRO)
  // Disable the BQ25896 charger watchdog without changing charging limits.
  const int timer=deckReadRegister(0x6b,7);if(timer>=0)deckWriteRegister(0x6b,7,timer&~0x30);
#else
  pinMode(38,INPUT_PULLUP);
#endif
  pinMode(0,INPUT_PULLUP);delay(500);return true;
}
#endif
