# Hardware and radio presets

| Board | Processor | Companion connection | Installation |
| --- | --- | --- | --- |
| Heltec WiFi LoRa 32 V3 | ESP32-S3 | Local Wi-Fi or Bluetooth | Browser USB installer |
| Heltec WiFi LoRa 32 V4.2 OLED / GC1109 | ESP32-S3 | Local Wi-Fi or Bluetooth | Browser USB installer (`heltec-v4`) |
| Heltec WiFi LoRa 32 V4.3 OLED / KCT8103L | ESP32-S3 | Local Wi-Fi or Bluetooth | Browser USB installer (`heltec-v4-3`) |
| LILYGO T-Deck | ESP32-S3 | Standalone + local Wi-Fi or Bluetooth | Browser USB installer (`t-deck`) |
| LILYGO T-Deck Plus | ESP32-S3 | Standalone + local Wi-Fi or Bluetooth | Browser USB installer (`t-deck-plus`) |
| LILYGO T-Deck Pro v1.0 | ESP32-S3 | Standalone + local Wi-Fi or Bluetooth | Browser USB installer (`t-deck-pro`) |
| LILYGO T-Deck Pro v1.1 | ESP32-S3 | Standalone + local Wi-Fi or Bluetooth | Browser USB installer (`t-deck-pro-v1-1`) |
| LILYGO T-Deck Pro Max | ESP32-S3 | Standalone + local Wi-Fi or Bluetooth | Browser USB installer (`t-deck-pro-max`) |
| Heltec Mesh Node T114 | nRF52840 | Bluetooth | S140 7.3.0 UF2/HEX (`heltec-t114`) |
| RAK4631 | nRF52840 | Bluetooth | Matching S140 UF2/HEX download |

Heltec V3/V4 OLED boards’ built-in 128×64 OLED shows setup Wi-Fi credentials and the setup URL. If the display is absent or initialization fails, its AP uses `12345678`. RAK4631 and T114 use Bluetooth and has no setup AP.

V4 profiles use 16 MiB flash; V3 uses 8 MiB. Match the V4 OLED revision/FEM to the [manufacturer’s guide](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/). The T114 TFT and optional V4 TFT are not initialized by these profiles. New board profiles are build-verified; physical radio, pairing, amplifier, battery, and display verification remains outstanding. [T-Deck models](T-Deck.md) use their TFT/e-paper screen and keyboard for standalone chat, with Wi-Fi/Bluetooth companions available too. The five new T-Deck profiles have passed syntax checks and host tests; firmware builds and physical-device verification are pending. Other boards have not been ported. Select hardware suitable for your local frequency band.

## Included presets

| Network | Region | Preset | MHz | BW kHz | SF | CR |
| --- | --- | --- | --- | --- | --- | --- |
| meshtastic | US | LongFast | 906.875 | 250 | 11 | 4/5 |
| meshtastic | US | MeshOregon | 918.5 | 125 | 8 | 4/5 |
| meshtastic | EU_868 | LongFast | 869.525 | 250 | 11 | 4/5 |
| meshtastic | EU_433 | LongFast | 433.875 | 250 | 11 | 4/5 |
| meshcore | US | US / Canada narrow | 910.525 | 62.5 | 7 | 4/5 |
| meshcore | EU_868 | Europe narrow | 869.618 | 62.5 | 7 | 4/5 |

The preset catalog is [config/presets.json](../../config/presets.json). Match your nearby network’s RF settings. Meshtastic transport settings are selected automatically from the preset; there is no channel-name field. Region selection must suit your deployment; these presets are not an exhaustive regional catalog.

Meshtastic uses its standard public transport key. MeshCore uses meshit's built-in application group. Private transport-key entry and scope/transport-code routing are not implemented. Switching networks does not bridge traffic between them.

Next: [Installation](Installation.md) · [Connection and settings](Connection-and-Settings.md).

[Wiki home](Home.md)

## Transmit power

Firmware selects the highest supported output within the preset ceiling: US 30 dBm, EU_868 27 dBm, and EU_433 10 dBm, following the [Meshtastic region definitions](https://github.com/meshtastic/firmware/blob/master/src/mesh/RadioInterface.cpp). Ordinary SX1262 targets (V3, all T-Decks, RAK4631, T114) use up to 22 dBm chip output. V4.2 and V4.3 use FEM gain curves to reach nominal 28 dBm antenna-port output in US presets, 27 dBm in EU_868, or 10 dBm in EU_433. Chip drive stays within the PA curve's supported range. Both V4 FEMs switch their transmit-control pin with TX/RX. All SX1262 targets use a 140 mA PA current limit.

References: [Semtech SX1262](https://www.semtech.com/products/wireless-rf/lora-connect/sx1262), [Heltec V4 specifications](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/), [FEM gain curves](https://github.com/meshtastic/firmware/blob/master/src/configuration.h), and [V4 FEM pin controls](https://github.com/meshtastic/firmware/blob/master/variants/esp32s3/heltec_v4/variant.h). Values are configured estimates; actual output requires RF measurement and depends on board revision, supply, and frequency.
