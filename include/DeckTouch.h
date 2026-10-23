#pragma once
#if defined(MESHIT_TDECK_PLUS)
#include <Wire.h>
#include "DeckScene.h"
class DeckTouch {
  uint8_t address=0;uint32_t lastContact=0;bool blocked=false;int track=-1;meshit::DeckGesture gesture;
  bool read(uint16_t reg,uint8_t* data,size_t size){
    Wire.beginTransmission(address);Wire.write(uint8_t(reg>>8));Wire.write(uint8_t(reg));if(Wire.endTransmission(false))return false;
    if(Wire.requestFrom(address,uint8_t(size))!=size)return false;for(size_t i=0;i<size;++i)data[i]=Wire.read();return true;
  }
  void clear(){Wire.beginTransmission(address);Wire.write(uint8_t(0x81));Wire.write(uint8_t(0x4e));Wire.write(uint8_t(0));Wire.endTransmission();}

public:
  meshit::DeckCalibration calibration;
  bool begin(){pinMode(16,INPUT);for(uint8_t candidate:{uint8_t(0x5d),uint8_t(0x14)}){address=candidate;uint8_t id[4]={};if(read(0x8140,id,4)&&id[0]=='9'&&id[1]=='1'&&id[2]=='1'){clear();return true;}}address=0;return false;}
  meshit::DeckPointerEvent poll(bool calibrating=false){
    if(!address)return {};uint8_t status=0;
    if(!read(0x814e,&status,1)||!(status&0x80)){
      // Recover a lost release report instead of retaining the previous gesture axis.
      if(uint32_t(millis()-lastContact)>750){blocked=false;track=-1;return gesture.cancel();}return {};
    }
    const unsigned count=status&15;if(!count){clear();track=-1;if(blocked){blocked=false;return gesture.cancel();}return gesture.release();}
    uint8_t point[8]={};if(count!=1||!read(0x814f,point,sizeof(point))){clear();blocked=true;track=-1;return gesture.cancel();}if(blocked){clear();return {};}if(track>=0&&track!=point[0]){clear();blocked=true;track=-1;return gesture.cancel();}track=point[0];
    clear();const auto xy=meshit::deckTouchPoint(point[1]|(int(point[2])<<8),point[3]|(int(point[4])<<8),true);
    const auto position=calibrating?xy:calibration.map(xy.first,xy.second);
    lastContact=millis();return gesture.contact(position.first,position.second);
  }
};
#endif
