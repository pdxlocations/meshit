#pragma once
#if defined(MESHIT_TDECK)
#include <ArduinoJson.h>
#include "DeckChat.h"
#include "DeckRequest.h"
#include "DeckKeyboard.h"
#include "DeckPanel.h"
#include "DeckScreenIdle.h"
#include "UnreadMessages.h"
#include "RadioStats.h"
#include "DeckTouch.h"
#if defined(MESHIT_TDECK_PLUS)
#include "Storage.h"
#endif
class DeckUi {
  meshit::DeckChat chat;DeckKeyboard keyboard;bool keyboardReady=false,initialized=false;uint32_t next=0;
  meshit::DeckScreenIdle idle;bool touchDown=false;uint32_t seenActivity=0;
  bool screenInput(bool active,uint32_t activity){
    const bool traffic=activity!=seenActivity;seenActivity=activity;
    const auto state=idle.update(millis(),active,traffic);
    if(state.sleep||state.wake)deckPanel().setSleeping(state.sleep);
#if defined(MESHIT_TDECK_PLUS)
    if(state.sleep||state.wake){tapLatch=meshit::DeckTapLatch();scroll.stop(millis());hasRendered=false;}
#endif
    if(!state.allowInput)keyboard.cancelCenter();
    return state.allowInput;
  }
  meshit::DeckTextScroll textScroll;
  uint32_t syncedRevision=0;bool hasSynced=false;
  std::vector<meshit::DeckPreset> catalog;
#if defined(MESHIT_TDECK_PLUS)
  DeckTouch touch;meshit::DeckTapLatch tapLatch;meshit::DeckScene scene;meshit::DeckScroll scroll;
  std::array<std::pair<int,int>,3> calibrationSamples{};meshit::DeckCalibration pendingCalibration;
  struct RenderState {
    uint32_t revision=0;uint16_t battery=0;size_t unread=0,cursor=0;
    bool largeText=false;int page=0,appearance=0,scroll=0;meshit::RadioActivity activity=meshit::RadioActivity::Standby;meshit::Id selected=0,reply=0;
    bool ready=false,bluetooth=false,keyboardReady=false,top=false,dark=false,focused=false,systemDark=false,followSelection=false,transmitEnabled=true,bluetoothEnabled=false,wifiEnabled=false;
    std::string room,draft,error,network,region,wifiSsid,wifiPassword,savedNickname,name,password;
    bool operator==(const RenderState& other)const{
      return largeText==other.largeText&&bluetoothEnabled==other.bluetoothEnabled&&wifiEnabled==other.wifiEnabled&&activity==other.activity&&transmitEnabled==other.transmitEnabled&&followSelection==other.followSelection&&revision==other.revision&&battery==other.battery&&unread==other.unread&&cursor==other.cursor&&page==other.page&&appearance==other.appearance&&scroll==other.scroll&&selected==other.selected&&reply==other.reply&&ready==other.ready&&bluetooth==other.bluetooth&&keyboardReady==other.keyboardReady&&top==other.top&&dark==other.dark&&focused==other.focused&&systemDark==other.systemDark&&room==other.room&&draft==other.draft&&error==other.error&&network==other.network&&region==other.region&&wifiSsid==other.wifiSsid&&wifiPassword==other.wifiPassword&&savedNickname==other.savedNickname&&name==other.name&&password==other.password;
    }
  } renderedState;
  bool hasRendered=false;
#endif
  std::vector<meshit::DeckPreset> choices(JsonDocument& presets){
    std::vector<meshit::DeckPreset> result;
    for(JsonObjectConst p:presets.as<JsonArrayConst>())result.push_back({p["id"]|"",p["network"]|"",std::string(p["network"]|"")+" / "+(p["region"]|"")+" / "+(p["name"]|""),p["region"]|"",std::string(String(p["frequency"].as<float>(),2).c_str())+" MHz / SF"+std::to_string(p["sf"].as<int>())+" / BW"+std::string(String(p["bandwidth"].as<float>(),1).c_str())+" / CR4/"+std::to_string(p["cr"].as<int>())});
    return result;
  }
public:
  void begin(bool configured,bool dark=false,int appearance=-1){
    hasSynced=false;idle.begin(millis());touchDown=false;
    chat.systemDark=false;chat.appearance=appearance>=0&&appearance<=2?appearance:dark?2:0;chat.dark=chat.appearance==2;
#if defined(MESHIT_TDECK_PLUS)
    chat.touchAvailable=touch.begin();
    {JsonDocument stored;if(!deserializeJson(stored,readFile("/touch-calibration.json"))&&stored["version"]==1&&stored["coefficients"].is<JsonArray>()&&stored["coefficients"].size()==6){
      meshit::DeckCalibration saved;bool numeric=true;for(size_t i=0;i<6;++i){numeric=numeric&&stored["coefficients"][i].is<float>();saved.coefficients[i]=stored["coefficients"][i].as<float>();}
      if(numeric&&saved.valid())touch.calibration=saved;
    }}
    hasRendered=false;
#endif
    keyboardReady=keyboard.begin();chat.page=configured?meshit::DeckChat::Chat:meshit::DeckChat::Presets;initialized=true;}
  void tick(meshit::Core& core,JsonDocument& settings,JsonDocument& presets,uint32_t identity,uint32_t& revision,
            UnreadMessages& unread,bool ready,bool bluetooth,uint16_t battery,const String& name,const String& password,
            const std::function<String(const String&)>& rpc,meshit::RadioActivity activity=meshit::RadioActivity::Standby,bool pairing=false){
    if(!initialized||int32_t(millis()-next)<0)return;next=millis()+20;
    if(catalog.empty())catalog=choices(presets);
    chat.largeText=settings["deckLargeText"]|false;chat.transmitEnabled=settings["txEnabled"]|true;chat.bluetoothEnabled=settings["deckBluetoothEnabled"]|false;chat.wifiEnabled=settings["deckWifiEnabled"]|false;
    chat.savedSsid=settings["ssid"]|"";chat.savedNickname=settings["nickname"]|"";
    const auto previousPage=chat.page;const auto previousRoom=chat.room;
    int key=keyboard.read();meshit::DeckAction action;bool touchHandled=false;
#if defined(MESHIT_TDECK_PLUS)
    const bool calibrating=chat.page==meshit::DeckChat::Calibrate;
    const auto event=touch.poll(calibrating);
    if(event.pressed)touchDown=true;if(event.released)touchDown=false;
    if(!screenInput(keyboard.active()||key||touchDown||event.pressed||event.released||event.dragY,core.messageActivity))return;
    if(pairing){hasRendered=false;return;}
    if(calibrating){
      touchHandled=event.pressed||event.released;
      if(event.tap){
        if(chat.cursor<3){
          calibrationSamples[chat.cursor++]={event.x,event.y};
          if(chat.cursor==3&&!pendingCalibration.fit(calibrationSamples)){chat.cursor=0;chat.error="Targets too close. Try again.";}
        }else if(!pendingCalibration.confirms(event.x,event.y)){chat.cursor=0;chat.error="Verification failed. Try again.";}
        else {
          JsonDocument stored;stored["version"]=1;auto values=stored["coefficients"].to<JsonArray>();for(float value:pendingCalibration.coefficients)values.add(value);
          String serialized;serializeJson(stored,serialized);
          if(writeFile("/touch-calibration.json",serialized)){touch.calibration=pendingCalibration;chat.page=meshit::DeckChat::Settings;chat.cursor=5;chat.error="Touch calibration saved";}
          else chat.error="Could not save. Tap to retry.";
        }
      }
    }else{
    if(event.pressed){scroll.stop(millis());tapLatch.begin(scene,chat,event.x,event.y);}
    const int drag=meshit::deckTouchScrollDelta(chat,scene,event);if(drag)scroll.drag(drag,millis());
    // Page navigation uses explicit buttons; horizontal finger drift must never open Rooms.
    if(event.released)scroll.release(millis());
    if(event.released){if(const auto* hit=tapLatch.finish(chat,event.tap)){action=meshit::activateDeckHit(*hit,chat,core,catalog,identity);if(hit->action==meshit::DeckTap::Cancel)scroll.reset(millis());touchHandled=true;}}
    }
#endif
#if !defined(MESHIT_TDECK_PLUS)
    if(!screenInput(keyboard.active()||key,core.messageActivity))return;
    if(pairing)return;
#endif
#if !defined(MESHIT_TDECK_PLUS)
    if(chat.page==meshit::DeckChat::Chat&&(key==meshit::DeckUp||key==meshit::DeckDown)){textScroll.move(key==meshit::DeckUp?-1:1);key=0;}
#endif
    if(key&&!touchHandled){action=chat.key(key,core,catalog,identity);
#if defined(MESHIT_TDECK_PLUS)
      if(key==meshit::DeckUp||key==meshit::DeckDown){if(chat.page==meshit::DeckChat::Read)scroll.move(key==meshit::DeckDown?28:-28,millis());else scroll.select(millis());}
#endif
    }
#if defined(MESHIT_TDECK_PLUS)
    if(chat.page!=previousPage||chat.room!=previousRoom)scroll.reset(millis());
#endif
    {
      if(!action.op.empty()){
        JsonDocument req,res;meshit::deckRequest(req,action);
        String input;serializeJson(req,input);
        if(deserializeJson(res,rpc(input)))chat.error="Radio response unavailable";
        else if(res["error"].is<const char*>())chat.error=res["error"].as<std::string>();
        else {chat.accepted(action,meshit::fromHex(res["messageId"]|""));
#if defined(MESHIT_TDECK_PLUS)
          if(action.op=="post")scroll.select(millis());
#endif
          if(action.op=="wifi")chat.error="Restart to join saved Wi-Fi.";
        }
      }
    }
#if !defined(MESHIT_TDECK_PLUS)
    if(chat.room!=previousRoom)textScroll.reset();else if(chat.page!=previousPage||action.op=="post")textScroll.select();
#endif
    deckPanel().setLargeText(chat.largeText);
    if(!hasSynced||syncedRevision!=revision){chat.sync(core);syncedRevision=revision;hasSynced=true;}
#if defined(MESHIT_TDECK_PLUS)
    if(!keyboardReady)chat.error="Keyboard unavailable";
    unread.prune(core);
    const int requestedScroll=scroll.tick(millis(),scene.maxScroll);
    RenderState current;current.activity=activity;current.revision=revision;current.battery=plausibleBatteryVoltage(battery)?batteryBars(battery)+1:0;current.unread=unread.count();
    current.largeText=chat.largeText;current.cursor=chat.cursor;current.page=chat.page;current.appearance=chat.appearance;current.scroll=requestedScroll;
    current.selected=chat.selected;current.reply=chat.reply;current.ready=ready;current.bluetooth=bluetooth;
    current.keyboardReady=keyboardReady;current.top=chat.top;current.dark=chat.dark;current.focused=chat.focused;
    current.bluetoothEnabled=chat.bluetoothEnabled;current.wifiEnabled=chat.wifiEnabled;current.transmitEnabled=chat.transmitEnabled;current.followSelection=scroll.followSelection;current.systemDark=chat.systemDark;current.room=chat.room;current.draft=chat.draft;current.error=chat.error;
    current.network=chat.network;current.region=chat.region;current.wifiSsid=chat.wifiSsid;
    current.wifiPassword=chat.wifiPassword;current.savedNickname=chat.savedNickname;
    current.name=name.c_str();current.password=password.c_str();
    if(hasRendered&&current==renderedState)return;
    {scene=meshit::makeDeckScene(chat,core,catalog,ready,name.c_str(),password.c_str(),DeckGraphics::measure,requestedScroll,battery,unread.count(),scroll.followSelection,[&](const std::string& room){return unread.count(room,core);},identity,activity);
    scroll.setPosition(scene.scroll);const uint32_t renderedRevision=revision;
    const bool painted=deckPanel().drawScene(scene);if(painted){renderedState=std::move(current);hasRendered=true;}
    if(painted&&chat.page==meshit::DeckChat::Chat&&renderedRevision==revision){std::vector<meshit::Id> visible;for(const auto& hit:scene.hits)if(hit.action==meshit::DeckTap::Select&&hit.box.h>=40)visible.push_back(hit.target);unread.viewedIds(chat.room,visible,core);}
    else if(painted&&chat.page==meshit::DeckChat::Read&&renderedRevision==revision)unread.viewedIds(chat.room,{chat.selected},core);
    return;}
#endif
    std::vector<std::string> lines;std::vector<meshit::Id> visible;const auto page=chat.page;
    const size_t maxLines=deckPanel().lineCount();
    std::string batteryText=battery?std::to_string(battery/1000)+"."+std::to_string((battery%1000)/100)+"V":"--V";
    const size_t outgoing=meshit::outgoingCount(core);
    if(page==meshit::DeckChat::Credentials){
      lines={"meshit / connections",name.c_str(),"Wi-Fi: "+std::string(password.c_str()),"http://192.168.4.1","LAN: http://meshit.local","Bluetooth PIN:",password.substring(2).c_str(),"Out: "+std::to_string(outgoing)+" New: "+std::to_string(unread.count()),batteryText,"Enter: back"};
    }else if(page==meshit::DeckChat::Read){
      lines={"# "+chat.room};
      if(const auto* m=core.find(chat.selected)){visible.push_back(m->event.id);
        lines.push_back(m->event.nick.empty()?"Anonymous":m->event.nick);
        std::vector<std::string> body;deckPanel().wrap(body,m->event.text);
        const size_t window=maxLines-3,start=std::min(chat.cursor,body.size()>window?body.size()-window:size_t(0));chat.cursor=start;
        for(size_t i=start;i<body.size()&&lines.size()<maxLines-1;++i)lines.push_back(body[i]);
      }else lines.push_back("Message no longer stored");
      lines.push_back("Up/down: scroll  Enter: back");
    }else if(page==meshit::DeckChat::Navigation||page==meshit::DeckChat::Menu||page==meshit::DeckChat::Rooms||page==meshit::DeckChat::Presets||page==meshit::DeckChat::Settings||page==meshit::DeckChat::Appearance||page==meshit::DeckChat::Network||page==meshit::DeckChat::Regions||page==meshit::DeckChat::Wifi){
      lines={page==meshit::DeckChat::Navigation?"meshit / navigation":page==meshit::DeckChat::Menu?"meshit / menu":page==meshit::DeckChat::Rooms?"meshit / rooms":page==meshit::DeckChat::Settings?"meshit / settings":page==meshit::DeckChat::Appearance?"meshit / appearance":page==meshit::DeckChat::Network?"meshit / network":page==meshit::DeckChat::Regions?"meshit / region":page==meshit::DeckChat::Wifi?"meshit / local Wi-Fi":"meshit / radio preset"};
      std::vector<std::string> items;if(page==meshit::DeckChat::Navigation)items=chat.navigationItems;else if(page==meshit::DeckChat::Menu)items=chat.menuItems(core,identity);else if(page==meshit::DeckChat::Rooms){items=chat.rooms(core);for(auto& room:items){const auto fresh=unread.count(room,core);room+=" ("+std::to_string(meshit::roomMessageCount(core,room))+")"+(fresh?" / "+std::to_string(fresh)+" new":"");}}else if(page==meshit::DeckChat::Settings)items=chat.settingsItems();else if(page==meshit::DeckChat::Appearance)items={"System","Light","Dark"};else if(page==meshit::DeckChat::Network)items={"Meshtastic","MeshCore"};else if(page==meshit::DeckChat::Regions)items=chat.regions(catalog);else if(page==meshit::DeckChat::Wifi)items={"Network: "+chat.wifiSsid,"Password: keep saved","Save settings"};else for(const auto& p:chat.filtered(catalog))items.push_back(p.label);
      if(chat.hasBackRow())items.insert(items.begin(),"Back");
      const size_t window=maxLines-3,start=chat.cursor>=window?chat.cursor-window+1:0;
      for(size_t i=start;i<items.size()&&lines.size()<maxLines-2;++i)lines.push_back((chat.cursor==i?"> ":"  ")+items[i]);
#if defined(MESHIT_TDECK_PRO)
      if(page==meshit::DeckChat::Presets)lines.push_back("New preset clears history");
      lines.push_back("Alt+E/X: move; Enter: select");
#else
      if(page==meshit::DeckChat::Presets)lines.push_back("New preset clears history");
      lines.push_back("Ball: move; Enter: select");
#endif
    }else if(chat.isEditor()){
      lines={page==meshit::DeckChat::NewRoom?"meshit / new room":page==meshit::DeckChat::Nickname?"meshit / nickname":page==meshit::DeckChat::Ssid?"meshit / network name":"meshit / Wi-Fi password"};deckPanel().wrap(lines,(page==meshit::DeckChat::WifiPassword?std::string(chat.draft.size(),'*'):chat.draft)+"_");lines.push_back("Enter: save  Menu: cancel");
    }else {
      lines={"# "+chat.room+" / "+(chat.top?"Top":"New"),
             std::string(ready?(activity==meshit::RadioActivity::Tx?"Radio TX ^":activity==meshit::RadioActivity::Rx?"Radio RX v":"Radio standby"):(settings["configured"]|false)?"Radio unavailable":"Choose radio preset")+"  Out:"+std::to_string(outgoing)+" New:"+std::to_string(unread.count())+(bluetooth?" BT":"")};
      const auto list=chat.rows(core);const size_t bodyLimit=maxLines-4;
      std::vector<std::string> feed;std::vector<meshit::Id> owners;int selectedTop=0,selectedHeight=0;
      for(const auto& row:list){const auto* m=core.find(row.id);if(!m)continue;const int top=feed.size();
        bool pending=false;for(const auto& job:core.pending)if(job.event.id==row.id&&!job.ack&&!job.history)pending=true;
        feed.push_back(std::string(row.id==chat.selected?"> ":"  ")+(m->event.nick.empty()?"Anonymous":m->event.nick)+" "+std::to_string(core.score(row.id))+(m->mine?(m->acked?" [ok]":pending?" [...]":" [retry]"):""));
        deckPanel().wrap(feed,m->event.text,row.depth);while(owners.size()<feed.size())owners.push_back(row.id);
        if(row.id==chat.selected){selectedTop=top;selectedHeight=int(feed.size())-top;}
      }
      const int viewport=std::max(1,int(bodyLimit)-int(lines.size()));
      const int first=textScroll.layout(feed.size(),viewport,selectedTop,selectedHeight);
      if(!textScroll.followsSelection()&&!owners.empty())chat.selected=owners[first];
      if(feed.empty())lines.push_back("No messages");
      for(int i=first;i<int(feed.size())&&lines.size()<bodyLimit;++i){lines.push_back(feed[i]);if(std::find(visible.begin(),visible.end(),owners[i])==visible.end())visible.push_back(owners[i]);}
      while(lines.size()<bodyLimit)lines.push_back("");
      lines.push_back(chat.reply?"Post a reply:":"Write something interesting...");
      std::vector<std::string> draft;deckPanel().wrap(draft,chat.draft+"_");
      // Keep the end of a long draft visible while typing.
      lines.push_back(draft.empty()?"_":draft.back());
#if defined(MESHIT_TDECK_PRO)
      lines.push_back("$: menu  Alt+E/X: move");
#else
      lines.push_back("Enter / ball press: select or post");
#endif
    }
    if(!keyboardReady)chat.error="Keyboard unavailable; use companion";
    if(!chat.error.empty()){if(lines.size()>=maxLines)lines.resize(maxLines-1);lines.push_back(chat.error);}
    unread.prune(core);
    const uint32_t renderedRevision=revision;
    if(deckPanel().draw(lines,false,battery)&&(page==meshit::DeckChat::Chat||page==meshit::DeckChat::Read)&&renderedRevision==revision){unread.viewedIds(chat.room,visible,core);}
  }
};
#endif
