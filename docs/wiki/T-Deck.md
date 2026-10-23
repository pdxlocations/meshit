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

The Plus initializes its GT911 touchscreen when detected and uses the same orientation for touch and display. Tap the chat/gear icons to switch between messages and Settings. Tap the room title to choose a room, **New / Top** to sort, **Reply** beneath a message to reply, the up arrow to toggle a vote, and **Post** to send your typed message. Tap the resend icon to retry an unacknowledged outgoing post. Tap a message body to open its options menu with **Back** first, followed by **Reply**, voting, resend, and other actions. Choose **Read full message** there to open its full text. Taps keep the control touched at finger-down until release, so incoming messages cannot move a different control underneath the tap; multi-touch contacts are canceled.

On the message list, vertical swipes move content with the finger: swipe up to move content up, or down to move it down. A vertical swipe may start anywhere below the header, including over the composer. Menus retain finger-following scrolling. Scrolling updates while dragging and continues briefly with momentum after release; touching the screen stops it. Trackball selection remains separate from touch scrolling. Every selection menu starts with a full-width **Back** row, selectable by touch, Enter, or trackball press. Editors and read-only screens also have a labeled **Back** control at the top. Rooms includes a **New room** button. Settings offers radio connection details, nickname, local Wi-Fi credentials, and the same Meshtastic / MeshCore, region, and preset choices as the companion clients. Appearance offers **System**, **Light**, and **Dark** and saves the choice on the device; System uses the firmware’s default light appearance because the standalone device has no operating-system theme. The radio connection page shows this device’s connection details; discovering and selecting an external radio remains a companion-client function. Text fields can be tapped to focus, then edited with the physical keyboard. Editors have tappable **Save** and **Cancel** controls; canceling restores your chat draft. Local Wi-Fi changes take effect after restarting the radio, and a blank password keeps the saved password. The full-message view also has **Reply**, **Upvote**, and, when available, **Resend** controls, plus a back button. Use the physical keyboard to enter text. The composer starts at one line and expands as you type, and changes to **Post a reply** while replying.

The Plus display now uses landscape rotation 3 to correct the reported upside-down image; touch coordinates rotate with it. Original T-Deck and Pro display rotations remain unchanged. Touch absence or failed detection leaves keyboard and trackball controls available. Physical touch alignment and panel orientation still need verification on the tested Plus. Other models retain their existing standalone interface and keyboard controls.

## Keyboard controls

| Action | Original / Plus | Pro / Pro Max |
| --- | --- | --- |
| Move through messages and menus | Roll trackball up/down; Plus also accepts vertical swipes | Hold **Alt + E / X** |
| Open or close navigation | Microphone key | **$** key, **Alt + T**, or a short BOOT press |
| Open selected message actions | Enter / short trackball press with an empty composer | Enter with an empty composer |
| Open Settings without touch | Hold trackball press for 1.2 seconds, then release | Hold BOOT for 1.2 seconds, then release |
| Select menu item | Enter or trackball press | Enter |
| Return / cancel reply | Menu → New post | **Alt + Q**, or Menu → New post |
| Write a post | Type, then Enter | Type, then Enter |

On Plus, use the header buttons for page navigation; horizontal swipes do not change pages. Horizontal trackball movement does not navigate; up/down movement selects messages. While replying, tap **Cancel reply** above the text field to return to a new post without deleting your draft. The message menu contains **Back**, **Reply**, **Upvote / Remove upvote**, **Downvote / Remove downvote**, **Resend**, and **Read full message**. Actions apply to the selected message, shown with `>`. The separate navigation menu contains **Back**, **Rooms**, **New room**, **New post**, **New / Top**, and **Settings**. Wi-Fi, radio configuration, nickname, and appearance are in Settings. Posts default to Top sorting; replies remain below their parents.

