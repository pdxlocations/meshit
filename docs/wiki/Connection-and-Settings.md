# Connect and configure

- **ESP:** read the `Meshit-…` setup Wi-Fi name and eight-digit password on the radio’s OLED. The password is a four-digit PIN repeated twice (for example, `48214821`). If the display is absent or cannot initialize, join the `Meshit-…` Wi-Fi network with **`12345678`**. Open **http://192.168.4.1**; no serial monitor is needed. Choose the network, region, and preset. In Settings, supply your home Wi-Fi details, then restart the radio and visit **http://meshit.local**. The setup hotspot closes after one minute on the LAN; the display then shows `meshit.local` and the LAN IP.
- **nRF or ESP:** use the companion's Bluetooth connection. The browser version can use Web Bluetooth in supported browsers; iPhone uses the native companion.
- **Mobile:** the UI is bundled locally, so it can open without an internet connection. Connect by Bluetooth or by local Wi-Fi on ESP. Local drafts and a bounded cached feed remain available while disconnected.

The initial preset catalog covers US/Canada and EU 868 MHz, with Meshtastic LongFast, MeshCore narrow, and the user-supplied **MeshOregon: 918.5 MHz, BW125, SF8, CR4/5**. Meshtastic transport uses the standard public channel key; its channel name is editable. MeshOregon defaults to `LongFast` for that transport name because only its RF settings were supplied; match the local network's channel name when needed. All Meshit rooms share one transport channel. Switching network or preset starts a fresh radio history.

## Setup credentials

Current Heltec V3 firmware initializes its 128×64 OLED at boot and shows the setup AP name, numeric password in large type, and setup URL. A detected, initialized display uses a four-digit PIN repeated twice, retained across restarts. Without a working supported display, the setup AP password is always **`12345678`**, including after an update from older firmware. Older saved alphanumeric passwords are replaced automatically. Home Wi-Fi credentials are unchanged.

If older firmware does not display setup details, install a build with on-screen setup support. RAK4631 has no Wi-Fi/AP; use its Bluetooth companion.

## Settings

Open the gear icon and choose **Appearance → Light, Dark, or System**. Appearance changes apply immediately and do not change radio settings or history.

Open the **Settings** gear icon to change the connection, nickname, radio network, region, and preset. Nicknames are optional and fit 16 UTF-8 bytes; blank means Anonymous. Changing the network or preset starts a fresh radio history.

For a second ESP on the LAN, select **Another radio on Wi-Fi** and enter its HTTP address. In a browser, HTTP RPC must be same-origin; use the radio's own webpage for local Wi-Fi control. Native companions provide the local HTTP adapter.

![Meshit connection, identity, and radio settings](../screenshots/settings.png)

Screenshot from the local simulator; no Wi-Fi credentials are shown. The hosted flasher installs firmware; the radio's local page or mobile companion is where you chat.

[Wiki home](Home.md)
