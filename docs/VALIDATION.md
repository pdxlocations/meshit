# Verification record

Verified on October 21, 2026. No radio was flashed and no RF or physical-phone test was performed.

## Passed

- **Firmware compilation:** Heltec V3, RAK4631 S140 7.3.0, and RAK4631 S140 6.1.1, using the pinned PlatformIO environments. ESP embeds the compressed web interface directly into its application image. nRF linker layouts reserve its larger history filesystem.
- **C++ behavior under AddressSanitizer and UndefinedBehaviorSanitizer:** six-byte IDs, strict UTF-8 and 160-character limits, reply relationships, lowest-voted eviction, descendant eviction, upvote removal and replay ordering, recent gap recovery, first-hearing history skip, upvote-triggered old recovery, reply-triggered ancestor recovery, binary snapshot restore/truncation rejection, channel-busy behavior, exact retry count, forged/early echo rejection, and late implicit checks.
- **Cryptography:** independent Node/OpenSSL vectors confirm the application ChaCha20-Poly1305 envelope, Meshtastic AES-CTR wrapping, and MeshCore AES-ECB/HMAC wrapping. Every single-byte mutation of a test envelope is rejected before application processing.
- **Carrier and fragment bounds:** maximal 160-character four-byte Unicode messages fit the bounded fragment count. Tests round-trip both network carriers and exercise relay evidence, channel mismatch, and out-of-order assembly.
- **Companion protocol tests:** first-run network/preset selection, posts, rooms, replies, votes and removals, simulated implicit checks, HTTP error propagation, serialized BLE RPCs, disconnect rejection, and UTF-8 split across single-byte notifications. The simulator shares the firmware's C++ core; its echoes are simulated, not RF measurements.
- **Mobile assets:** bundled successfully with esbuild; all UI assets are local. Native Bluetooth and HTTP adapters are included.
- **Android:** Gradle `assembleDebug --offline` passed with Android Studio's Java runtime and the installed SDK. APK: `dist/companions/meshit-android-v<version>-<commit>-debug.apk`.
- **iOS:** Xcode built the app and generated Bluetooth adapter for the generic iOS Simulator destination, with signing disabled. Project: `ios/App/App.xcodeproj`. This is a compilation result, not a signed phone release or simulator runtime test.
- **Firmware packaging:** RAK UF2 conversion verifies family `0xADA52840`, the S140-specific application start, and no overlap with reserved history flash. SHA-256 hashes accompany all packaged images.

## Practical limits of these results

Host round trips and upstream format inspection do not prove interoperability with deployed repeaters. Physical tests still need to establish Meshtastic and MeshCore forwarding, actual modem timing and implicit echoes, retries after fragment loss, busy-channel handling, flash recovery after power loss, and peak heap use with a full Unicode-heavy history.

Local Wi-Fi setup, LAN joining, `meshit.local` discovery, BLE reconnection and notification pacing require radios and real Android/iOS devices. The iOS simulator cannot validate BLE; the installed simulator service also reported a runtime-version mismatch during compilation. Android signing/distribution and iOS provisioning are not configured.

Desktop Chrome visual review and updated chat screenshots in light/dark mode plus settings screenshots were completed using the local simulator during the documentation update. No mobile viewport or physical-device screenshots were captured. UI/API behavior and native bundling were checked through tests and builds. The local preview is available through `npm run dev`.

The firmware size reports describe linked static RAM and flash, not maximum live heap consumption. No PSRAM is required by the implementation. The documented resource caps are application bounds, not a claim of measured hardware battery life or memory headroom.

## Web flasher and release automation

The added Form8-style flasher and release tooling were checked locally:

