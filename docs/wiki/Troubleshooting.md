# Troubleshooting

| Symptom | What to check |
| --- | --- |
| Release tag creation fails with HTTP 403 after a push | Older workflows created their tag after the build, when its commit might no longer be a branch head. Current workflows reserve the exact tag before building. For an old failed run, restore its missing tag at the recorded `RELEASE_COMMIT`, create a draft prerelease for that existing tag if the old release-creation step also receives a 403, then rerun only the failed publishing job; do not move the tag to newer code. |
| No releases listed | Publish a release containing `meshit-release.zip`; enable **Show prereleases** for betas. PHP caches results for five minutes. |
| USB install unavailable | Use desktop Chrome/Edge, HTTPS, a data cable, and the Heltec target. Close other apps using the serial port. |
| RAK download blocked | Read the SoftDevice line in `INFO_UF2.TXT` and select exactly 6.1.1 or 7.3.0. Do not infer it from the bootloader version. |
| Firmware verification fails | Re-select the release and download again. Keep the board and SoftDevice matched; do not bypass validation. |
| ESP setup Wi-Fi missing | Read the AP name on the OLED or look for `mesit-…` in Wi-Fi settings. After one minute on your LAN, the setup hotspot closes; use `meshit.local` or the LAN IP shown on screen. Restart to reopen setup Wi-Fi. |
| No display / setup password unknown | With current firmware, use `12345678` when the supported display is absent or cannot initialize. A working OLED shows its eight-digit password. Update older firmware for this setup flow; a serial monitor is not required. |
| `meshit.local` does not resolve | Join the same LAN. If mDNS is unavailable, use the radio's LAN IP. Restart after saving Wi-Fi credentials. |
| Bluetooth unavailable in browser | Use a Web Bluetooth-capable browser or the native companion; iPhone uses the native app. A simulator cannot validate phone BLE. |
| Radio disconnected | In Settings select Wi-Fi (ESP32) and join mesit-… in device Wi-Fi settings, or select Bluetooth, Find radios, choose a radio, and Connect. Saved messages remain visible offline. |
| ESP32 Bluetooth not found / will not pair | Disconnect clients from the setup Wi-Fi first. ESP32 Bluetooth is hidden while Wi-Fi clients are attached. Its pairing PIN is the last 6 digits of the setup Wi-Fi password (345678 for the no-display fallback). Remove an old OS bond if firmware credentials changed. |
| Connected but no messages | Confirm network, region, and preset match nearby meshit users and relay infrastructure. Ordinary network chat is not displayed. |
| No rebroadcast check | No qualifying relayed echo was heard. This is not a read receipt; RF coverage and repeater behavior still require hardware checks. |
| Messages disappeared | Check for a network/preset change, fresh install, or the 200-message retention limit. Parent eviction also removes descendants. |
| Sending disabled | Complete setup, restore the radio connection, and check storage/identity warnings. Keep text within 160 characters. |
| Heltec reports “Radio storage is unavailable” | Older firmware used the wrong storage partition name. Update the existing installation with firmware containing the partition-name fix, then restart. Use an app-only update; a fresh install erases saved settings and messages. If the warning persists, report it before erasing the radio. |
| Hosted page returns PHP source | Enable PHP 8.1+ with cURL, JSON, and ZipArchive. Static GitHub Pages cannot run the bridge. |

When [reporting a bug](https://github.com/pdxlocations/meshit/issues), include board, firmware version, installed RAK SoftDevice, network/preset, browser or phone OS, reproduction steps, and relevant logs. Remove Wi-Fi passwords and other private details.

See [installation](Installation.md), [connections](Connection-and-Settings.md), and [verification limits](../VALIDATION.md).

[Wiki home](Home.md)
