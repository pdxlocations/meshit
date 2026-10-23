#pragma once
#if defined(MESHIT_TDECK_PLUS)
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "DeckScene.h"
class DeckSurface:public Adafruit_GFX {
  uint16_t* pixels=nullptr;uint16_t* previous=nullptr;bool firstFrame=true;Adafruit_ST7789* target=nullptr;
public:
  meshit::DeckBox clip{0,0,320,240};
  DeckSurface():Adafruit_GFX(320,240){}
  void begin(Adafruit_ST7789& display){target=&display;if(!pixels){pixels=static_cast<uint16_t*>(ps_malloc(320*240*2));if(!pixels)pixels=static_cast<uint16_t*>(malloc(320*240*2));if(pixels){previous=static_cast<uint16_t*>(ps_malloc(320*240*2));if(previous)memset(previous,0,320*240*2);}}}
  void drawPixel(int16_t x,int16_t y,uint16_t color)override{if(x<0||y<0||x>=320||y>=240||!clip.contains(x,y))return;if(pixels)pixels[y*320+x]=color;else if(target)target->drawPixel(x,y,color);}
  uint16_t pixel(int x,int y)const{return pixels&&x>=0&&y>=0&&x<320&&y<240?pixels[y*320+x]:0xffff;}
  void invalidate(){firstFrame=true;}
  void present(){
    if(!pixels||!target)return;if(!previous||firstFrame){target->drawRGBBitmap(0,0,pixels,320,240);if(previous)memcpy(previous,pixels,320*240*2);firstFrame=false;return;}
    int left=320,top=240,right=-1,bottom=-1;
    for(int y=0;y<240;++y)for(int x=0;x<320;++x)if(pixels[y*320+x]!=previous[y*320+x]){left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);}
    if(right<left)return;
    target->startWrite();target->setAddrWindow(left,top,right-left+1,bottom-top+1);
    for(int y=top;y<=bottom;++y){target->writePixels(pixels+y*320+left,right-left+1);memcpy(previous+y*320+left,pixels+y*320+left,(right-left+1)*2);}
    target->endWrite();
  }
};
class DeckGraphics {
  DeckSurface surface;
  static const GFXfont& font(meshit::DeckFont f){return f==meshit::DeckFont::Bold?FreeSansBold9pt7b:FreeSans9pt7b;}
  static std::string printable(const std::string& value){std::string out;for(unsigned char c:value){if(c>=32&&c<127)out+=char(c);else if(c>=0xc0)out+='?';else if(c=='\n')out+=' ';}return out;}
  void label(int x,int baseline,const std::string& value,meshit::DeckFont f,uint16_t color){
    const GFXfont& face=font(f);const GFXglyph* glyphs=reinterpret_cast<const GFXglyph*>(pgm_read_ptr(&face.glyph));const uint8_t* bitmap=reinterpret_cast<const uint8_t*>(pgm_read_ptr(&face.bitmap));
    const int num=f==meshit::DeckFont::Small?2:1,den=f==meshit::DeckFont::Small?3:1;
    for(unsigned char c:printable(value)){const GFXglyph* g=glyphs+c-32;
      const int w=pgm_read_byte(&g->width),h=pgm_read_byte(&g->height),xo=int8_t(pgm_read_byte(&g->xOffset)),yo=int8_t(pgm_read_byte(&g->yOffset));
      const int offset=pgm_read_word(&g->bitmapOffset);
      if(f==meshit::DeckFont::Small){
        auto floorDiv=[](int n,int d){return n>=0?n/d:-((-n+d-1)/d);};
        for(int py=floorDiv(yo*2,3);py<-floorDiv(-(yo+h)*2,3);++py)for(int px=floorDiv(xo*2,3);px<-floorDiv(-(xo+w)*2,3);++px){
          unsigned coverage=0;
          for(int dy:{1,3})for(int dx:{1,3}){const int sx=floorDiv((px*4+dx)*3,8)-xo,sy=floorDiv((py*4+dy)*3,8)-yo;
            if(sx>=0&&sy>=0&&sx<w&&sy<h){const int bit=sy*w+sx;if(pgm_read_byte(bitmap+offset+bit/8)&(0x80>>(bit%8)))++coverage;}}
          if(coverage){const uint16_t bg=surface.pixel(x+px,baseline+py);const unsigned r=(((color>>11)*coverage+(bg>>11)*(4-coverage)+2)/4),g=((((color>>5)&63)*coverage+((bg>>5)&63)*(4-coverage)+2)/4),b=(((color&31)*coverage+(bg&31)*(4-coverage)+2)/4);surface.drawPixel(x+px,baseline+py,(r<<11)|(g<<5)|b);}
        }
      }else for(int sy=0;sy<h;++sy)for(int sx=0;sx<w;++sx){const int bit=sy*w+sx;if(pgm_read_byte(bitmap+offset+bit/8)&(0x80>>(bit%8)))surface.drawPixel(x+xo+sx,baseline+yo+sy,color);}
      x+=pgm_read_byte(&g->xAdvance)*num/den;
    }
  }
public:
  void invalidate(){surface.invalidate();}
  static int measure(const std::string& value,meshit::DeckFont f){
    const auto& face=font(f);const auto* glyphs=reinterpret_cast<const GFXglyph*>(pgm_read_ptr(&face.glyph));int width=0;
    for(unsigned char c:printable(value)){const int advance=pgm_read_byte(&glyphs[c-32].xAdvance);width+=f==meshit::DeckFont::Small?advance*2/3:advance;}return width;
  }
  void draw(const meshit::DeckScene& scene,Adafruit_ST7789& display){
    surface.begin(display);
    for(const auto& op:scene.paint){surface.clip=op.clip;const auto b=op.box;const int x=b.x,y=b.y,w=b.w,h=b.h;const uint16_t c=op.color;
      using S=meshit::DeckShape;
      switch(op.shape){
        case S::Fill:surface.fillRect(x,y,w,h,c);break;
        case S::Round:surface.fillRoundRect(x,y,w,h,4,c);break;
        case S::Outline:surface.drawRoundRect(x,y,w,h,4,c);break;
        case S::Line:surface.drawLine(x,y,x+w,y+h,c);break;
        case S::Text:label(x,y,op.text,op.font,c);break;
        case S::Dot:if(op.text=="on")surface.fillCircle(x+w/2,y+h/2,w/2,c);else surface.drawCircle(x+w/2,y+h/2,w/2,c);break;
        case S::Chat:surface.drawRoundRect(x,y,w,h-3,3,c);surface.drawLine(x+4,y+h-4,x+1,y+h,c);surface.drawLine(x+1,y+h,x+8,y+h-4,c);break;
        case S::Gear:{
          // The same 24-unit outline used by the companion client's settings SVG.
          const float points[][2]={{9.5,3},{8.9,5.2},{6.9,6.1},{4.9,5.4},{2.5,9.6},{4.1,11.1},{4.1,13.4},{2.5,14.9},{4.9,19.1},{6.9,18.4},{8.9,19.3},{9.5,21.5},{14.5,21.5},{15.1,19.3},{17.1,18.4},{19.1,19.1},{21.5,14.9},{19.9,13.4},{19.9,11.1},{21.5,9.6},{19.1,5.4},{17.1,6.1},{15.1,5.2},{14.5,3}};
          for(size_t i=0;i<24;++i){const auto& a=points[i];const auto& b=points[(i+1)%24];surface.drawLine(x+int(a[0]*w/24),y+int(a[1]*h/24),x+int(b[0]*w/24),y+int(b[1]*h/24),c);}
          surface.drawCircle(x+w/2,y+h/2,std::max(1,w/8),c);break;}

        case S::Up:surface.drawLine(x+w/2,y+2,x+w/2,y+h,c);surface.drawLine(x+1,y+6,x+w/2,y+1,c);surface.drawLine(x+w/2,y+1,x+w-1,y+6,c);break;
        case S::Reply:surface.drawLine(x,y,x,y+h-3,c);surface.drawLine(x,y+h-3,x+w,y+h-3,c);surface.drawLine(x+w-3,y+h-6,x+w,y+h-3,c);surface.drawLine(x+w,y+h-3,x+w-3,y+h,c);break;
        case S::Check:surface.drawLine(x+1,y+7,x+5,y+11,c);surface.drawLine(x+5,y+11,x+12,y+2,c);surface.drawLine(x+1,y+8,x+5,y+12,c);surface.drawLine(x+5,y+12,x+13,y+2,c);break;
        case S::Retry:surface.drawCircle(x+7,y+7,5,c);surface.fillTriangle(x+11,y+1,x+14,y+6,x+8,y+6,c);break;
        case S::Pending:for(int i=0;i<3;++i)surface.fillCircle(x+2+i*5,y+8,1,c);break;
        case S::Poop:{const uint16_t brown=0x8ac7;surface.fillRoundRect(x+1,y+13,19,6,3,brown);surface.fillRoundRect(x+3,y+8,15,7,3,brown);surface.fillCircle(x+10,y+6,5,brown);surface.fillTriangle(x+8,y+5,x+9,y,x+14,y+7,brown);surface.fillCircle(x+7,y+12,2,0xffff);surface.fillCircle(x+13,y+12,2,0xffff);surface.drawPixel(x+7,y+12,0);surface.drawPixel(x+13,y+12,0);surface.drawLine(x+8,y+16,x+12,y+16,0xffff);break;}
      }
    }surface.present();
  }
};
#endif