- All three firmware environments rebuilt with version metadata; PlatformIO exports the actual ESP bootloader, partition, OTA, and flash-mode inputs for packaging.
- Real app/factory/UF2/HEX artifacts pass the browser verifier. The factory image spans 8 MiB, contains the exact application bytes, and clears the remainder. RAK validation uses the `0x80000` reserved-storage boundary.
- Browser-flow unit tests verify RAK downloading without Web Serial, explicit SoftDevice matching, corrupted-download rejection, and clearing prepared files after selection changes.
- PHP syntax and bridge tests pass, covering allowed GitHub URLs, exact ZIP paths, size limits, duplicate entries, and actual image bytes from the completed release ZIP.
- Release package tests cover catalog completeness, mixed-build rejection, ZIP integrity, and checksums. The complete `dist/meshit-release.zip` and standalone APK/checksum are generated locally.
- The Android workflow commands (`assembleDebug`, `lintDebug`, and unit tests) pass using the installed SDK/runtime.
- All three GitHub workflow files pass actionlint 1.7.12. The workflows themselves have not run on GitHub; that requires pushing this source to the repository. No GitHub release or hosted site was published during this work.

Hosting instructions and exact release steps are in [webflasher/README.md](../webflasher/README.md). USB flashing and preservation of saved radio data remain physical-hardware checks.

## Reproduce

```sh
npm ci
pio pkg install -e heltec-v3
npm test
npm run build:firmware
npm run package:firmware
npm run mobile:android
npm run mobile:ios
```

After regenerating mobile assets, `npm run mobile:sync` also regenerates the iOS Swift Bluetooth registration adapter. Build Android with Gradle/Android Studio and iOS with Xcode; generated projects remain outside the checked-in source.

## On-device Wi-Fi setup

The setup password policy is tested in the host suite: no-display fallback `12345678`, migration of old or malformed credentials, eight ASCII digits with a repeated four-digit PIN, deterministic boundary inputs, and reuse across restarts. The Heltec display path probes I2C before initialization and selects the fallback when probing or allocation fails. RAK builds retain Bluetooth-only access.

The OLED uses [Heltec V3’s board-defined Vext, reset, SDA, and SCL pins](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/variants/heltec_wifi_lora_32_V3/pins_arduino.h) and the pinned ThingPulse SSD1306 driver. It shows the AP name, large numeric password, and setup URL, then LAN details after the setup AP closes. On-device readability, missing-display detection, and joining the AP still require a physical radio; compilation and host tests do not establish those hardware results.

## Form8-style connection controls

The companion now has one Wi-Fi (ESP32) mode and a Bluetooth (ESP32 / nRF52) flow with Find radios, an explicit device selection, Connect, and Disconnect. Native Wi-Fi uses `192.168.4.1`; the browser uses the page's own origin. The selected connection type persists locally. Host tests cover native scan filtering, explicit selection, reconnect without rescanning, handshake failure cleanup, cancellation of queued BLE commands, and rejecting late replies from a previous connection.

ESP32 requires authenticated, encrypted pairing with the last six digits of its setup password. Wi-Fi clients take priority over Bluetooth discovery, and authenticated Bluetooth pauses Wi-Fi until disconnect, following Form8. All three firmware targets compile. Native scan behavior, OS pairing prompts, and Wi-Fi/Bluetooth handoff still need testing with physical radios and phones.

Meshtastic channel-name entry is removed. Firmware selects its internal transport value from the preset and ignores/removes saved overrides. The README contains light-mode screenshots only; the wiki contains usage and setup guidance without screenshots or styling commentary.

## Additional board profiles and branding

The target catalog and build command include Heltec V3, V4.2 OLED (GC1109), V4.3 OLED (KCT8103L), RAK4631 with S140 7.3.0 or 6.1.1, and Heltec T114 with S140 7.3.0. V4 uses its own 16 MiB partition layout and FEM power/switch pins; T114 uses its SX1262 SPI/IRQ/reset/busy pins and peripheral enable instead of the RAK mapping. New board profiles require physical verification. T114's TFT and optional V4 TFT are not initialized.

