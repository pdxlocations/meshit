#include <Core.h>
#include <CryptoBox.h>
#include <Carriers.h>
#include <ArduinoJson.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
using namespace meshit;
int main(){
  Core core;uint16_t sequence=0;core.allocate=[&](){return (Id(0x12345678)<<16)|++sequence;};std::mt19937 rng(17);core.jitter=[&](){return rng();};
  JsonDocument settings,presets;settings.to<JsonObject>();std::ifstream file("config/presets.json");deserializeJson(presets,file);uint32_t revision=1;
  auto start=std::chrono::steady_clock::now();std::vector<std::pair<uint32_t,Event>> echoes;
  std::string input;while(std::getline(std::cin,input)){
    uint32_t now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    for(auto it=echoes.begin();it!=echoes.end();)if(it->first<=now){core.receive(it->second,now,true);it=echoes.erase(it);++revision;}else ++it;
    if(settings["txEnabled"]|true)core.tick(now,[&](const Event& e,bool history){if(!history)echoes.push_back({now+1000,e});return true;});
    JsonDocument req,res;if(deserializeJson(req,input)){res["error"]="Invalid request";}
    else {res["id"]=req["id"];std::string op=req["op"]|"";
      if(op=="state"){res["node"]="12345678";res["revision"]=revision;res["count"]=core.messages.size();res["settings"]=settings;res["settings"]["txEnabled"]=settings["txEnabled"]|true;res["presets"]=presets;res["ready"]=settings["configured"]|false;res["board"]="Heltec V3";res["wifiSupported"]=true;res["storageReady"]=true;res["identityReady"]=true;res["demo"]=true;res["radioState"]="standby";}
      else if(op=="messages"){
        res["revision"]=revision;auto list=res["messages"].to<JsonArray>();size_t offset=req["offset"]|0;
        for(size_t i=offset;i<core.messages.size()&&i<offset+8;++i){auto& m=core.messages[i];auto& e=m.event;auto o=list.add<JsonObject>();o["id"]=hex(e.id);o["parent"]=e.ref?hex(e.ref):"";o["room"]=e.room;o["nickname"]=e.nick;o["text"]=e.text;o["order"]=m.order;o["mine"]=m.mine;o["acknowledged"]=m.acked;o["votes"]=core.score(e.id);o["voted"]=core.voted(e.id,0x12345678);o["status"]=m.acked?"peer confirmed":"sending";}
      }else if(op=="settings"){
        bool valid=false;for(JsonObjectConst p:presets.as<JsonArrayConst>())if(p["id"]==req["preset"]&&p["network"]==req["network"])valid=true;
        if(!valid)res["error"]="Choose a network and matching preset";
        else {if(settings["preset"]!=req["preset"]){core.messages.clear();core.pending.clear();core.votes.clear();}for(const char* key:{"network","preset","nickname","ssid"})if(req[key].is<const char*>())settings[key]=req[key];settings["configured"]=true;++revision;res["ok"]=true;}
      }else if(op=="txEnabled"){
        if(!req["enabled"].is<bool>())res["error"]="Transmit enabled must be true or false";
        else {settings["txEnabled"]=req["enabled"].as<bool>();++revision;res["ok"]=true;}
      }else if(op=="resend"){
      if(!(settings["txEnabled"]|true))res["error"]="Transmit is disabled";
      else if(!(settings["configured"]|false)||!core.resend(fromHex(req["target"]|""),now))res["error"]="Cannot resend: message unavailable, acknowledged, already queued, or radio not ready";
      else {res["ok"]=true;++revision;}
    }else if(op=="post"||op=="vote"){
        Event e;e.kind=op=="post"?Kind::Post:Kind::Vote;e.ref=fromHex(req[op=="post"?"parent":"target"]|"");e.room=req["room"].as<std::string>();e.text=req["text"].as<std::string>();e.nick=settings["nickname"]|"";e.up=req["up"]|true;
        if(!(settings["txEnabled"]|true))res["error"]="Transmit is disabled";else if(!(settings["configured"]|false))res["error"]="Choose a network first";else if(!core.submit(e,now))res["error"]="Invalid message or queue full";else {res["ok"]=true;res["messageId"]=hex(core.pending.back().event.id);++revision;}
      }else res["error"]="Unknown operation";
    }
    serializeJson(res,std::cout);std::cout<<std::endl;
  }
}
