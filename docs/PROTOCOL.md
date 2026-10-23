# meshit v1 protocol

The application core is independent of Arduino. The same C++ headers drive firmware, the host simulator, and sanitizer tests.

## Identity and events

IDs are 48-bit values displayed as 12 hexadecimal digits: a random persistent 32-bit sender plus a 16-bit sequence. The counter advances for every originated event (post, vote, request). The radio reserves counter blocks in flash before use to prevent reuse after an interrupted write; restarting can skip numbers. A sender rotates on sequence exhaustion. Identities need no advertisements or discovery packets. Nicknames are only fields in posts.

All multibyte application fields are little-endian. Event layout:

| Field | Bytes |
| --- | --- |
| Kind: post=1, vote=2, request=3, history post=4, receipt=5 | 1 |
| Event ID | 6 |
| Reference: parent, vote target, requested ID, or zero | 6 |

Posts/history append room length (1), nickname length (1), text length (2), then their UTF-8 bytes. Room and nickname each fit 16 bytes; an empty nickname means Anonymous. Text is at most 160 Unicode scalar values and 640 UTF-8 bytes. Invalid UTF-8, overlong encodings, surrogates, NULs, and controls other than line breaks are rejected. Names cannot contain line breaks. Votes append one byte: 1 selects an upvote, 2 selects a downvote, and 0 clears either vote. Older firmware rejects downvotes; update participating radios to synchronize them. Requests and receipts append nothing. Receipts reference the acknowledged live post and must have a different sender from that post.

Posts are immutable. A reply references its immediate parent in the same room. Cycles and excessive ancestry are rejected; retained descendant trees are deleted when their parent is evicted. Received replies whose parent has not been heard can be shown as roots, without requesting old history solely because of a reply. Locally displaced parents are tombstoned, preventing their replies from returning while those tombstones remain retained.

For a target and voter, only the newest vote event takes effect. Upvotes add one to the score; downvotes subtract one. Switching direction replaces the previous vote, and selecting the active direction again clears it. Votes and removals use separate event IDs. No resistance to invented identities is attempted.

## Encryption and fragments

The complete encoded event is encrypted using the vetted Crypto library's ChaCha20-Poly1305 implementation. The key is SHA-256 of the public string `Meshit application encryption v1`. Envelope:

| Field | Bytes |
| --- | --- |
| `MI`, version 1 | 3 |
| Hardware-generated random nonce | 12 |
| Ciphertext | event size |
| Poly1305 tag | 16 |

The 15-byte prefix is authenticated associated data. A failed tag discards the whole event before application processing. No content is exposed as ordinary network chat text.

An envelope is split into fragments, each containing `m`, version 1, random 6-byte transfer ID, fragment index, fragment count, and up to 112 envelope bytes. The maximum is seven fragments; even 160 four-byte Unicode characters fit. Ordinary short ASCII messages take fewer fragments. Fragmentation is bounded by the same rules on both networks. Four simultaneous reassemblies expire after ten minutes, accommodating duty-limited presets. A new encryption nonce and transfer ID are generated on each transmission attempt so network duplicate suppression does not prevent a retry.

## Network carriers

**Meshtastic:** standard 16-byte broadcast header, default hop limit/start of 7 (`hop_limit=7`, `hop_start=7`, header flags `0xE7`), WantAck clear, preset-selected public channel-name hash, and a protobuf Data message on private application port 256. Data field 9 (`bitfield`) is explicitly present as zero, matching modern origin metadata without requesting a response or opting into MQTT uplink. The transport payload uses the standard public AES-128-CTR key and nonce layout. meshit accepts only matching broadcast/channel/private-port traffic. A lower remaining hop limit than hop start establishes that the frame was relayed.

**MeshCore:** v1 flood group-data packet, empty initial path, development datagram type `0xFF4D`. The group key is SHA-256 of `Meshit group transport v1`; group wrapping uses MeshCore's AES-128-ECB and truncated HMAC-SHA256 convention. The entire meshit envelope remains independently authenticated. Repeaters do not need to understand the meshit group. Raw custom packet type is deliberately avoided because upstream MeshCore does not flood-relay it. Only group-data flood packets are accepted; path hash sizes 1–3 are supported, with at most 64 path bytes. A nonempty relay path establishes a rebroadcast.

