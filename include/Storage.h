#pragma once
#include <Arduino.h>
#if defined(MESHIT_ESP32)
#include <LittleFS.h>
inline bool storageBegin(){return LittleFS.begin(true);}
inline String readFile(const char* name){auto f=LittleFS.open(name,"r");return f?f.readString():String();}
inline bool writeFile(const char* name,const String& value){
  String tmp=String(name)+".tmp";auto f=LittleFS.open(tmp,"w");if(!f)return false;bool ok=f.print(value)==value.length();f.flush();f.close();return ok&&LittleFS.rename(tmp,name);
}
#else
#include <InternalFileSystem.h>
#include <MeshStorage.h>
using namespace Adafruit_LittleFS_Namespace;
inline bool storageBegin(){return MeshFS.begin();}
inline String readFile(const char* name){File f(MeshFS);if(!f.open(name,FILE_O_READ))return {};String s=f.readString();f.close();return s;}
inline bool writeFile(const char* name,const String& value){
  // Keep a previous copy during replacement; recover it on read at boot.
  String tmp=String(name)+".tmp",bak=String(name)+".bak";File f(MeshFS);MeshFS.remove(tmp.c_str());if(!f.open(tmp.c_str(),FILE_O_WRITE))return false;
  bool ok=f.print(value)==value.length();f.flush();f.close();if(!ok)return false;
  MeshFS.remove(bak.c_str());if(MeshFS.exists(name)&&!MeshFS.rename(name,bak.c_str()))return false;
  if(!MeshFS.rename(tmp.c_str(),name))return false;MeshFS.remove(bak.c_str());return true;
}
#endif
inline String recoverFile(const char* name){String s=readFile(name);if(!s.length())s=readFile((String(name)+".bak").c_str());return s;}
