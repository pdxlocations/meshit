#pragma once
#include "BatteryGauge.h"
namespace meshit {
// Shared by the TFT scene and text/e-paper headers; voltage bands are approximate.
template<class Surface> void drawDeckBattery(Surface& surface,int x,int y,int w,int h,int bars,uint16_t color){
  const int body=w-3;surface.drawRect(x,y,body,h,color);surface.fillRect(x+body,y+h/3,3,h/3,color);
  if(bars<0){
    const int center=x+body/2,top=y+(h-10)/2;
    surface.drawLine(center-2,top,center-2,top+3,color);
    surface.drawLine(center+2,top,center+2,top+3,color);
    surface.fillRect(center-4,top+3,9,4,color);
    surface.drawLine(center,top+7,center,top+9,color);return;
  }
  const int barWidth=(body-7)/4;
  for(int i=0;i<bars;++i)surface.fillRect(x+3+i*(barWidth+1),y+3,barWidth,h-6,color);
}
}