The webflasher uses a logo-only header and a slate dark palette. Brand text, setup AP/Bluetooth names, USB product name, native app labels, docs, and release titles use lowercase meshit. Protocol key-derivation literals retain their original bytes for compatibility with existing radios.

## T-Deck port verification

The five T-Deck profiles have passed `-fsyntax-only` checks of `src/main.cpp` with the installed ESP32-S3 Arduino, RadioLib, NimBLE, display, and keyboard headers. These checks create no target object files or firmware binaries. The dependency versions resolve for the TFT and e-paper profiles. Host C++ tests cover threaded ordering, keyboard replies, retained drafts on failed queue attempts, upvote toggles, resend actions, new rooms, preset selection, input length, Unicode-safe backspace, and full-message viewing. Flasher tests verify that all five profiles remain distinct choices with the correct installation offsets. Catalog checks cover all eight ESP32 and three nRF targets.

No T-Deck firmware or Android binaries have been built locally. Link-time size, actual RF operation for each frequency option, Pro Max expander power/antenna routing, ST7789 panel revisions, e-paper behavior during traffic, physical keyboard events, battery readings, and real-device Wi-Fi/Bluetooth pairing remain to be verified on hardware. Release workflows build the new targets when explicitly invoked; push/PR CI remains checks-only.

### T-Deck Plus graphical interface and touch

The Plus uses landscape rotation 3; the original T-Deck keeps rotation 1. Its GT911 touch coordinates follow the same 180-degree correction. Host tests exercise corner transforms, tap/release deduplication, swipe recognition, shared draw/hit geometry, reply targets, posting, votes, settings/appearance, and stale-message handling. Local rendered previews cover light, dark, reply, and Settings states using the same scene layout and FreeSans glyphs. The Plus uses an off-screen buffer, preferably in PSRAM, and redraws changed frames.

No firmware binaries were built for this change. The new orientation, actual GT911 alignment and event timing, touch usability, and frame-buffer memory behavior need confirmation on the user's T-Deck Plus. Other target display rotations and renderers are unchanged.

### Plus touch scrolling and control coverage

Touch drags now update a pixel offset continuously, with momentum and bounds checks, instead of moving message selection once on release. Dragging up increases the offset and moves content up; dragging down reverses it. Host checks cover both directions, momentum, edge clamping, and independence from the selected post. Menu and detail views use the same pixel-scroll mechanism. The LCD uses a 40 MHz transaction clock and transfers only changed pixel regions when its PSRAM frame cache is available; pairing/setup screens invalidate that cache so returning to chat redraws correctly.

Control checks cover composer focus, editor Save/Cancel, restoring drafts, detail-view Reply, local Wi-Fi settings, masked passwords, and preserving saved passwords when left blank. Native request serialization is shared with the tested command helper. Existing thread ordering and reply references are unchanged. The touch direction, smoothness, and transfer rate still need verification on the physical Plus. No firmware or Android binaries were built for this update.

T-Deck settings parity: the Plus gear uses the companion SVG outline. Settings now includes explicit System/Light/Dark appearance, nickname, Meshtastic/MeshCore → region → preset selection with radio parameters, and local Wi-Fi credentials. Appearance saves separately without resetting the radio. Non-Plus keyboard interfaces expose these settings pages as lists. Host tests cover filtered preset selection, back navigation, appearance serialization, and existing editor controls; all five T-Deck profiles pass syntax-only checks. No firmware or Android binaries were built, and hardware display/touch validation remains pending.

T-Deck navigation: horizontal trackball pulses are ignored. Plus horizontal touch gestures navigate Rooms ↔ Chat ↔ Settings, with axis locking to keep vertical scrolling from switching views. The reply composer has a 112×34 pixel cancel target, independent of the text field. Host tests verify swipe directions, vertical gesture isolation, ignored left/right keys, cancel hit testing at the button edge, and preservation of draft text.

