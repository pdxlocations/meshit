#pragma once
#include <Arduino.h>
#include "BatteryGauge.h"
struct BatteryReading {uint16_t millivolts=0;};
inline BatteryReading& batteryReading(){static BatteryReading reading;return reading;}
#if defined(MESHIT_ESP32) && defined(MESHIT_HAS_OLED)
// Heltec V3: GPIO1, 390k/100k divider, GPIO37 enable. Detect V3/V3.2 polarity.
inline void updateBattery(uint32_t now) {
  static bool initialized=false,sampling=false;
  static uint32_t next=0,settledAt=0;
  if(!initialized){
    pinMode(1,INPUT);
    pinMode(37,INPUT);
    analogSetPinAttenuation(1,ADC_2_5db);initialized=true;
  }
  if(!sampling){
    if(int32_t(now-next)<0)return;
    const int enabled=digitalRead(37)==HIGH?LOW:HIGH;
    digitalWrite(37,enabled);pinMode(37,OUTPUT);
    settledAt=now+10;sampling=true;return;
  }
  if(int32_t(now-settledAt)<0)return;
  uint32_t sum=0;
  for(unsigned i=0;i<8;++i)sum+=analogReadMilliVolts(1);
  pinMode(37,INPUT);
  sampling=false;next=now+5000;
  const uint32_t denominator=8*10;
  const uint32_t mv=(sum*49+denominator/2)/denominator;
  auto& reading=batteryReading();
  if(!plausibleBatteryVoltage(mv)){reading.millivolts=0;return;}
  reading.millivolts=reading.millivolts?(3*reading.millivolts+mv)/4:mv;
}
#else
inline void updateBattery(uint32_t){}
#endif
