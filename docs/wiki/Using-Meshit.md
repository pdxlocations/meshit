# Using Meshit

## Rooms and conversations

Choose a room on the left, enter up to **160 Unicode characters**, and select **Post to room**. Select **Reply** beneath a post to start a threaded response; **Cancel reply** returns to a new conversation. Posts are immutable.

Use **New room** to create a room name of up to 16 UTF-8 bytes. A new empty room stays on your companion until its first post, when other radios can discover it. All rooms share the selected transport channel; a room is not a separate private channel.

## Upvotes and sorting

Select the up arrow to upvote, and select it again to remove your vote. **New** sorts by recent local order; **Top** sorts by votes, with recent order breaking ties. Votes are bounded local state, not a globally synchronized ledger.

## The green check

A check means the radio heard an authenticated rebroadcast of its message from another radio. It does not prove delivery to every participant or that anyone read it. No check means no qualifying rebroadcast was heard. The radio allows one initial transmission plus at most three retries. Busy-channel waits do not consume attempts.

## History and offline use

The radio retains **200 posts and replies across all rooms**. When full, it evicts lowest-voted messages first, then oldest; evicting a parent removes its descendants. Upvotes influence retention but do not pin a message forever. Rooms disappear when their last retained message disappears.

Companions keep local drafts and a bounded cached feed while disconnected; sending requires a ready radio. Preview history lasts only for the simulator process. Radio history is persisted across restarts, subject to storage health and installation choices.

![Rooms, replies, votes, and simulated rebroadcast check](../screenshots/chat.png)

Sample conversation in the simulator. Both example authors were entered on the same simulated radio, so both posts show “you.”

[Wiki home](Home.md)
