#pragma once
#include <cstdint>
namespace meshit {
enum class RadioPowerProfile {Sx1262,Gc1109,Kct8103l};
#if defined(MESHIT_HELTEC_V4_KCT8103L)
constexpr auto boardPowerProfile=RadioPowerProfile::Kct8103l;
#elif defined(MESHIT_HELTEC_V4)
constexpr auto boardPowerProfile=RadioPowerProfile::Gc1109;
#else
constexpr auto boardPowerProfile=RadioPowerProfile::Sx1262;
#endif
constexpr int maxTxPower(RadioPowerProfile profile){return profile==RadioPowerProfile::Sx1262?22:28;}
// Net FEM gain includes board attenuation and PA compression; indexed by SX1262 dBm.
// Reference: meshtastic/firmware src/configuration.h and variants/esp32s3/heltec_v4/variant.h.
constexpr int txOutputPower(RadioPowerProfile profile,int drive){
  constexpr int gc[]={11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,10,10,9,9,8,7};
  constexpr int kct[]={13,13,13,13,13,13,13,13,13,13,13,13,13,13,12,12,11,11,10,9,8,7};
  return drive+(profile==RadioPowerProfile::Sx1262?0:profile==RadioPowerProfile::Gc1109?gc[drive<0?0:drive]:kct[drive<0?0:drive]);
}
struct TxPower {int8_t drive=-9,output=-127;bool valid=false;};
// Maximize antenna-port output within the preset ceiling, using the lowest drive on PA plateaus.
constexpr TxPower selectTxPower(RadioPowerProfile profile,int ceiling){
  TxPower selected;const int maximum=maxTxPower(profile),maxDrive=profile==RadioPowerProfile::Sx1262?22:21;
  for(int drive=-9;drive<=maxDrive;++drive){const int output=txOutputPower(profile,drive);if(output<=ceiling&&output<=maximum&&output>selected.output)selected={int8_t(drive),int8_t(output),true};}
  return selected;
}
}