A short trackball press acts on release, like Enter: select the highlighted menu option, save an editor, or post the current draft. Choose **Reply**, type, and press Enter to post to that message. Failed queue attempts retain your draft. **Resend** queues the original outgoing message without creating another post; it is available only when the radio permits a resend. `[ok]` means a matching rebroadcast or a direct meshit receipt was heard, `[...]` means queued, and `[retry]` means no acknowledgment is currently pending.

**Read full message** opens a scrolling view so long posts remain readable; use up/down and Enter to return. The device uses a limited glyph set and displays unsupported Unicode as `?`; stored and transmitted text keeps its original UTF-8. Phone/browser clients retain full text and emoji rendering.

Pro keyboards support held Shift for uppercase and held Sym for the printed numbers and punctuation. The original/Plus keeps its existing keyboard-controller firmware; meshit does not reflash that controller.

## Companion connection and device status

Open **Wi-Fi / Bluetooth** to read the `meshit-` MAC-based device name, saved numeric AP password, browser address, Bluetooth PIN, battery voltage, outgoing queue count, and new-message count. Phone/browser connection and Wi-Fi/Bluetooth handoff work as on other ESP32 radios. Bluetooth pairing temporarily displays the PIN on the device.

The chat header shows battery voltage, radio activity (standby / RX down arrow / TX up arrow), outgoing messages, and new messages. The Plus header shows **New N** for unread messages across rooms; tap it to open Rooms. Each room lists its retained message count and, when nonzero, its new-message count. Standalone chat clears unread status for messages shown on screen, leaving messages outside the visible part of the list and in other rooms unread. Reading a full message also clears its unread status. Outgoing counts queued user posts and votes awaiting completion; acknowledgments and exhausted retries remove them from that count. E-paper refreshes are throttled, with periodic full refreshes; radio receive handling continues during the panel’s busy wait.

Touch is enabled on T-Deck Plus when its GT911 controller is detected. Other models currently retain keyboard/trackball input. GPS, SD storage, audio, cellular data, motion sensors, and haptics are not used by meshit. Chat uses the keyboard/trackball and the LoRa radio. The host companion remains available if the keyboard cannot initialize.

## Hardware references

Pin assignments and panel setup follow LILYGO’s [T-Deck sources](https://github.com/Xinyuan-LilyGO/T-Deck), [Pro v1.0 sources](https://github.com/Xinyuan-LilyGO/T-Deck-Pro/tree/HD-V1-250326), [Pro v1.1 sources](https://github.com/Xinyuan-LilyGO/T-Deck-Pro/tree/HD-V2-250915), and [Pro Max sources](https://github.com/Xinyuan-LilyGO/T-Deck-Pro/tree/HD-V3-250911). The 433 MHz preset follows [Meshtastic’s radio settings](https://meshtastic.org/docs/overview/radio-settings/).

[Wiki home](Home.md)

Touch page navigation uses the header buttons and Back controls. Horizontal swipes do not switch pages, preventing finger drift on messages from opening Rooms. Small movements under 18 pixels remain taps; vertical scrolling starts after that threshold.

### Touch calibration

On a touchscreen-equipped T-Deck Plus, open **Settings → Calibrate touch** (if touch is misaligned, hold the trackball button for 1.2 seconds, release, then press it once: Calibrate touch is already highlighted). Tap the center of each of the three corner crosshairs, lifting your finger between targets, then tap the center crosshair to verify and save. Calibration corrects offset, scale, axis rotation, and skew and persists across restarts. Failed verification restarts the sequence; the previous calibration stays active until a verified replacement is saved. Press **Enter**, the trackball, or **Esc** to cancel without saving. Calibration does not change your chat draft or reply target.

After one minute without button, keyboard, trackball, touch, or new message/vote activity, the display sleeps. Any button press wakes it; that first press is consumed. Trackball movement, touch, and new messages or votes also wake it and restart the timer. ACKs and duplicate traffic do not wake it. The radio and companion connections keep working. TFT models turn off the backlight and panel; e-paper models hibernate the display controller and retain their last image.