T-Deck Plus vertical touch follow-up: chat swipe direction is reversed as requested, while menu scrolling is unchanged. Chat gestures may start anywhere below the header rather than only within the short message viewport. Axis selection tolerates diagonal jitter; a missing GT911 release is canceled after 750 ms without contact reports so subsequent gestures can select a fresh axis. Host tests cover reversed movement, swipe origins, diagonal jitter, cancellation, and recovery. Physical touchscreen validation remains pending.

T-Deck message actions start with Back; Enter on the initial selection returns to the list without changing reply state or draft. Original/Plus trackball center now emits Enter on the press edge, matching keyboard selection, editor save, and post behavior; Pro BOOT remains a menu key. Host navigation tests exercise the new Back-first ordering and shifted actions.

T-Deck Plus message taps open the action menu immediately for the tapped message, with Back selected. The full row supplies the body target while Reply/Vote/Resend retain priority. Horizontal navigation requires at least 60 pixels and a predominantly horizontal gesture, reducing accidental Rooms navigation from short or diagonal movement. Host tests exercise visible row hit testing, initial/repeated taps, target-specific Reply, Back, and rejected short/diagonal navigation gestures.

Live display counters: Plus header shows total unread messages and opens Rooms on tap; room entries show retained and unread message counts. Standalone views acknowledge only displayed message IDs rather than whole rooms. The viewed RPC advertises bounded ID-based acknowledgments so Android/iOS/browser clients can update unread counters despite changing radio revisions; legacy revision-based requests remain supported. LCD/API/T-Deck outgoing counts share one pending-user-event calculation. Host tests cover per-room counts, selective viewing, later unread arrivals, header badge routing, room labels, acknowledgment and retry-expiry queue counts; client tests cover ID-based views during changing revisions. No firmware or Android builds were run.

T-Deck touch targeting follow-up: tap targets are captured at finger-down and accepted only if page/room are unchanged at release. A touch action suppresses a simultaneous keyboard selection. GT911 reports are read with their tracking ID; multi-touch or changing-contact reports cancel the gesture until release. Taps use initial coordinates. Settings, Rooms, Network, Region, Presets, Appearance, and Local Wi-Fi now all begin with a selectable Back row; editors/read-only screens have a labeled top Back control. Host tests cover latched message targets, canceled taps, page changes, and every new Back row. All five profiles pass syntax-only checks; physical confirmation of the reported accidental Rooms behavior is still needed. No firmware or Android binaries were built.

Session recovery verification: `npm test`, `npm run test:targets`, and `npm run test:webflasher` pass. Added host regressions for canceling a latched tap when the room changes and keyboard wraparound back to the Back row on all seven selection pages; the ASan/UBSan core suite passes with these additions. Firmware syntax checks were not repeated during this recovery, and no firmware or Android binaries were built.

T-Deck Plus touch calibration: Settings exposes Calibrate touch when the GT911 is detected. Three full-screen crosshairs fit an affine coordinate transform; a fourth center tap verifies it before atomic local storage and activation. Boot validates saved coefficients; invalid or missing data leaves the identity mapping. Calibration bypasses the current mapping, and Enter/trackball press or Esc cancels without replacing it. Host tests cover translation, scale, swapped axes, skew, singular samples, invalid coefficients, verification, crosshair rendering, Settings entry, and cancellation with draft/reply preservation. `npm test`, all five T-Deck syntax-only checks, and `git diff --check` pass. Physical touch accuracy and restart persistence still need device verification; no firmware binary was built.

T-Deck recovery and menu cleanup: a 1.2-second center/BOOT hold opens Settings and highlights Calibrate touch when available. Short center actions fire only on release, so a long hold never first sends a draft or saves an editor. The microphone/menu key opens a separate navigation menu; message actions contain only Back, Reply, voting, Resend, and Read full message. Tests cover hold/release/repeat/wraparound timing, recovery with a retained draft and reply, calibration entry/cancellation, keyboard navigation, and Navigation touch targets.