Neither adapter forwards received packets, emits node info, requests network acknowledgments, announces rooms, or emits location/telemetry. Modem setup uses a 16-symbol preamble for Meshtastic and MeshCore's 32-symbol preamble at SF5–8 (16 above SF8), plus the network-specific sync word. Initial meshit support is for public-key Meshtastic channels and the built-in MeshCore application group; private transport-key entry and scope/transport-code routing are not implemented.

## Checks and retries

A post receives its delivery check after either an authenticated matching echo with relay evidence or an authenticated meshit receipt (kind 5) from another sender referencing that post's ID. The post must be locally originated and already transmitted. Local enqueueing, an unrelayed self-echo, network-level ACK, upvote, and history response do not create a check. A late qualifying echo or receipt still confirms a sent post after retries end.

A meshit receiver schedules a receipt only after storing a directly received live post, including a matching retry of a stored post. It never acknowledges history downloads, votes, or receipts. Receipts wait 250–999 ms, follow normal channel/duty/TX-enable checks, and expire after 60 seconds. There are at most four queued receipts and 32 recent target cooldown entries (10 seconds per target). Hearing another receiver's receipt suppresses a queued duplicate. Receipts are one-shot and not persisted; a later retry of the original post can solicit another. Meshtastic receipts have zero relay hops; MeshCore uses the existing group-data wrapper. Both directly communicating nodes need updated firmware; older nodes ignore the new kind and retain rebroadcast-only confirmation.

The scheduler allows one initial attempt plus three retries, starting at 45 seconds with increasing waits and jitter. Channel-busy checks and refused sends do not consume attempts. Each attempt begins when the first fragment enters the radio; remaining fragments follow asynchronously. Failed or interrupted later fragments are repaired by a later whole-event retry. The PHY enforces the selected preset's airtime spacing; pending RF jobs prevent interleaving. Pending attempt counts and sent/check state are saved in the binary snapshot. Actual radio timing remains subject to hardware validation.

## Recovery and resource bounds

There is no periodic history exchange. First hearing a sender establishes its high-water mark. Later gaps queue at most the newest 16 missing IDs. An upvote or downvote fetches its missing target, then any missing ancestors needed to connect that reply to its original post. Every received reply, including a recovered history reply, also checks its parent chain and requests the first missing ancestor without waiting for an upvote. Each recovered ancestor advances dependent threads toward their original post. Unrelated traffic does not restart exhausted recovery. Recovery follows at most 32 levels and retains the existing deduplication, tombstone, queue, and retry limits. Gap requests are broadcast, retried at most three times, and do not request an ACK. Peers with retained content schedule one delayed response; hearing an answer cancels their duplicate response. History posts preserve their original IDs but do not advance the receiver's live high-water mark. A missing request/control event need not be recoverable, so unavailable gaps stop quietly.

| Resource | Bound |
| --- | --- |
| Posts and replies across rooms | 200 |
| Active transmission/history-response jobs | 12 |
| Missing IDs | 16 |
| Sender high-water marks | 64 |
| Recent event dedup IDs | 512 |
| Displaced-message tombstones | 512 |
| Current vote states | 512 |
| Deferred votes for missing posts | 32 |
| Recent recoverable vote events | 32 |
| Queued direct receipts | 4 |
| Receipt cooldown targets | 32 |
| Fragment reassemblies | 4 × 7 fragments |
| Local RPC input line | 2048 bytes |
| RPC message page | 8 messages |

Vote-state bounds prevent unbounded voter storage. If that bound is reached, the oldest tracked voter/target states drop out; scores are local retained counts rather than a global vote ledger. Deduplication, tombstones, and sender marks are bounded; this is not permanent replay prevention. Sender marks are session-local, so a reboot starts new hearing baselines. An evicted body cannot be recovered from a node that no longer stores it.

ESP uses LittleFS. RAK uses a reserved 436 KiB LittleFS region below the framework's normal filesystem and bootloader area. Snapshot version MIT2 stores signed vote states and reads existing MIT1 snapshots; older firmware cannot read MIT2 snapshots. Binary snapshots stream directly to flash, avoiding a second JSON history allocation. Config and identity writes retain a backup during replacement on nRF. Snapshots use temporary files and committed trailers; malformed snapshots are rejected. Radios save history, votes, tombstones, pending sends, and acknowledgment state across restart. Connected companions also cache at most 200 messages locally.

## Local companion API

