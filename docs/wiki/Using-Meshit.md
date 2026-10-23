# Using meshit

## Rooms and conversations

Choose a room on the left, enter up to **160 Unicode characters**, and select **Post**. Select **Reply** beneath a post to start a threaded response; **Cancel reply** returns to a new conversation. The composer starts at one line and expands as you type. Its placeholder changes to **Post a reply** while replying. Posts are immutable.

Use **New room** to create a room name of up to 16 UTF-8 bytes. A new empty room stays on your companion until its first post, when other radios can discover it. All rooms share the selected transport channel; a room is not a separate private channel.

## Upvotes, downvotes, and sorting

Select the up arrow to add one to the score or the down arrow to subtract one. Select the active arrow again to clear your vote. Switching direction replaces your previous vote. Update the companion and all participating radios for downvote support. A companion disables downvotes when connected to older radio firmware. **New** sorts by recent local order; **Top** sorts by votes, with recent order breaking ties. Votes are bounded local state, not a globally synchronized ledger.

## The delivery check

A check means another radio confirmed receipt, either through an authenticated rebroadcast or a direct meshit receipt. It does not prove delivery to every participant or that anyone read it. No check means no qualifying confirmation was heard. Direct receipts work without a repeater when both peers have updated firmware and transmission is enabled. Posts and replies appear immediately with a sending indicator. Acknowledgment is shown by the checkmark, without a completion banner. Actions such as voting and saving settings show progress while waiting for the radio, then clear it when complete. Reconnecting, returning from Settings to chat, and switching rooms clear old notices. The radio allows one initial transmission plus at most three retries. When those attempts finish without a check, a resend icon appears in the checkmark’s place on our own messages only. Received messages cannot be resent. Resend queues the original message again without creating a duplicate post. Busy-channel waits do not consume attempts.

The header shows a green circle for connected standby, an up arrow for TX, and a down arrow for RX. An outlined muted circle means disconnected. The connection label is available to screen readers and on hover.

## History and offline use

The radio retains **200 posts and replies across all rooms**. When full, it evicts lowest-voted messages first, then oldest; evicting a parent removes its descendants. Net upvotes minus downvotes influence retention but do not pin a message forever. Rooms disappear when their last retained message disappears.

Companions keep local drafts and a bounded cached feed while disconnected; sending requires a ready radio. Preview history lasts only for the simulator process. Radio history is persisted across restarts, subject to storage health and installation choices.


“Downloading messages” appears only after a radio answers and the initial message pages are being fetched, including after reconnecting or changing nodes. Scanning, pairing, waiting for the first radio response, and routine message/ACK updates do not show it.

## Current companion screenshots

![Light chat with replies, upvotes, downvotes, and signed vote scores](../screenshots/chat.png)

[Dark-mode view](../screenshots/chat-dark.png) · [Settings view](../screenshots/settings.png)

These pointer-free page captures use fictional messages in the local C++ simulator. Delivery checks are simulated, and all sample posts use the simulator’s local identity even when their nicknames differ. They do not establish RF interoperability or phone BLE behavior.

[Wiki home](Home.md)
