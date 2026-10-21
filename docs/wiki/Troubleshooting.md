# Troubleshooting

| Symptom | What to check |
| --- | --- |
| No releases listed | Publish a release containing `meshit-release.zip`; enable **Show prereleases** for betas. PHP caches results for five minutes. |
| USB install unavailable | Use desktop Chrome/Edge, HTTPS, a data cable, and the Heltec target. Close other apps using the serial port. |
| RAK download blocked | Read the SoftDevice line in `INFO_UF2.TXT` and select exactly 6.1.1 or 7.3.0. Do not infer it from the bootloader version. |
| Firmware verification fails | Re-select the release and download again. Keep the board and SoftDevice matched; do not bypass validation. |
| ESP setup Wi-Fi missing | Read the boot log at 115200 baud. A successful LAN join closes the setup hotspot. |
| `meshit.local` does not resolve | Join the same LAN. If mDNS is unavailable, use the radio's LAN IP. Restart after saving Wi-Fi credentials. |
| Bluetooth unavailable in browser | Use a Web Bluetooth-capable browser or the native companion; iPhone uses the native app. A simulator cannot validate phone BLE. |
| Radio disconnected | Check power, reconnect in Settings, and confirm the selected Wi-Fi/Bluetooth connection. Saved messages can remain visible offline. |
| Connected but no messages | Confirm network, region, preset, and Meshtastic channel name match nearby Meshit users and relay infrastructure. Ordinary network chat is not displayed. |
| No rebroadcast check | No qualifying relayed echo was heard. This is not a read receipt; RF coverage and repeater behavior still require hardware checks. |
| Messages disappeared | Check for a network/preset change, fresh install, or the 200-message retention limit. Parent eviction also removes descendants. |
| Sending disabled | Complete setup, restore the radio connection, and check storage/identity warnings. Keep text within 160 characters. |
| Hosted page returns PHP source | Enable PHP 8.1+ with cURL, JSON, and ZipArchive. Static GitHub Pages cannot run the bridge. |

When [reporting a bug](https://github.com/pdxlocations/meshit/issues), include board, firmware version, installed RAK SoftDevice, network/preset, browser or phone OS, reproduction steps, and relevant logs. Remove Wi-Fi passwords and other private details.

See [installation](Installation.md), [connections](Connection-and-Settings.md), and [verification limits](../VALIDATION.md).

[Wiki home](Home.md)
