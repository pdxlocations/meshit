# Install firmware

Use **[meshit.pdxlocs.com](https://meshit.pdxlocs.com)** for firmware installation and verified downloads. Published builds come from [GitHub Releases](https://github.com/pdxlocations/meshit/releases); enable **Show prereleases** for betas. If no builds are listed, a release containing `meshit-release.zip` must be published first.

## Heltec WiFi LoRa 32 V3 / V4 OLED

1. Open the flasher in desktop Chrome or Edge over HTTPS. Use a USB data cable and close serial monitors using the port.
2. Choose a release and the exact **Heltec WiFi LoRa 32 V3**, **V4.2 OLED / GC1109**, or **V4.3 OLED / KCT8103L** target. Check the PCB revision before selecting V4.
3. Choose **Update** for a compatible meshit installation, or **First installation / start fresh** for a new installation.
4. Confirm the board and data replacement when applicable, then choose **Download & verify firmware**.
5. Choose **Connect USB & install**, select the radio, and complete installation.

After restart, read the setup AP name and eight-digit numeric password on the OLED, join that network, and open **http://192.168.4.1**. On first setup without a working display, join `meshit-…` with **`12345678`**. No serial monitor is needed with current firmware. See [connection and settings](Connection-and-Settings.md) for details.

Update writes the application at `0x10000`. Preserve data only with a matching board/partition layout and **Erase device unchecked**. Fresh installation writes a complete 8 MiB (V3) or 16 MiB (V4) image at `0x0` and replaces saved data regardless of the erase checkbox. The companion website is embedded; no filesystem upload is needed.

## RAK4631 / T114

1. Double-press Reset to mount the bootloader drive.
2. Read the **SoftDevice** line in `INFO_UF2.TXT`. The bootloader version is a different number.
3. Select the matching target below in the flasher, confirm the installed SoftDevice, and download the verified UF2.
4. Copy that UF2 to the bootloader drive. After restart, [connect over Bluetooth](Connection-and-Settings.md).

| Installed SoftDevice | Flasher target | Application file |
| --- | --- | --- |
| S140 6.1.1 | RAK4631 / stock S140 6.1.1 | `meshit-rak4631-s140-6-1-1.uf2` |
| S140 7.3.0 | RAK4631 / S140 7.3.0 | `meshit-rak4631.uf2` |
| S140 7.3.0 | Heltec T114 / S140 7.3.0 | `meshit-heltec-t114.uf2` |

Stop if the SoftDevice is unknown or unsupported. These are application-only images and do not replace the bootloader or SoftDevice. HEX downloads are for programming tools that preserve both. RAK4631 and T114 use downloads and drag-and-drop installation, not the ESP USB installer.

Next: [Connect and configure](Connection-and-Settings.md). See [flasher details](../../webflasher/README.md) for validation and packaging behavior.


## LILYGO T-Deck family

Choose the exact T-Deck model from the ESP32 installer: T-Deck, Plus, Pro v1.0, Pro v1.1, or Pro Max. Audio and cellular options share their revision’s firmware profile. A published release containing the new targets is required before the flasher lists them. Use an application-only update to preserve existing meshit settings; replacing another firmware generally requires a fresh installation.

The device screen and keyboard can configure the radio and run chat without a phone. Bluetooth and Wi-Fi default to Off. Enable the desired connection in on-device **Settings**, then open **Radio connection** for the AP name, password, URL, and pairing PIN. See [T-Deck controls and hardware revisions](T-Deck.md).

[Wiki home](Home.md)
