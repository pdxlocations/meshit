# Companion screenshots

These images show the current `web/` client running against the local C++ simulator, not a physical radio or an Android emulator:

- `chat.png`: light chat with an upvote, a threaded reply, and a downvote.
- `chat-dark.png`: the same conversation in dark mode.
- `settings.png`: the complete Settings page, including Transmit enabled and Appearance at the end.

The messages and nicknames are fictional. All sample posts use the simulator’s local identity, so they show “you”; delivery checks are simulated. The captures use a 1200 × 800 CSS-pixel viewport at 2× pixel density. Settings is captured at full page height.

To refresh them, run `npm run dev`, configure a sample network, add sample posts/replies/votes, and choose Light or Dark in Settings. Use Chrome DevTools’ **Capture full size screenshot** command. This captures rendered page content without the pointer, browser chrome, or DevTools. Inspect every saved image before replacing these files; do not use screenshots containing a mouse pointer, caret, selection, menus, or unrelated application content.

README and wiki pages reference these paths. `tools/export_wiki.py` converts the wiki image links to their raw URLs in the main repository. Publish repository images along with the corresponding documentation update before publishing new wiki image links.
