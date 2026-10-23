#pragma once
#include <Core.h>

namespace meshit {
enum DeckKey {DeckUp=256,DeckDown,DeckMenu,DeckBack,DeckLeft,DeckRight};
struct DeckPreset {std::string id,network,label,region,detail;};
struct DeckAction {std::string op,text,room,network,preset,ssid,password;Id target=0;bool up=true;};
struct DeckRow {Id id;unsigned depth;};
// Hardware-independent chat interaction; actions go through the same RPC as companions.
class DeckChat {
public:
  enum Page {Chat,Menu,Rooms,NewRoom,Presets,Nickname,Credentials,Read,Settings,Wifi,Ssid,WifiPassword,Appearance,Network,Regions};
  Page page=Chat;std::string room="general",draft,error;Id selected=0,reply=0;bool top=true,dark=false,focused=true,systemDark=false;int appearance=0;std::string network,region;size_t cursor=0;
  std::string savedDraft,savedSsid,savedNickname,wifiSsid,wifiPassword;Id savedReply=0;Page editReturn=Chat;
  bool isEditor()const{return page==NewRoom||page==Nickname||page==Ssid||page==WifiPassword;}
  void edit(Page next,const std::string& value,Page back){savedDraft=draft;savedReply=reply;draft=value;editReturn=back;page=next;cursor=0;focused=true;}
  void leaveEditor(){draft=savedDraft;reply=savedReply;page=editReturn;cursor=0;}
  void back(){error.clear();if(isEditor())leaveEditor();else{if(page==Presets&&!network.empty()){page=Regions;cursor=0;return;}if(page==Regions){page=Network;cursor=0;return;}page=page==Read||page==Menu||page==Rooms||page==Settings?Chat:Settings;cursor=0;}}
  void cancelReply(){reply=0;page=Chat;cursor=0;focused=false;error.clear();}
  bool hasBackRow()const{return page==Settings||page==Rooms||page==Presets||page==Appearance||page==Network||page==Regions||page==Wifi;}
  std::vector<DeckPreset> filtered(const std::vector<DeckPreset>& presets)const{std::vector<DeckPreset> out;for(const auto& p:presets)if((network.empty()||p.network==network)&&(region.empty()||p.region==region))out.push_back(p);return out;}
  std::vector<std::string> regions(const std::vector<DeckPreset>& presets)const{std::vector<std::string> out;for(const auto& p:presets)if(p.network==network&&std::find(out.begin(),out.end(),p.region)==out.end())out.push_back(p.region);return out;}
  std::vector<std::string> settingsItems()const{return {"Radio connection","Network","Nickname","Appearance","Local Wi-Fi"};}
  void swipe(int direction){if(!direction)return;if(isEditor())leaveEditor();error.clear();if(direction<0){if(page==Rooms)page=Chat;else page=Settings;}else{if(page==Chat)page=Rooms;else if(page!=Rooms)page=Chat;}cursor=0;}
  const std::vector<std::string> menu={"Back","Reply","Toggle upvote","Resend","Rooms","New room","New post","New / Top","Radio preset","Nickname","Wi-Fi / Bluetooth","Read full message"};
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
  std::vector<std::string> menuItems(const Core& core,uint32_t identity)const{
    auto items=menu;if(selected)items[2]=core.voted(selected,identity)?"Remove upvote":"Upvote";return items;
  }
  DeckAction key(int key,const Core& core,const std::vector<DeckPreset>& presets,uint32_t identity){
    sync(core);error.clear();DeckAction action;
    if(key==DeckLeft||key==DeckRight)return action;
    if(key==DeckMenu||key==12){if(isEditor())leaveEditor();page=page==Menu?Chat:Menu;cursor=0;return action;}
    if(key==DeckBack||key==27){if(page!=Chat)back();else cancelReply();return action;}
    if(key==DeckUp||key==DeckDown){
      if(page==Read){if(key==DeckDown&&cursor<MAX_TEXT)++cursor;else if(key==DeckUp&&cursor)--cursor;return action;}
      const bool down=key==DeckDown;size_t count=page==Menu?menu.size():page==Rooms?rooms(core).size():page==Presets?filtered(presets).size():page==Network?2:page==Regions?regions(presets).size():page==Appearance?3:page==Settings?settingsItems().size():page==Wifi?3:0;count+=hasBackRow()?1:0;
      if(count){cursor=down?(cursor+1)%count:(cursor+count-1)%count;return action;}
      if(page==Chat){auto list=rows(core);for(size_t i=0;i<list.size();++i)if(list[i].id==selected){if(down&&i+1<list.size())selected=list[i+1].id;else if(!down&&i)selected=list[i-1].id;break;}}
      return action;
    }
    if(key=='\r'||key=='\n'){
      if(hasBackRow()&&cursor==0){back();return action;}const size_t option=hasBackRow()?cursor-1:cursor;
      if(page==Settings){switch(option){case 0:page=Credentials;break;case 1:page=Network;break;case 2:edit(Nickname,savedNickname,Settings);break;case 3:page=Appearance;cursor=0;return action;case 4:wifiSsid=savedSsid;wifiPassword.clear();page=Wifi;break;}cursor=0;return action;}
      if(page==Appearance){appearance=int(option);dark=appearance==0?systemDark:appearance==2;action.op="deckAppearance";action.text=std::to_string(appearance);page=Settings;cursor=4;return action;}
      if(page==Network){network=option==0?"meshtastic":"meshcore";region.clear();page=Regions;cursor=0;return action;}
      if(page==Regions){auto list=regions(presets);if(option<list.size()){region=list[option];page=Presets;cursor=0;}return action;}
      if(page==Wifi){if(option==0)edit(Ssid,wifiSsid,Wifi);else if(option==1)edit(WifiPassword,wifiPassword,Wifi);else{action.op="wifi";action.ssid=wifiSsid;action.password=wifiPassword;}return action;}
      if(page==Menu){
        switch(cursor){
          case 0:page=Chat;break;
          case 1:if(selected){reply=selected;page=Chat;}break;
          case 2:if(selected){action.op="vote";action.target=selected;action.up=!core.voted(selected,identity);page=Chat;}break;
          case 3:if(selected){action.op="resend";action.target=selected;page=Chat;}break;
          case 4:page=Rooms;break;case 5:edit(NewRoom,"",Rooms);break;
          case 6:page=Chat;reply=0;break;case 7:top=!top;page=Chat;break;
          case 8:page=Network;break;case 9:edit(Nickname,savedNickname,Chat);break;case 10:page=Credentials;break;case 11:if(selected)page=Read;break;
        }cursor=0;return action;
      }
      if(page==Rooms){auto list=rooms(core);if(option<list.size()){room=list[option];selected=reply=0;draft.clear();page=Chat;sync(core);}return action;}
      if(page==Presets){auto list=filtered(presets);if(option<list.size()){const auto& p=list[option];action.op="settings";action.network=p.network;action.preset=p.id;}return action;}
      if(page==Credentials||page==Read){page=Chat;return action;}
      if(page==Ssid||page==WifiPassword){if(page==Ssid)wifiSsid=draft;else wifiPassword=draft;leaveEditor();return action;}
      if(page==NewRoom){
        while(!draft.empty()&&draft.front()==' ')draft.erase(0,1);
        while(!draft.empty()&&draft.back()==' ')draft.pop_back();
        for(char& c:draft)if(c>='A'&&c<='Z')c+=32;
        if(textValid(draft,MAX_ROOM,MAX_ROOM)&&draft.find('\n')==std::string::npos){room=draft;draft.clear();selected=reply=0;page=Chat;}else error="Room: 1-16 UTF-8 bytes";return action;}
      if(page==Nickname){if(textValid(draft,MAX_NICK,MAX_NICK,true)){action.op="nickname";action.text=draft;}else error="Nickname: up to 16 bytes";return action;}
      if(page==Chat){if(draft.empty()){page=Menu;cursor=0;}else if(textValid(draft,MAX_TEXT,MAX_CHARS)){action.op="post";action.text=draft;action.room=room;action.target=reply;}else error="Use up to 160 characters";return action;}
    }
    if(page!=Chat&&!isEditor())return action;
    focused=true;
    if(key==8||key==127){if(!draft.empty()){size_t at=draft.size()-1;while(at&&(uint8_t(draft[at])&0xc0)==0x80)--at;draft.erase(at);}return action;}
    if(key>=32&&key<127){auto next=draft+char(key);const size_t limit=page==Chat?MAX_CHARS:page==Ssid?32:page==WifiPassword?63:16;if(textValid(next,page==Chat?MAX_TEXT:limit,limit))draft=next;}
    return action;
  }
  void accepted(const DeckAction& action,Id id=0){if(action.op=="post"){draft.clear();reply=0;if(id)selected=id;}else if(action.op=="nickname"){savedNickname=action.text;leaveEditor();}else if(action.op=="settings"){page=Chat;}else if(action.op=="wifi"){savedSsid=action.ssid;page=Settings;wifiPassword.clear();}}
};
}
