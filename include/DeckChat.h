#pragma once
#include <Core.h>

namespace meshit {
enum DeckKey {DeckUp=256,DeckDown,DeckMenu,DeckBack,DeckLeft,DeckRight};
struct DeckPreset {std::string id,network,label;};
struct DeckAction {std::string op,text,room,network,preset;Id target=0;bool up=true;};
struct DeckRow {Id id;unsigned depth;};
// Hardware-independent chat interaction; actions go through the same RPC as companions.
class DeckChat {
public:
  enum Page {Chat,Menu,Rooms,NewRoom,Presets,Nickname,Credentials,Read,Settings};
  Page page=Chat;std::string room="general",draft,error;Id selected=0,reply=0;bool top=true,dark=false;size_t cursor=0;
  const std::vector<std::string> menu={"Reply","Toggle upvote","Resend","Rooms","New room","New post","New / Top","Radio preset","Nickname","Wi-Fi / Bluetooth","Read full message"};
  std::vector<std::string> rooms(const Core& core)const{
    std::vector<std::string> list{room};for(const auto& m:core.messages)if(std::find(list.begin(),list.end(),m.event.room)==list.end())list.push_back(m.event.room);
    std::sort(list.begin(),list.end());return list;
  }
  std::vector<DeckRow> rows(const Core& core)const{
    std::vector<DeckRow> out;std::vector<const Message*> ordered;
    for(const auto& m:core.messages)if(m.event.room==room)ordered.push_back(&m);
    std::sort(ordered.begin(),ordered.end(),[&](const Message* a,const Message* b){const int x=core.score(a->event.id),y=core.score(b->event.id);return top&&x!=y?x>y:a->order>b->order;});
    std::function<void(Id,unsigned)> append=[&](Id parent,unsigned depth){
      if(depth>=32)return;
      for(const auto* m:ordered)if(m->event.ref==parent){out.push_back({m->event.id,depth});append(m->event.id,depth+1);}
    };
    for(const auto* m:ordered)if(!m->event.ref||!core.find(m->event.ref)){out.push_back({m->event.id,0});append(m->event.id,1);}
    return out;
  }
  void sync(const Core& core){const auto list=rows(core);if(std::none_of(list.begin(),list.end(),[&](const DeckRow& r){return r.id==selected;}))selected=list.empty()?0:list.front().id;if(reply&&!core.find(reply))reply=0;}
  DeckAction key(int key,const Core& core,const std::vector<DeckPreset>& presets,uint32_t identity){
    sync(core);error.clear();DeckAction action;
    if(key==DeckLeft||key==DeckRight){page=page==Chat?(key==DeckLeft?Rooms:Settings):Chat;cursor=0;return action;}
    if(key==DeckMenu||key==12){if(page==NewRoom||page==Nickname)draft.clear();page=page==Menu?Chat:Menu;cursor=0;return action;}
    if(key==DeckBack||key==27){if(page==NewRoom||page==Nickname)draft.clear();page=Chat;reply=0;return action;}
    if(key==DeckUp||key==DeckDown){
      if(page==Read){if(key==DeckDown&&cursor<MAX_TEXT)++cursor;else if(key==DeckUp&&cursor)--cursor;return action;}
      const bool down=key==DeckDown;size_t count=page==Menu?menu.size():page==Rooms?rooms(core).size():page==Presets?presets.size():page==Settings?4:0;
      if(count){cursor=down?(cursor+1)%count:(cursor+count-1)%count;return action;}
      if(page==Chat){auto list=rows(core);for(size_t i=0;i<list.size();++i)if(list[i].id==selected){if(down&&i+1<list.size())selected=list[i+1].id;else if(!down&&i)selected=list[i-1].id;break;}}
      return action;
    }
    if(key=='\r'||key=='\n'){
      if(page==Settings){switch(cursor){case 0:page=Credentials;break;case 1:page=Presets;break;case 2:page=Nickname;draft.clear();break;case 3:dark=!dark;break;}cursor=0;return action;}
      if(page==Menu){
        switch(cursor){
          case 0:if(selected){reply=selected;page=Chat;}break;
          case 1:if(selected){action.op="vote";action.target=selected;action.up=!core.voted(selected,identity);page=Chat;}break;
          case 2:if(selected){action.op="resend";action.target=selected;page=Chat;}break;
          case 3:page=Rooms;break;case 4:page=NewRoom;draft.clear();break;
          case 5:page=Chat;reply=0;break;case 6:top=!top;page=Chat;break;
          case 7:page=Presets;break;case 8:page=Nickname;draft.clear();break;case 9:page=Credentials;break;case 10:if(selected)page=Read;break;
        }cursor=0;return action;
      }
      if(page==Rooms){auto list=rooms(core);if(cursor<list.size()){room=list[cursor];selected=reply=0;draft.clear();page=Chat;sync(core);}return action;}
      if(page==Presets){if(cursor<presets.size()){const auto& p=presets[cursor];action.op="settings";action.network=p.network;action.preset=p.id;}return action;}
      if(page==Credentials||page==Read){page=Chat;return action;}
      if(page==NewRoom){
        while(!draft.empty()&&draft.front()==' ')draft.erase(0,1);
        while(!draft.empty()&&draft.back()==' ')draft.pop_back();
        for(char& c:draft)if(c>='A'&&c<='Z')c+=32;
        if(textValid(draft,MAX_ROOM,MAX_ROOM)&&draft.find('\n')==std::string::npos){room=draft;draft.clear();selected=reply=0;page=Chat;}else error="Room: 1-16 UTF-8 bytes";return action;}
      if(page==Nickname){if(textValid(draft,MAX_NICK,MAX_NICK,true)){action.op="nickname";action.text=draft;}else error="Nickname: up to 16 bytes";return action;}
      if(page==Chat){if(draft.empty()){page=Menu;cursor=0;}else if(textValid(draft,MAX_TEXT,MAX_CHARS)){action.op="post";action.text=draft;action.room=room;action.target=reply;}else error="Use up to 160 characters";return action;}
    }
    if(page!=Chat&&page!=NewRoom&&page!=Nickname)return action;
    if(key==8||key==127){if(!draft.empty()){size_t at=draft.size()-1;while(at&&(uint8_t(draft[at])&0xc0)==0x80)--at;draft.erase(at);}return action;}
    if(key>=32&&key<127){auto next=draft+char(key);const size_t limit=page==Chat?MAX_CHARS:16;if(textValid(next,page==Chat?MAX_TEXT:16,limit))draft=next;}
    return action;
  }
  void accepted(const DeckAction& action,Id id=0){if(action.op=="post"){draft.clear();reply=0;if(id)selected=id;}else if(action.op=="settings"||action.op=="nickname"){draft.clear();page=Chat;}}
};
}
