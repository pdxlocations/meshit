#pragma once
#if defined(MESHIT_TDECK)
#include <ArduinoJson.h>
#include "DeckChat.h"
#include "DeckRequest.h"
#include "DeckKeyboard.h"
#include "DeckPanel.h"
#include "UnreadMessages.h"
#include "DeckTouch.h"
#if defined(MESHIT_TDECK_PLUS)
#include "Storage.h"
#endif
class DeckUi {
  meshit::DeckChat chat;DeckKeyboard keyboard;bool keyboardReady=false,initialized=false;uint32_t next=0;
  std::vector<meshit::DeckPreset> catalog;
#if defined(MESHIT_TDECK_PLUS)
  DeckTouch touch;meshit::DeckScene scene;meshit::DeckScroll scroll;

#endif
  std::vector<meshit::DeckPreset> choices(JsonDocument& presets){
    std::vector<meshit::DeckPreset> result;
    for(JsonObjectConst p:presets.as<JsonArrayConst>())result.push_back({p["id"]|"",p["network"]|"",std::string(p["network"]|"")+" / "+(p["region"]|"")+" / "+(p["name"]|"")});
    return result;
  }
public:
  void begin(bool configured,bool dark=false){
    chat.dark=dark;
#if defined(MESHIT_TDECK_PLUS)
    touch.begin();
#endif
    keyboardReady=keyboard.begin();chat.page=configured?meshit::DeckChat::Chat:meshit::DeckChat::Presets;initialized=true;}
  void tick(meshit::Core& core,JsonDocument& settings,JsonDocument& presets,uint32_t identity,uint32_t& revision,
            UnreadMessages& unread,bool ready,bool bluetooth,uint16_t battery,const String& name,const String& password,
            const std::function<String(const String&)>& rpc){
    if(!initialized||int32_t(millis()-next)<0)return;next=millis()+20;
    if(catalog.empty())catalog=choices(presets);
    chat.savedSsid=settings["ssid"]|"";chat.savedNickname=settings["nickname"]|"";
    const auto previousPage=chat.page;const auto previousRoom=chat.room;
    int key=keyboard.read();meshit::DeckAction action;
#if defined(MESHIT_TDECK_PLUS)
    const auto event=touch.poll();
    if(event.pressed)scroll.stop(millis());
    if(event.dragY&&scene.viewport.contains(event.x,event.originY))scroll.drag(event.dragY,millis());
    if(event.released)scroll.release(millis());
    if(event.tap){if(const auto* hit=meshit::deckHitAt(scene,event.x,event.y))action=meshit::activateDeckHit(*hit,chat,core,catalog,identity);}
#endif
    if(key){action=chat.key(key,core,catalog,identity);
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
    chat.sync(core);
#if defined(MESHIT_TDECK_PLUS)
    if(chat.dark!=(settings["deckDark"]|false)){
      settings["deckDark"]=chat.dark;String saved;serializeJson(settings,saved);
      if(!writeFile("/settings.json",saved))chat.error="Could not save appearance";
    }
    if(!keyboardReady)chat.error="Keyboard unavailable";
    unread.prune(core);
    {scene=meshit::makeDeckScene(chat,core,catalog,ready,name.c_str(),password.c_str(),DeckGraphics::measure,scroll.tick(millis(),scene.maxScroll),battery?std::to_string(battery/1000)+"."+std::to_string((battery%1000)/100)+"V":"",unread.count(),scroll.followSelection);
    scroll.setPosition(scene.scroll);const uint32_t renderedRevision=revision;
    if(deckPanel().drawScene(scene)&&chat.page==meshit::DeckChat::Chat)unread.viewed(chat.room,renderedRevision,revision,core);
    return;}
#endif
    std::vector<std::string> lines;const auto page=chat.page;
    const size_t maxLines=deckPanel().lineCount();
    std::string batteryText=battery?std::to_string(battery/1000)+"."+std::to_string((battery%1000)/100)+"V":"--V";
    size_t outgoing=0;for(const auto& p:core.pending)if(!p.ack&&!p.history&&(p.event.kind==meshit::Kind::Post||p.event.kind==meshit::Kind::Vote))++outgoing;
    if(page==meshit::DeckChat::Credentials){
      lines={"meshit / connections",name.c_str(),"Wi-Fi: "+std::string(password.c_str()),"http://192.168.4.1","LAN: http://meshit.local","Bluetooth PIN:",password.substring(2).c_str(),"Out: "+std::to_string(outgoing)+" New: "+std::to_string(unread.count()),batteryText,"Enter: back"};
    }else if(page==meshit::DeckChat::Read){
      lines={"# "+chat.room};
      if(const auto* m=core.find(chat.selected)){
        lines.push_back(m->event.nick.empty()?"Anonymous":m->event.nick);
        std::vector<std::string> body;deckPanel().wrap(body,m->event.text);
        const size_t window=maxLines-3,start=std::min(chat.cursor,body.size()>window?body.size()-window:size_t(0));chat.cursor=start;
        for(size_t i=start;i<body.size()&&lines.size()<maxLines-1;++i)lines.push_back(body[i]);
      }else lines.push_back("Message no longer stored");
      lines.push_back("Up/down: scroll  Enter: back");
    }else if(page==meshit::DeckChat::Menu||page==meshit::DeckChat::Rooms||page==meshit::DeckChat::Presets){
      lines={page==meshit::DeckChat::Menu?"meshit / menu":page==meshit::DeckChat::Rooms?"meshit / rooms":"meshit / radio preset"};
      std::vector<std::string> items;if(page==meshit::DeckChat::Menu)items=chat.menu;else if(page==meshit::DeckChat::Rooms)items=chat.rooms(core);else for(const auto& p:catalog)items.push_back(p.label);
      const size_t window=maxLines-3,start=chat.cursor>=window?chat.cursor-window+1:0;
      for(size_t i=start;i<items.size()&&lines.size()<maxLines-2;++i)lines.push_back((chat.cursor==i?"> ":"  ")+items[i]);
#if defined(MESHIT_TDECK_PRO)
      if(page==meshit::DeckChat::Presets)lines.push_back("New preset clears history");
      lines.push_back("Alt+E/X: move; Enter: select");
#else
      if(page==meshit::DeckChat::Presets)lines.push_back("New preset clears history");
      lines.push_back("Ball: move; Enter: select");
#endif
    }else if(page==meshit::DeckChat::NewRoom||page==meshit::DeckChat::Nickname){
      lines={page==meshit::DeckChat::NewRoom?"meshit / new room":"meshit / nickname"};deckPanel().wrap(lines,chat.draft+"_");lines.push_back("Enter: save  Menu: cancel");
    }else {
      lines={"# "+chat.room+" / "+(chat.top?"Top":"New")+" / "+batteryText,
             std::string(ready?"Radio ready":(settings["configured"]|false)?"Radio unavailable":"Choose radio preset")+"  Out:"+std::to_string(outgoing)+" New:"+std::to_string(unread.count())+(bluetooth?" BT":"")};
      const auto list=chat.rows(core);size_t start=0;for(size_t i=0;i<list.size();++i)if(list[i].id==chat.selected){start=i;break;}
      if(list.empty())lines.push_back("No messages");
      const size_t bodyLimit=maxLines-4;
      for(size_t i=start;i<list.size()&&lines.size()<bodyLimit;++i){const auto* m=core.find(list[i].id);if(!m)continue;
        if(i==start&&m->event.ref){const auto* parent=core.find(m->event.ref);lines.push_back(parent?"Reply to: "+parent->event.text:"Parent requested...");}
        bool pending=false;for(const auto& p:core.pending)if(p.event.id==m->event.id&&!p.ack&&!p.history)pending=true;
        lines.push_back(std::string(i==start?"> ":"  ")+(m->event.nick.empty()?"Anonymous":m->event.nick)+" +"+std::to_string(core.score(m->event.id))+(m->mine?(m->acked?" [ok]":pending?" [...]":" [retry]"):""));
        std::vector<std::string> body;deckPanel().wrap(body,m->event.text,list[i].depth);
        for(const auto& line:body)if(lines.size()<bodyLimit)lines.push_back(line);
      }
      while(lines.size()<bodyLimit)lines.push_back("");
      lines.push_back(chat.reply?"Post a reply:":"Write something interesting...");
      std::vector<std::string> draft;deckPanel().wrap(draft,chat.draft+"_");
      // Keep the end of a long draft visible while typing.
      lines.push_back(draft.empty()?"_":draft.back());
#if defined(MESHIT_TDECK_PRO)
      lines.push_back("$: menu  Alt+E/X: move");
#else
      lines.push_back("Enter: post  Ball: menu");
#endif
    }
    if(!keyboardReady)chat.error="Keyboard unavailable; use companion";
    if(!chat.error.empty()){if(lines.size()>=maxLines)lines.resize(maxLines-1);lines.push_back(chat.error);}
    unread.prune(core);
    const uint32_t renderedRevision=revision;
    if(deckPanel().draw(lines)&&page==meshit::DeckChat::Chat){unread.viewed(chat.room,renderedRevision,revision,core);}
  }
};
#endif
