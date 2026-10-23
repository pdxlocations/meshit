# LILYGO T-Deck

T-Deck radios can run meshit directly on their built-in display and keyboard. The same radio also serves the browser client over Wi-Fi and connects to mobile companions over Bluetooth. Both interfaces share rooms, messages, votes, sending state, and the radio’s 200-message history.

## Choose your model

| Hardware | Firmware target | Screen |
| --- | --- | --- |
| T-Deck, original | `t-deck` | ST7789 TFT |
| T-Deck Plus | `t-deck-plus` | ST7789 TFT, Plus initialization |
| T-Deck Pro v1.0 | `t-deck-pro` | GDEQ031T10 e-paper |
| T-Deck Pro v1.1 | `t-deck-pro-v1-1` | GDEQ031T10 e-paper |
| T-Deck Pro Max | `t-deck-pro-max` | GDEQ031T10 e-paper |

Audio and cellular options use the profile for their board revision. A LoRa-equipped board is required. Match the radio’s physical band and antenna to your selected preset; 433 MHz Meshtastic hardware can use the **EU_433 / LongFast** preset (433.875 MHz). The same binaries cover regional radio options; they do not change the RF hardware’s matching network.

The original and Plus share radio wiring but differ in panel initialization. Pro v1.0 and v1.1 have different display reset pins. Pro Max uses an XL9555 expander for radio power and antenna selection; this profile selects the internal antenna. These five profiles have passed source syntax checks against their libraries and host tests. Firmware builds, RF operation, pairing, display behavior, and keyboard wiring still need physical verification. No firmware or Android binaries were built locally for this port.

## First setup

Use the [web installer](https://meshit.pdxlocs.com) and choose your exact model after a release with these targets is published. If USB upload does not start, hold BOOT while resetting the board; on the original T-Deck, the trackball center is BOOT. Release it after entering download mode. Application-only updates preserve existing meshit credentials and settings.

On first boot, choose the radio preset on the device. The list includes each preset’s network and region. **Enter** selects it. Changing the network or preset clears radio history, just as in the companion; queued transmissions must finish first.

## T-Deck Plus touch controls

The Plus initializes its GT911 touchscreen when detected and uses the same orientation for touch and display. Tap the chat/gear icons to switch between messages and Settings. Tap the room title to choose a room, **New / Top** to sort, **Reply** beneath a message to reply, the up arrow to toggle a vote, and **Post** to send your typed message. Tap the resend icon to retry an unacknowledged outgoing post. A second tap on the selected message opens its full text.

Drag vertically to scroll messages and menus in the direction of your finger. Scrolling updates while dragging and continues briefly with momentum after release; touching the screen stops it. Trackball selection remains separate from touch scrolling. Rooms includes a **New room** button. Settings offers radio connection details, radio presets, nickname, local Wi-Fi credentials, and appearance; the appearance choice is saved on the device. Text fields can be tapped to focus, then edited with the physical keyboard. Editors have tappable **Save** and **Cancel** controls; canceling restores your chat draft. Local Wi-Fi changes take effect after restarting the radio, and a blank password keeps the saved password. The full-message view also has **Reply**, **Upvote**, and, when available, **Resend** controls, plus a back button. Use the physical keyboard to enter text. The composer starts at one line and expands as you type, and changes to **Post a reply** while replying.

The Plus display now uses landscape rotation 3 to correct the reported upside-down image; touch coordinates rotate with it. Original T-Deck and Pro display rotations remain unchanged. Touch absence or failed detection leaves keyboard and trackball controls available. Physical touch alignment and panel orientation still need verification on the tested Plus. Other models retain their existing standalone interface and keyboard controls.

## Keyboard controls

| Action | Original / Plus | Pro / Pro Max |
| --- | --- | --- |
| Move through messages and menus | Roll trackball up/down; Plus also accepts vertical swipes | Hold **Alt + E / X** |
| Open or close menu | Trackball center, microphone key, or Enter with an empty composer | **$** key, **Alt + T**, BOOT, or Enter with an empty composer |
| Select menu item | Enter | Enter |
| Return / cancel reply | Menu → New post | **Alt + Q**, or Menu → New post |
| Write a post | Type, then Enter | Type, then Enter |

On Plus, rolling left opens Rooms and rolling right opens Settings; tapping the header controls provides the same navigation. The menu offers **Reply**, **Toggle upvote**, **Resend**, **Rooms**, **New room**, **New post**, **New / Top**, **Radio preset**, **Nickname**, **Wi-Fi / Bluetooth**, and **Read full message**. Actions apply to the selected message, shown with `>`. Posts default to Top sorting; replies remain below their parents.

Choose **Reply**, type, and press Enter to post to that message. Failed queue attempts retain your draft. **Resend** queues the original outgoing message without creating another post; it is available only when the radio permits a resend. `[ok]` means a matching rebroadcast was heard, `[...]` means queued, and `[retry]` means no acknowledgment is currently pending.

**Read full message** opens a scrolling view so long posts remain readable; use up/down and Enter to return. The device uses a limited glyph set and displays unsupported Unicode as `?`; stored and transmitted text keeps its original UTF-8. Phone/browser clients retain full text and emoji rendering.

Pro keyboards support held Shift for uppercase and held Sym for the printed numbers and punctuation. The original/Plus keeps its existing keyboard-controller firmware; meshit does not reflash that controller.

## Companion connection and device status

Open **Wi-Fi / Bluetooth** to read the `mesit-` MAC-based device name, saved numeric AP password, browser address, Bluetooth PIN, battery voltage, outgoing queue count, and new-message count. Phone/browser connection and Wi-Fi/Bluetooth handoff work as on other ESP32 radios. Bluetooth pairing temporarily displays the PIN on the device.

The chat header shows battery voltage, radio readiness, outgoing messages, and new messages. Viewing a room clears its new-message count. E-paper refreshes are throttled, with periodic full refreshes; radio receive handling continues during the panel’s busy wait.

Touch is enabled on T-Deck Plus when its GT911 controller is detected. Other models currently retain keyboard/trackball input. GPS, SD storage, audio, cellular data, motion sensors, and haptics are not used by meshit. Chat uses the keyboard/trackball and the LoRa radio. The host companion remains available if the keyboard cannot initialize.

## Hardware references

Pin assignments and panel setup follow LILYGO’s [T-Deck sources](https://github.com/Xinyuan-LilyGO/T-Deck), [Pro v1.0 sources](https://github.com/Xinyuan-LilyGO/T-Deck-Pro/tree/HD-V1-250326), [Pro v1.1 sources](https://github.com/Xinyuan-LilyGO/T-Deck-Pro/tree/HD-V2-250915), and [Pro Max sources](https://github.com/Xinyuan-LilyGO/T-Deck-Pro/tree/HD-V3-250911). The 433 MHz preset follows [Meshtastic’s radio settings](https://meshtastic.org/docs/overview/radio-settings/).

[Wiki home](Home.md)
