#pragma once
#include "Storage.h"
#include <Snapshot.h>
inline bool saveHistory(const meshit::Core& core){
#if defined(MESHIT_ESP32)
  auto f=LittleFS.open("/history.tmp","w");if(!f)return false;bool ok=meshit::writeSnapshot(f,core);f.flush();f.close();return ok&&LittleFS.rename("/history.tmp","/history.bin");
#else
  File f(MeshFS);MeshFS.remove("/history.tmp");if(!f.open("/history.tmp",FILE_O_WRITE))return false;bool ok=meshit::writeSnapshot(f,core);f.flush();f.close();if(!ok)return false;
  MeshFS.remove("/history.bak");if(MeshFS.exists("/history.bin")&&!MeshFS.rename("/history.bin","/history.bak"))return false;
  if(!MeshFS.rename("/history.tmp","/history.bin"))return false;MeshFS.remove("/history.bak");return true;
#endif
}
inline bool loadHistory(meshit::Core& core,uint32_t now){
#if defined(MESHIT_ESP32)
  auto f=LittleFS.open("/history.bin","r");if(!f)return false;return meshit::readSnapshot(f,core,now);
#else
  File f(MeshFS);const char* name=MeshFS.exists("/history.bin")?"/history.bin":"/history.bak";if(!f.open(name,FILE_O_READ))return false;return meshit::readSnapshot(f,core,now);
#endif
}
