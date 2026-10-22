# Hardware and radio presets

| Board | Processor | Companion connection | Installation |
| --- | --- | --- | --- |
| Heltec WiFi LoRa 32 V3 | ESP32-S3 | Local Wi-Fi or Bluetooth | Browser USB installer |
| Heltec WiFi LoRa 32 V4.2 OLED / GC1109 | ESP32-S3 | Local Wi-Fi or Bluetooth | Browser USB installer (`heltec-v4`) |
| Heltec WiFi LoRa 32 V4.3 OLED / KCT8103L | ESP32-S3 | Local Wi-Fi or Bluetooth | Browser USB installer (`heltec-v4-3`) |
| Heltec Mesh Node T114 | nRF52840 | Bluetooth | S140 7.3.0 UF2/HEX (`heltec-t114`) |
| RAK4631 | nRF52840 | Bluetooth | Matching S140 UF2/HEX download |

Heltec V3/V4 OLED boards’ built-in 128×64 OLED shows setup Wi-Fi credentials and the setup URL. If the display is absent or initialization fails, its AP uses `12345678`. RAK4631 and T114 use Bluetooth and has no setup AP.

V4 profiles use 16 MiB flash; V3 uses 8 MiB. Match the V4 OLED revision/FEM to the [manufacturer’s guide](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/). The T114 TFT and optional V4 TFT are not initialized by these profiles. New board profiles are build-verified; physical radio, pairing, amplifier, battery, and display verification remains outstanding. Other boards have not been ported. Select hardware suitable for your local frequency band.

## Included presets

| Network | Region | Preset | MHz | BW kHz | SF | CR |
| --- | --- | --- | --- | --- | --- | --- |
| meshtastic | US | LongFast | 906.875 | 250 | 11 | 4/5 |
| meshtastic | US | MeshOregon | 918.5 | 125 | 8 | 4/5 |
| meshtastic | EU_868 | LongFast | 869.525 | 250 | 11 | 4/5 |
| meshcore | US | US / Canada narrow | 910.525 | 62.5 | 7 | 4/5 |
| meshcore | EU_868 | Europe narrow | 869.618 | 62.5 | 7 | 4/5 |

The preset catalog is [config/presets.json](../../config/presets.json). Match your nearby network’s RF settings. Meshtastic transport settings are selected automatically from the preset; there is no channel-name field. Region selection must suit your deployment; these presets are not an exhaustive regional catalog.

Meshtastic uses its standard public transport key. MeshCore uses meshit's built-in application group. Private transport-key entry and scope/transport-code routing are not implemented. Switching networks does not bridge traffic between them.

Next: [Installation](Installation.md) · [Connection and settings](Connection-and-Settings.md).

[Wiki home](Home.md)
