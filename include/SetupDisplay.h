#pragma once
#include <Arduino.h>

#if defined(MESHIT_ESP32) && defined(MESHIT_HAS_OLED)
#include <Wire.h>
#include <SSD1306Wire.h>

class SetupDisplay {
  SSD1306Wire screen{0x3c,SDA_OLED,SCL_OLED,GEOMETRY_128_64,I2C_ONE,400000};
  bool ready=false;
  String previous;
public:
  bool begin(){
    // Heltec V3's OLED is on its active-low Vext rail and dedicated I2C pins.
    pinMode(Vext,OUTPUT);digitalWrite(Vext,LOW);delay(100);
    pinMode(RST_OLED,OUTPUT);digitalWrite(RST_OLED,LOW);delay(20);digitalWrite(RST_OLED,HIGH);delay(20);
    if(!Wire.begin(SDA_OLED,SCL_OLED,400000))return false;
    Wire.setTimeOut(50);
    // The driver's init() only checks allocation, so explicitly probe first.
    Wire.beginTransmission(0x3c);
    if(Wire.endTransmission()!=0)return false;
    ready=screen.init();
    if(ready){screen.setTextAlignment(TEXT_ALIGN_LEFT);screen.clear();}
    return ready;
  }
  void showAccessPoint(const String& name,const String& password){
    if(!ready)return;
    const String view="ap|"+name+"|"+password;if(view==previous)return;previous=view;
    screen.clear();screen.setFont(ArialMT_Plain_10);
    screen.drawString(0,0,"AP: "+name);screen.drawString(0,12,"Wi-Fi password:");
    screen.setFont(ArialMT_Plain_24);screen.drawString(0,22,password);
    screen.setFont(ArialMT_Plain_10);screen.drawString(0,51,"http://192.168.4.1");screen.display();
  }
  void showNetwork(bool connected,const String& ip){
    if(!ready)return;
    const String view="lan|"+String(connected)+"|"+ip;if(view==previous)return;previous=view;
    screen.clear();screen.setFont(ArialMT_Plain_10);
    screen.drawString(0,0,connected?"Wi-Fi connected":"Wi-Fi disconnected");
    if(connected){screen.drawString(0,18,"http://meshit.local");screen.drawString(0,36,ip);}
    else{screen.drawString(0,18,"Use Bluetooth, or");screen.drawString(0,36,"restart for setup Wi-Fi");}
    screen.display();
  }
};
#else
class SetupDisplay {
public:
  bool begin(){return false;}
  void showAccessPoint(const String&,const String&){}
  void showNetwork(bool,const String&){}
};
#endif