Calibrated swipe direction follow-up: removed the extra chat-only sign inversion. Both chat and menu content now follow vertical finger motion in calibrated display coordinates. Regression tests exercise up/down swipes after offset, scale, axis-swap, skew, and flipped-axis transforms. `npm test` and `git diff --check` pass. The previous PlatformIO IDE metadata is no longer present, so firmware syntax checks were not repeated for this follow-up.

TX power and receive-only controls: presets use antenna-port ceilings of 30/27/10 dBm for US/EU_868/EU_433. Board selection chooses up to 22 dBm for unamplified SX1262 targets and nominal 28 dBm for Heltec V4 FEM targets, with gain-curve compensation, a 140 mA SX1262 current limit, and V4.3 GPIO5 switching on TX. Static compilation checks cover the resolved PlatformIO flags for all 11 targets, and host tests verify maximal output without exceeding chip, FEM, or preset limits. Physical RF output remains unmeasured.

Transmit enabled is saved separately from network settings and defaults to On. The T-Deck Settings row and companion checkbox share the `txEnabled` RPC. Disabling stops an active packet, clears its unsent fragments, suppresses the scheduler and packet-start paths, rejects local post/vote/resend requests, and leaves reception active. Queue entries remain available for later retries. Tests cover both T-Deck settings layouts, RPC Boolean validation, receive-only reads, re-enabling, draft retention, browser send disablement, and rollback after a save error. `npm test`, `npm run test:targets`, `npm run test:webflasher`, and `git diff --check` pass.

Full PlatformIO builds pass for all 11 firmware environments with the board-power selector and TX toggle, including all five T-Deck revisions, both V4 FEMs, both RAK SoftDevice layouts, and T114. The ESP32 artifacts were refreshed after the companion checkbox styling was finalized. Firmware was built but not flashed; over-the-air receive-only behavior and conducted TX power still require physical verification.

Direct peer receipts and activity feedback: added meshit event kind 5, a compact receipt for directly received live posts, using the existing authenticated envelope on both carriers. It confirms only a locally sent post and stops its retries. Up to four receipt jobs use jitter, channel/duty checks, expiry, duplicate cooldowns, and peer suppression; history, relayed posts, votes, and receipts do not generate receipts. Meshtastic receipts use zero relay hops. Tests cover direct two-node confirmation, unsent/self/malformed rejection, lost-receipt retries, suppression, bounded queues, channel-busy expiry, snapshot check persistence, and encryption/fragment/carrier round trips on both networks. Previous core scenarios explicitly disable auto-receipts so the old rebroadcast and recovery invariants remain tested independently.

The header has standby/RX/TX antenna states on Plus and companions, and corresponding status text on other T-Decks. A 1.5-second RX pulse and TX priority drive the API and native scene; companions also compare packet counters to detect bursts between polls. Tests cover priority, expiry, clock wraparound, icon rendering, connection hit targeting, client state/counter transitions, and reset to standby. Download status is gated to initial connection/reconnection and node switches. Background revision updates and churn remain silent; browser tests cover both quiet refresh and initial-load progress.

`npm test`, firmware target checks, webflasher tests, and diff checks pass. Final full builds pass for all 11 targets, including the ACK bookkeeping change. Physical direct-link ACK exchange and icon visibility still need verification on radios. Neither flashing nor publishing has been performed.

Downvotes: the full ASan/UBSan core suite and all 28 companion tests pass, including signed wire values, mutually exclusive vote transitions, stale-event rejection, deferred thread recovery, retained vote recovery after restart, MIT1 migration/MIT2 snapshots, RPC validation, and companion downvote controls. All 11 firmware targets build successfully. Android debug assembly, lint, and unit-test tasks pass; the packaged APK contains the updated download-indicator gating and downvote controls. iOS shared assets are synced; no native iOS build or physical radio/touch test was performed for this change.
