#pragma once
#if defined(MESHIT_TDECK)
#include "DeckHardware.h"
#include "DeckChat.h"
#if defined(MESHIT_TDECK_PRO)
#include <Adafruit_TCA8418.h>
#endif
class DeckKeyboard {
  uint32_t next=0;bool boot=false;
#if defined(MESHIT_TDECK_PRO)
  Adafruit_TCA8418 keypad;bool ready=false;bool shiftLeft=false,shiftRight=false,symbol=false,alt=false;
#else
  bool ball[4]={false,false,false,false};uint32_t lastBall=0;
#endif
public:
  bool begin(){
#if defined(MESHIT_TDECK_PRO)
    ready=keypad.begin(0x34,&Wire);if(ready){keypad.matrix(4,10);keypad.flush();}return ready;
#else
    const int pins[]={3,15,1,2};for(unsigned i=0;i<4;++i){pinMode(pins[i],INPUT_PULLUP);ball[i]=digitalRead(pins[i]);}
    Wire.beginTransmission(0x55);return Wire.endTransmission()==0;
#endif
  }
  int read(){
    const bool pressed=digitalRead(0)==LOW;
    if(pressed&&!boot){boot=true;return meshit::DeckMenu;}boot=pressed;
    if(int32_t(millis()-next)<0)return 0;next=millis()+10;
#if defined(MESHIT_TDECK_PRO)
    if(!ready)return 0;
    // TCA8418 key numbers follow its fixed 10-column FIFO scan order.
    for(unsigned n=0;n<10&&keypad.available();++n){
      const uint8_t event=keypad.getEvent(),number=event&0x7f;const bool down=event&0x80;
      if(number==31){shiftRight=down;continue;}if(number==35){shiftLeft=down;continue;}
      if(number==32){symbol=down;continue;}if(number==30){alt=down;continue;}
      if(!down||!number||number>35)continue;
      // Explicit physical rows avoid relying on a C string containing modifiers.
      const char row1[]="poiuytrewq",row2[]={'\b','l','k','j','h','g','f','d','s','a'},row3[]={'\r','$','m','n','b','v','c','x','z',0};
      char c=number<=10?row1[number-1]:number<=20?row2[number-11]:number<=30?row3[number-21]:number==33?' ':0;
      if(alt){if(c=='e')return meshit::DeckUp;if(c=='x')return meshit::DeckDown;if(c=='q')return meshit::DeckBack;if(c=='t')return meshit::DeckMenu;}
      if(c=='$')return meshit::DeckMenu;
      if(symbol){const char symbols1[]="@+-_)(321#",symbols2[]={'\b','"','\'',';',':','/','6','5','4','*'},symbols3[]={'\r','$','.',',','!','?','9','8','7',0};c=number<=10?symbols1[number-1]:number<=20?symbols2[number-11]:number<=30?symbols3[number-21]:number==34?'0':c;}
      if((shiftLeft||shiftRight)&&c>='a'&&c<='z')c-=32;
      if(c)return c;
    }return 0;
#else
    const int pins[]={3,15,1,2};
    for(unsigned i=0;i<4;++i){const bool value=digitalRead(pins[i]);const bool moved=value!=ball[i];ball[i]=value;if(i<2&&moved&&uint32_t(millis()-lastBall)>90){lastBall=millis();return i==0?meshit::DeckUp:meshit::DeckDown;}}
    Wire.requestFrom(uint8_t(0x55),uint8_t(1));if(Wire.available())return Wire.read();return 0;
#endif
  }
};
#endif
