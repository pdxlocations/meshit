# History, encryption, and protocol

The radio keeps **200 posts and replies total across rooms**, evicting lowest-voted first and then oldest. Evicting a parent removes its entire descendant thread, even if replies have votes. Upvoted messages remain until displaced. A room disappears when its last retained message disappears; empty room drafts remain local until their first post.

Numbering is automatic: a six-byte ID combines a sender identity and its counter. Hearing a new sender establishes a baseline without downloading old history. Subsequent gaps trigger bounded requests. Old missing posts are requested only when a newly heard upvote references them. No central numbering server or clock synchronization is needed.

Application content uses **ChaCha20-Poly1305** with a built-in shared key and random 96-bit nonces. No key entry is required. It obscures broker contents even if an MQTT gateway removes transport encryption. The key is public in this source; this is not secrecy from other meshit users or someone inspecting the implementation. Transport metadata and traffic volume remain visible. MQTT forwarding depends on the gateway's application-port/channel filters; compatibility does not guarantee passage through every broker.

See [protocol and bounds](../PROTOCOL.md), [implementation decisions](../../REQUIREMENTS.md), and [verification](../VALIDATION.md).

```sh
npm test
```

## Trust and interoperability

The shared application key is public. Nicknames are not verified identities, and the implementation does not resist invented voting identities. Trust local LAN and BLE peers: application encryption does not secure the local settings interface or authenticate its operator.

meshit originates application traffic and relies on existing relays. It does not bridge Meshtastic and MeshCore or turn ordinary Meshtastic/MeshCore chat into meshit rooms. All participants must use meshit and matching transport settings. Hardware forwarding, RF timing, power-loss behavior, and real-phone BLE still require physical validation; see the [verification record](../VALIDATION.md).


Meshtastic packets originate with `hop_limit=7` and `hop_start=7`. Relays decrement the remaining limit while retaining the starting value. The broadcast header does not request an ACK, and the explicitly present Data bitfield requests neither a response nor MQTT uplink. meshit's checkmark still requires an authenticated meshit message heard via a relay.

[Wiki home](Home.md)
