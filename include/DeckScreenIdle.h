#pragma once
#include <cstdint>
namespace meshit {
// Display-only sleep: radio servicing continues while input is polled.
class DeckScreenIdle {
  uint32_t lastActivity=0,quietSince=0;bool sleeping=false,swallowing=false;
public:
  struct Update {bool sleep=false,wake=false,allowInput=false;};
  void begin(uint32_t now){lastActivity=quietSince=now;sleeping=swallowing=false;}
  Update update(uint32_t now,bool active,bool traffic=false){
    Update result;
    if(traffic){lastActivity=now;if(sleeping){sleeping=false;result.wake=true;if(active){swallowing=true;quietSince=now;}}}
    if(active){lastActivity=quietSince=now;if(sleeping){sleeping=false;swallowing=true;result.wake=true;}}
    if(swallowing&&!active&&uint32_t(now-quietSince)>=200)swallowing=false;
    if(!sleeping&&!swallowing&&!active&&uint32_t(now-lastActivity)>=60000){sleeping=true;result.sleep=true;}
    result.allowInput=!sleeping&&!swallowing;return result;
  }
};
}
