#pragma once
#include <ArduinoJson.h>
#include "DeckChat.h"
namespace meshit {
inline void deckRequest(JsonDocument& req,const DeckAction& action){
  req.clear();req["op"]=action.op=="nickname"||action.op=="wifi"?"settings":action.op;
  if(action.op=="post"){req["room"]=action.room;req["text"]=action.text;req["parent"]=action.target?hex(action.target):"";}
  else if(action.op=="vote"||action.op=="resend"){req["target"]=hex(action.target);if(action.op=="vote")req["up"]=action.up;}
  else if(action.op=="settings"){req["network"]=action.network;req["preset"]=action.preset;}
  else if(action.op=="deckAppearance")req["appearance"]=action.text=="2"?2:action.text=="1"?1:0;
  else if(action.op=="nickname")req["nickname"]=action.text;
  else if(action.op=="wifi"){req["ssid"]=action.ssid;if(!action.password.empty())req["password"]=action.password;}
}
}
