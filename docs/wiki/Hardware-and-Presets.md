# Hardware and radio presets

| Board | Processor | Companion connection | Installation |
| --- | --- | --- | --- |
| Heltec WiFi LoRa 32 V3 | ESP32-S3 | Local Wi-Fi or Bluetooth | Browser USB installer |
| RAK4631 | nRF52840 | Bluetooth | Matching S140 UF2/HEX download |

Other boards have not been ported. Select hardware suitable for your local frequency band.

## Included presets

| Network | Region | Preset | MHz | BW kHz | SF | CR |
| --- | --- | --- | --- | --- | --- | --- |
| meshtastic | US | LongFast | 906.875 | 250 | 11 | 4/5 |
| meshtastic | US | MeshOregon | 918.5 | 125 | 8 | 4/5 |
| meshtastic | EU_868 | LongFast | 869.525 | 250 | 11 | 4/5 |
| meshcore | US | US / Canada narrow | 910.525 | 62.5 | 7 | 4/5 |
| meshcore | EU_868 | Europe narrow | 869.618 | 62.5 | 7 | 4/5 |

The preset catalog is [config/presets.json](../../config/presets.json). Match your nearby network's RF settings and transport channel. MeshOregon supplies RF settings only; its Meshtastic channel name defaults to `LongFast` and may need editing. Region selection must suit your deployment; these presets are not an exhaustive regional catalog.

Meshtastic uses its standard public transport key. MeshCore uses Meshit's built-in application group. Private transport-key entry and scope/transport-code routing are not implemented. Switching networks does not bridge traffic between them.

Next: [Installation](Installation.md) · [Connection and settings](Connection-and-Settings.md).

[Wiki home](Home.md)
