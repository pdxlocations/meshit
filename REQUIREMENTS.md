# meshit implementation requirements

Status: initial implementation complete; firmware, host tests, Android debug, and iOS simulator compilation passed. Hardware interoperability, visual UI review, and phone BLE validation remain.

## Confirmed

- Lightweight ESP32 and nRF52840 firmware; first boards Heltec V3 and RAK4631.
- Use neighboring `../form8` as a reference for board support, local web hosting, and mobile transports.
- No location, node announcements, direct messages, or ordinary forwarding of received network traffic. Explicitly requested cached-message responses are the agreed exception.
- meshit clients originate messages using either Meshtastic or MeshCore wire protocols. Existing network infrastructure relays them. Only meshit understands the application messages.
- First-run network selection, followed by region and modem presets; configuration remains accessible in Settings.
- Meshtastic MeshOregon preset: 918.5 MHz, BW 125 kHz, SF8, CR 4/5 (user supplied as CR5).
- Hearing our own packet rebroadcast or an authenticated meshit peer receipt produces a check beside our message. A direct meshit peer sends a compact receipt after receiving a live post. Retry three times without confirmation.
- Shared Android/iOS companion framework; ESP serves the companion on its local network at `meshit.local`, with BLE and initial Wi-Fi setup support.
- UI consists of chat and Settings. Rooms have a New room action and appear when room messages arrive. All rooms share the selected network channel; room names accompany messages.
- Text posts and threaded replies, upvotes that can be removed on either, no downvotes, New/Top sorting. Final text limit: 160 Unicode characters (replaces the earlier 200-byte proposal).
- Optional nickname in Settings, default Anonymous. Impersonation resistance is not required.
- Six-byte hexadecimal message identifiers requested, with gap detection and requests for missing messages.
- A missing message may be requested upon hearing an upvote or when needed to reconstruct a received reply’s ancestor chain.
- A 200-message cap across rooms (user called these channels); unvoted messages and inactive rooms fall off the list.
- Apply minimal application-layer encryption with a built-in key, without user key configuration, so broker payloads are not human readable. Shared built-in keys do not provide secrecy from other meshit users or anyone inspecting the implementation.

## Final interview decisions and implementation choices

- Numbering stays automatic: sender identity plus sequence, six bytes total, displayed as 12 hexadecimal digits. No user setup or global allocator.
- Any client with the retained content may answer a request; delayed duplicate-response suppression limits airtime.
- First hearing skips history; subsequent live gaps trigger bounded requests. Missing posts are fetched for newly heard positive upvotes and reply ancestor chains.
- The 200-message cap includes replies. Evict lowest-voted first, oldest first on ties. Upvoted messages stay until displaced. Evicting a parent deletes all descendants, regardless of reply votes. Rooms with no retained messages disappear.
- The implementation preserves the 160-character limit, including multibyte Unicode, using up to seven bounded fragments when metadata and encryption do not fit a carrier packet.
- Meshtastic private application port 256 and MeshCore flood group datagrams carry meshit. Upstream protocol sources and independent cryptographic vectors validate framing; physical interoperability is still to be verified. MQTT gateway filters may affect passage.
- ChaCha20-Poly1305 application encryption uses a built-in shared key and hardware-generated random nonces. No user key configuration; no custom cipher.
- One initial transmission plus three retries, stopping on a matching rebroadcast or a meshit peer receipt for a sent post. A check denotes confirmation from another radio, not a read receipt.
- Compact persistent radio history, votes, tombstones, and pending sends; companion caches 200 messages and drafts locally.
- Targets are Heltec V3, V4.2 OLED / GC1109, V4.3 OLED / KCT8103L, RAK4631 S140 7.3.0 and 6.1.1, Heltec T114 S140 7.3.0, and LILYGO T-Deck, Plus, Pro v1.0, Pro v1.1, and Pro Max. T-Deck supports both standalone screen/keyboard chat and Wi-Fi/Bluetooth companions; unused GPS, cellular, audio, and touch peripherals remain outside chat scope.
- See `docs/PROTOCOL.md` for resource bounds and `docs/VALIDATION.md` for evidence and remaining physical checks.

## Protocol references

- https://meshtastic.org/docs/overview/encryption/
- https://github.com/meshtastic/protobufs/blob/master/meshtastic/portnums.proto
- https://github.com/meshcore-dev/MeshCore/blob/main/docs/packet_format.md
