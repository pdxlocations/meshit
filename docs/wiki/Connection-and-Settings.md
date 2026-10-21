# Connect and configure

- **ESP:** read the `Meshit-…` setup Wi-Fi name and eight-digit password on the radio’s OLED. The password is a four-digit PIN repeated twice (for example, `48214821`). If the display is absent or cannot initialize, join the `Meshit-…` Wi-Fi network with **`12345678`**. Open **http://192.168.4.1**; no serial monitor is needed. Choose the network, region, and preset. In Settings, supply your home Wi-Fi details, then restart the radio and visit **http://meshit.local**. The setup hotspot closes after one minute on the LAN; the display then shows `meshit.local` and the LAN IP.
- **nRF or ESP:** use the companion's Bluetooth connection. The browser version can use Web Bluetooth in supported browsers; iPhone uses the native companion.
- **Mobile:** the UI is bundled locally, so it can open without an internet connection. Connect by Bluetooth or by local Wi-Fi on ESP. Local drafts and a bounded cached feed remain available while disconnected.

The initial preset catalog covers US/Canada and EU 868 MHz, with Meshtastic LongFast, MeshCore narrow, and the user-supplied **MeshOregon: 918.5 MHz, BW125, SF8, CR4/5**. Meshtastic transport uses the standard public key and the preset’s built-in channel hash automatically; no channel name is entered during setup. All Meshit rooms share one transport channel. Switching network or preset starts a fresh radio history.

## Setup credentials

Current Heltec V3 firmware initializes its 128×64 OLED at boot and shows the setup AP name, numeric password in large type, and setup URL. A detected, initialized display uses a four-digit PIN repeated twice, retained across restarts. Without a working supported display, the setup AP password is always **`12345678`**, including after an update from older firmware. Older saved alphanumeric passwords are replaced automatically. Home Wi-Fi credentials are unchanged.

If older firmware does not display setup details, install a build with on-screen setup support. RAK4631 has no Wi-Fi/AP; use its Bluetooth companion.

## Connect by Wi-Fi

1. Open the gear icon and choose **Wi-Fi (ESP32)** under **Connection type**.
2. Join the radio’s `Meshit-…` network in your phone or computer’s Wi-Fi settings, using the password on its display or `12345678` without a working display.
3. The native companion connects automatically at `192.168.4.1`. In a browser, open **http://192.168.4.1**. On a configured LAN, open **http://meshit.local** or the LAN IP shown on screen.

There is one Wi-Fi mode. The radio webpage connects to the radio serving that page. RAK4631 has no Wi-Fi.

## Connect by Bluetooth

1. Disconnect from the ESP32 radio’s setup Wi-Fi before using Bluetooth. While Wi-Fi clients are attached, ESP32 Bluetooth advertising is paused.
2. Select **Bluetooth (ESP32 / nRF52)** in Settings.
3. Choose **Find radios**, select your `Meshit-…` radio in **Available radios**, and choose **Connect**. Native companions scan for radios; supported browsers use the browser’s device chooser.
4. If ESP32 requests pairing, enter the **last six digits of its setup Wi-Fi password**. With the no-display password `12345678`, the Bluetooth PIN is **`345678`**. The OLED also shows the Bluetooth PIN while connected. RAK4631 uses its BLE UART connection without this ESP32 PIN.
5. Use **Disconnect** to end the session. You can select the same discovered radio and **Connect** again without scanning.

ESP32 pauses Wi-Fi during an authenticated Bluetooth connection and restores the setup hotspot when Bluetooth disconnects. Saved home Wi-Fi is rejoined as well; after one minute on the LAN, the setup hotspot closes normally. Messages remain on the radio. A disconnected companion keeps its cached feed and drafts but cannot send until reconnected. Connection type is saved locally; Bluetooth reconnection requires an explicit Connect action after reopening the app.

If an old ESP32 pairing stops working after a credential change, forget the radio in the operating system’s Bluetooth settings and pair again. Browser Bluetooth requires a supporting browser and secure context; use the native companion on iPhone.

## Radio settings

Open the **Settings** gear icon to change nickname, radio network, region, and preset. Nicknames are optional and fit 16 UTF-8 bytes; blank means Anonymous. Changing network or preset starts a fresh radio history. Meshtastic transport values come from the selected preset automatically; existing saved channel-name overrides are ignored and removed.

[Wiki home](Home.md)