HTTP: POST `/api/rpc` with JSON. BLE: Nordic UART service `6e400001-b5a3-f393-e0a9-e50e24dcca9e`, RX characteristic `…0002…`, TX `…0003…`. USB uses the same newline-delimited JSON API after boot diagnostics. Each BLE request carries an integer `id`, copied into the response. There is one outstanding request per companion; writes and notifications are streamed in 20-byte pieces with UTF-8 decoding across boundaries.

Operations: `state`; `messages` with offset; `post` with room/text/parent; `vote` with target/value (-1 down, 0 clear, 1 up; legacy target/up remains supported); and `settings` with network/preset/nickname and optional Wi-Fi credentials. Message pages contain a revision; clients discard torn multi-page reads and retry on the next refresh. Passwords are never included in state responses. HTTP browser requests must be same-origin. ESP32 BLE requires encrypted, authenticated pairing with a six-digit PIN derived from its setup password; Wi-Fi is paused during an authenticated BLE connection and resumes afterward. AP clients take priority over Bluetooth discovery. Local LAN and RAK BLE peers are trusted; application encryption does not authenticate a human operator or secure the local settings interface.

The `txEnabled` operation accepts a Boolean `enabled` and saves it immediately without changing network settings or clearing history. `state.settings.txEnabled` defaults to true for existing installations. When false, the radio remains ready to receive but does not transmit posts, votes, retries, history requests, or responses. Locally requested posts, votes, and resends return `Transmit is disabled`; retained pending jobs may resume when enabled. Disabling also stops an in-progress RF transmission and drops its remaining fragments; the normal retry mechanism can repair that event later.

`state.maxTxPowerDbm` is the configured profile ceiling (the chip-drive ceiling for unamplified targets); `txPowerDbm` is the configured antenna-port estimate, and `radioDriveDbm` is the actual SX1262 setting. The preset's `power` is an antenna-port ceiling, not a raw chip setting. RF output has hardware tolerances and is not measured by firmware.

## Primary protocol references

- [Meshtastic on-air header fields and flag masks](https://github.com/meshtastic/firmware/blob/develop/src/mesh/RadioInterface.h)
- [Meshtastic origin metadata and encoding](https://github.com/meshtastic/firmware/blob/develop/src/mesh/Router.cpp)
- [Meshtastic packet layout and routing](https://meshtastic.org/docs/overview/mesh-algo/)
- [Meshtastic application ports](https://github.com/meshtastic/protobufs/blob/master/meshtastic/portnums.proto)
- [Meshtastic radio presets](https://github.com/meshtastic/meshtastic/blob/master/docs/about/overview/radio-settings.mdx)
- [MeshCore packet and path format](https://github.com/meshcore-dev/MeshCore/blob/main/docs/packet_format.md)
- [MeshCore group datagrams](https://github.com/meshcore-dev/MeshCore/blob/main/docs/payloads.md)
- [MeshCore forwarding behavior](https://github.com/meshcore-dev/MeshCore/blob/main/src/Mesh.cpp)
- [MeshCore cryptographic wrapper](https://github.com/meshcore-dev/MeshCore/blob/main/src/Utils.cpp)
- [MeshCore experimental datagram range](https://github.com/meshcore-dev/MeshCore/blob/main/docs/number_allocations.md)
- [MeshCore preamble settings](https://github.com/meshcore-dev/MeshCore/blob/main/src/helpers/radiolib/RadioLibWrappers.h)
- [Puget Mesh's published narrow preset](https://github.com/pugetmesh/pugetmesh.github.io/blob/main/docs/meshcore/index.md)
- [Crypto library](https://github.com/rweather/arduinolibs/tree/master/libraries/Crypto)

`state.radioState` reports `standby`, `rx`, or `tx`. Standby means no current activity while listening; RX remains visible for 1.5 seconds after reception, and active TX takes priority. Companions also compare packet counters to expose short bursts between polls. The initial-history download indicator is shown only while connecting/reconnecting or switching nodes, not on routine revision refreshes.

`state.signedVotesSupported` advertises downvote support. Messages include `voted` and `downvoted` for the connected node plus the signed aggregate `votes` score. Shared browser, Android, and iOS clients disable downvotes against older firmware. Downvotes use the same event-ID gap recovery and retained-state responses as upvotes.

Settings changes do not wait for queued transmissions. Switching network or preset stops any active transmission, discards old-network pending jobs and partial incoming packets, and clears history, votes, and unread state before receiving on the selected preset. Changes within the same network/preset retain history and queued jobs; an interrupted send can retry normally. Settings must save successfully before any runtime change is applied.
