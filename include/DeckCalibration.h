#pragma once
#include <array>
#include <cmath>
#include <utility>
namespace meshit {
// Affine mapping of oriented GT911 coordinates into display pixels.
struct DeckCalibration {
  std::array<float,6> coefficients{{1,0,0,0,1,0}};
  static std::pair<int,int> target(size_t step){const std::pair<int,int> points[]={{24,24},{295,24},{24,215},{160,120}};return points[step%4];}
  std::pair<int,int> map(int x,int y)const{return {int(std::lround(coefficients[0]*x+coefficients[1]*y+coefficients[2])),int(std::lround(coefficients[3]*x+coefficients[4]*y+coefficients[5]))};}
  bool valid()const{for(float v:coefficients)if(!std::isfinite(v)||std::abs(v)>10000)return false;const float det=coefficients[0]*coefficients[4]-coefficients[1]*coefficients[3];return std::abs(det)>0.01f&&std::abs(det)<100;}
  bool fit(const std::array<std::pair<int,int>,3>& samples){
    const float x=samples[0].first,y=samples[0].second,dx=samples[1].first-x,dy=samples[1].second-y,ex=samples[2].first-x,ey=samples[2].second-y,det=dx*ey-dy*ex;
    if(std::abs(det)<1000)return false;
    DeckCalibration next;auto& c=next.coefficients;c[0]=271*ey/det;c[1]=-271*ex/det;c[2]=24-c[0]*x-c[1]*y;c[3]=-191*dy/det;c[4]=191*dx/det;c[5]=24-c[3]*x-c[4]*y;
    if(!next.valid())return false;*this=next;return true;
  }
  bool confirms(int x,int y)const{const auto point=map(x,y);return std::abs(point.first-160)<=18&&std::abs(point.second-120)<=18;}
};
}
