#pragma once
#if defined(MESHIT_TDECK_PLUS)
#include <Wire.h>
#include "DeckScene.h"
class DeckTouch {
  uint8_t address=0;uint32_t lastContact=0;meshit::DeckGesture gesture;
  bool read(uint16_t reg,uint8_t* data,size_t size){
    Wire.beginTransmission(address);Wire.write(uint8_t(reg>>8));Wire.write(uint8_t(reg));if(Wire.endTransmission(false))return false;
    if(Wire.requestFrom(address,uint8_t(size))!=size)return false;for(size_t i=0;i<size;++i)data[i]=Wire.read();return true;
  }
  void clear(){Wire.beginTransmission(address);Wire.write(uint8_t(0x81));Wire.write(uint8_t(0x4e));Wire.write(uint8_t(0));Wire.endTransmission();}

public:
  bool begin(){pinMode(16,INPUT);for(uint8_t candidate:{uint8_t(0x5d),uint8_t(0x14)}){address=candidate;uint8_t id[4]={};if(read(0x8140,id,4)&&id[0]=='9'&&id[1]=='1'&&id[2]=='1'){clear();return true;}}address=0;return false;}
  meshit::DeckPointerEvent poll(){
    if(!address)return {};uint8_t status=0;
    if(!read(0x814e,&status,1)||!(status&0x80)){
      // Recover a lost release report instead of retaining the previous gesture axis.
      if(uint32_t(millis()-lastContact)>750)return gesture.cancel();return {};
    }
    const unsigned count=status&15;if(!count){clear();return gesture.release();}
    uint8_t point[8]={};if(count>5||!read(0x8150,point,sizeof(point))){clear();return {};}
    clear();const auto xy=meshit::deckTouchPoint(point[0]|(int(point[1])<<8),point[2]|(int(point[3])<<8),true);
    if(xy.first<0||xy.second<0||xy.first>=320||xy.second>=240)return {};
    lastContact=millis();return gesture.contact(xy.first,xy.second);
  }
};
#endif
