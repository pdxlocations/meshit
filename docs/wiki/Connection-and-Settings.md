# Connect and configure

- **ESP:** read the `mesit-…` setup Wi-Fi name and eight-digit password on the radio’s OLED. The password is a four-digit PIN repeated twice (for example, `48214821`). On first setup without a working display, join the `mesit-…` Wi-Fi network with **`12345678`**. Open **http://192.168.4.1**; no serial monitor is needed. Choose the network, region, and preset. In Settings, supply your home Wi-Fi details, then restart the radio and visit **http://meshit.local**. The setup hotspot closes after one minute on the LAN; the display then shows `meshit.local` and the LAN IP.
- **nRF or ESP:** use the companion's Bluetooth connection. The browser version can use Web Bluetooth in supported browsers; iPhone uses the native companion.
- **Mobile:** the UI is bundled locally, so it can open without an internet connection. Connect by Bluetooth or by local Wi-Fi on ESP. Local drafts and a bounded cached feed remain available while disconnected.

The initial preset catalog covers US/Canada and EU 868 MHz, with Meshtastic LongFast, MeshCore narrow, and the user-supplied **MeshOregon: 918.5 MHz, BW125, SF8, CR4/5**. Meshtastic transport uses the standard public key and the preset’s built-in channel hash automatically; no channel name is entered during setup. All meshit rooms share one transport channel. Switching network or preset starts a fresh radio history.

## Setup credentials

Current Heltec V3/V4 firmware initializes its 128×64 OLED at boot and shows the setup AP name, numeric password in large type, and setup URL. AP and Bluetooth names use `mesit-` followed by the last four hexadecimal digits of the device’s factory MAC address, including leading zeroes (for example, `mesit-00af`). ESP radios use the factory base MAC for both names; nRF52 radios use the factory Bluetooth address. The name does not depend on message identity or saved settings.

On first setup, radios with a working display use a four-digit PIN repeated twice; headless radios use **`12345678`**. Existing eight-digit numeric passwords are retained across restarts and application firmware updates, even if display detection changes. ESP32’s Bluetooth PIN remains the last six digits of that saved password. New display-equipped radios derive their initial PIN from the hardware MAC suffix rather than a random boot value. Home Wi-Fi credentials remain saved. Use an application-only update to retain settings; a factory/full-erase installation removes saved credentials, including previously generated random passwords. The switch to MAC-based names changes the network name once, so select the new name after this update.

If older firmware does not display setup details, install a build with on-screen setup support. RAK4631 and T114 have no Wi-Fi/AP; use their Bluetooth companion.

## Connect by Wi-Fi

1. Open the gear icon and choose **Wi-Fi (ESP32)** under **Connection type**.
2. Join the radio’s `mesit-…` network in your phone or computer’s Wi-Fi settings, using the password on its display or the initial no-display password `12345678`.
3. The native companion connects automatically at `192.168.4.1`. In a browser, open **http://192.168.4.1**. On a configured LAN, open **http://meshit.local** or the LAN IP shown on screen.

There is one Wi-Fi mode. The radio webpage connects to the radio serving that page. RAK4631 and T114 have no Wi-Fi.

## Connect by Bluetooth

1. Disconnect from the ESP32 radio’s setup Wi-Fi before using Bluetooth. While Wi-Fi clients are attached, ESP32 Bluetooth advertising is paused.
2. Select **Bluetooth (ESP32 / nRF52)** in Settings.
3. Choose **Find radios**, select your `mesit-…` radio in **Available radios**, and choose **Connect**. Native companions scan for radios; supported browsers use the browser’s device chooser.
4. If ESP32 requests pairing, enter the **last six digits of its setup Wi-Fi password**. With the no-display password `12345678`, the Bluetooth PIN is **`345678`**. The OLED shows the Bluetooth PIN while pairing. RAK4631 and T114 use their BLE UART connections without this ESP32 PIN.
5. Use **Disconnect** to end the session. You can select the same discovered radio and **Connect** again without scanning.

ESP32 pauses Wi-Fi during an authenticated Bluetooth connection and restores the setup hotspot when Bluetooth disconnects. Saved home Wi-Fi is rejoined as well; after one minute on the LAN, the setup hotspot closes normally. Messages remain on the radio. A disconnected companion keeps its cached feed and drafts but cannot send until reconnected. Connection type is saved locally; Bluetooth reconnection requires an explicit Connect action after reopening the app.

If an old ESP32 pairing stops working after a credential change, forget the radio in the operating system’s Bluetooth settings and pair again. Browser Bluetooth requires a supporting browser and secure context; use the native companion on iPhone.

## Radio settings

Open the **Settings** gear icon to change nickname, radio network, region, and preset. Nicknames are optional and fit 16 UTF-8 bytes; blank means Anonymous. Changing network or preset starts a fresh radio history. Meshtastic transport values come from the selected preset automatically; existing saved channel-name overrides are ignored and removed.


## Heltec V3/V4 OLED display

During Bluetooth pairing, the LCD shows the six-digit PIN before authentication finishes. Enter it on your phone. Once Bluetooth is authenticated or a web companion is actively polling over Wi-Fi, the LCD shows battery bars and voltage, outgoing posts/votes awaiting a rebroadcast acknowledgment, and unread incoming messages. Opening a room in the client clears its new-message count on the LCD after the current messages load. Other rooms stay unread, and background polling or viewing Settings does not clear them. Retransmissions do not add to the new-message count. The footer shows the configured frequency and connection type appears at the top. No node count is displayed. Battery bars are a coarse voltage estimate; an unavailable or implausible reading shows `?` / `--V`.

After the client disconnects (or Wi-Fi requests stop for 15 seconds), the setup/network screen returns.

[Wiki home](Home.md)
