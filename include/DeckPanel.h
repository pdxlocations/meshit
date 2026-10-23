#pragma once
#if defined(MESHIT_TDECK)
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <vector>
#include <string>
#include "../firmware/esp32/Pins.h"
#if defined(MESHIT_TDECK_PRO)
#include <GxEPD2_BW.h>
#else
#include <Adafruit_ST7789.h>
#endif

class DeckPanel {
#if defined(MESHIT_TDECK_PRO)
  GxEPD2_BW<GxEPD2_310_GDEQ031T10,GxEPD2_310_GDEQ031T10::HEIGHT> screen{GxEPD2_310_GDEQ031T10(DECK_DISPLAY_CS,35,DECK_DISPLAY_RESET,37)};
  uint32_t frames=0;
#else
  Adafruit_ST7789 screen{&SPI,DECK_DISPLAY_CS,11,-1};
#endif
  bool ready=false;uint32_t last=0;std::string previous;
public:
  bool begin(){
#if defined(MESHIT_TDECK_PRO)
    screen.epd2.selectSPI(SPI,SPISettings(2000000,MSBFIRST,SPI_MODE0));
    screen.init(0,true,2,false);screen.setRotation(0);
#else
    screen.init(240,320);screen.setSPISpeed(20000000);
#if defined(MESHIT_TDECK_PLUS)
    // LILYGO's newer Plus panel needs different ST7789 power/gamma settings.
    const uint8_t b2[]={0x0c,0x0c,0x00,0x33,0x33},b7[]={0x75},bb[]={0x1a},c0[]={0x2c},c2[]={1},c3[]={0x13},c4[]={0x20},c6[]={0x0f},d0[]={0xa4,0xa1},d6[]={0xa1};
    const uint8_t e0[]={0xd0,0x0d,0x14,0x0d,0x0d,0x09,0x38,0x44,0x4e,0x3a,0x17,0x18,0x2f,0x30};
    const uint8_t e1[]={0xd0,0x09,0x0f,0x08,0x07,0x14,0x37,0x44,0x4d,0x38,0x15,0x16,0x2c,0x3e};
    screen.sendCommand(0xb2,b2,sizeof(b2));screen.sendCommand(0xb7,b7,1);screen.sendCommand(0xbb,bb,1);
    screen.sendCommand(0xc0,c0,1);screen.sendCommand(0xc2,c2,1);screen.sendCommand(0xc3,c3,1);screen.sendCommand(0xc4,c4,1);
    screen.sendCommand(0xc6,c6,1);screen.sendCommand(0xd0,d0,2);screen.sendCommand(0xd6,d6,1);screen.sendCommand(0xe0,e0,14);screen.sendCommand(0xe1,e1,14);
#endif
    screen.setRotation(1);screen.invertDisplay(true);
    pinMode(42,OUTPUT);digitalWrite(42,LOW);delay(4);digitalWrite(42,HIGH);
#endif
    screen.setFont(&FreeSans9pt7b);screen.setTextWrap(false);ready=true;return true;
  }
  void serviceWhileBusy(void (*callback)(const void*)){ 
#if defined(MESHIT_TDECK_PRO)
    screen.epd2.setBusyCallback(callback);
#endif
  }
  int width()const{return screen.width();}
  size_t lineCount()const{return (screen.height()-8)/20;}
  void wrap(std::vector<std::string>& lines,const std::string& value,unsigned depth=0){
    std::string text=printable(value),prefix(std::min(depth,4u)*2,' ');
    while(!text.empty()){
      size_t length=0;int16_t x,y;uint16_t w,h;
      while(length<text.size()){screen.getTextBounds((prefix+text.substr(0,length+1)).c_str(),0,0,&x,&y,&w,&h);if(w>screen.width()-8&&length)break;++length;}
      if(length<text.size()){const size_t space=text.rfind(' ',length-1);if(space!=std::string::npos&&space)length=space;}
      lines.push_back(prefix+text.substr(0,length));text.erase(0,length);while(!text.empty()&&text.front()==' ')text.erase(0,1);
    }
  }
  // All glyphs on the device use the sans-serif font; unsupported Unicode is
  // replaced only for display. The stored/transmitted UTF-8 remains untouched.
  static std::string printable(const std::string& value){std::string out;for(unsigned char c:value){if(c>=32&&c<127)out+=char(c);else if(c>=0xc0)out+='?';else if(c=='\n')out+=' ';}return out;}
  bool draw(const std::vector<std::string>& lines,bool force=false){
    if(!ready)return false;
    std::string signature;for(const auto& line:lines){signature+=line;signature+='\n';}
    if(signature==previous)return true;
#if defined(MESHIT_TDECK_PRO)
    if(!force&&uint32_t(millis()-last)<750)return false;
    if(frames++%30==0)screen.setFullWindow();else screen.setPartialWindow(0,0,screen.width(),screen.height());
    screen.firstPage();do {
#endif
      screen.fillScreen(0xffff);screen.setTextColor(0);screen.setFont(&FreeSans9pt7b);
      for(size_t i=0;i<lines.size()&&i<lineCount();++i){
        std::string text=printable(lines[i]);int16_t x,y;uint16_t w,h;
        do {screen.getTextBounds(text.c_str(),0,0,&x,&y,&w,&h);if(w>screen.width()-8&&!text.empty())text.pop_back();else break;}while(true);
        screen.setCursor(4,18+i*20);screen.print(text.c_str());
      }
#if defined(MESHIT_TDECK_PRO)
    }while(screen.nextPage());
#endif
    previous=signature;last=millis();return true;
  }
};
inline DeckPanel& deckPanel(){static DeckPanel panel;return panel;}
#endif
