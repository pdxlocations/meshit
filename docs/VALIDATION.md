# Verification record

Verified on October 21, 2026. No radio was flashed and no RF or physical-phone test was performed.

## Passed

- **Firmware compilation:** Heltec V3, RAK4631 S140 7.3.0, and RAK4631 S140 6.1.1, using the pinned PlatformIO environments. ESP embeds the compressed web interface directly into its application image. nRF linker layouts reserve its larger history filesystem.
- **C++ behavior under AddressSanitizer and UndefinedBehaviorSanitizer:** six-byte IDs, strict UTF-8 and 160-character limits, reply relationships, lowest-voted eviction, descendant eviction, upvote removal and replay ordering, recent gap recovery, first-hearing history skip, upvote-triggered old recovery, binary snapshot restore/truncation rejection, channel-busy behavior, exact retry count, forged/early echo rejection, and late implicit checks.
- **Cryptography:** independent Node/OpenSSL vectors confirm the application ChaCha20-Poly1305 envelope, Meshtastic AES-CTR wrapping, and MeshCore AES-ECB/HMAC wrapping. Every single-byte mutation of a test envelope is rejected before application processing.
- **Carrier and fragment bounds:** maximal 160-character four-byte Unicode messages fit the bounded fragment count. Tests round-trip both network carriers and exercise relay evidence, channel mismatch, and out-of-order assembly.
- **Companion protocol tests:** first-run network/preset selection, posts, rooms, replies, votes and removals, simulated implicit checks, HTTP error propagation, serialized BLE RPCs, disconnect rejection, and UTF-8 split across single-byte notifications. The simulator shares the firmware's C++ core; its echoes are simulated, not RF measurements.
- **Mobile assets:** bundled successfully with esbuild; all UI assets are local. Native Bluetooth and HTTP adapters are included.
- **Android:** Gradle `assembleDebug --offline` passed with Android Studio's Java runtime and the installed SDK. APK: `dist/companions/meshit-android-debug.apk`.
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
