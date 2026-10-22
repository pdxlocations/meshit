# meshit web flasher

The meshit installer will live at **https://meshit.pdxlocs.com**. For board-specific steps, see the [installation guide](../docs/wiki/Installation.md).

Upload the **contents of this folder** to a PHP-enabled HTTPS host, as with the Form8 installer. Firmware is loaded automatically from published releases in **pdxlocations/meshit**; you do not re-upload binaries to the host for every release.

## Deploy

1. Enable PHP **8.1+** with **cURL, JSON, and ZIP/ZipArchive**. Outbound HTTPS to GitHub and a writable system temporary directory are required.
2. Enable HTTPS on your domain. ESP USB installation requires HTTPS or localhost and desktop Chrome/Edge with Web Serial.
3. Upload `index.html`, `app.js`, `model.js`, `style.css`, `api.php`, `config.php`, and `nrf52-targets.json` together. No Node server, database, hosting credentials, or GitHub token belongs in this folder.
4. Visit `/api.php?resource=releases`. It should return JSON. If PHP source is displayed or downloaded, enable PHP before proceeding.
5. Publish a release using the workflow described below. After its build finishes, refresh the flasher. Enable **Show prereleases** for beta builds.

The repository is configured in `config.php`; links in `index.html` and `app.js` also point to `pdxlocations/meshit`. Update all three when forking. This is a PHP site; uploading it to static GitHub Pages will not run its release bridge.

## ESP installation

Choose the firmware version, your exact **Heltec V3 / V4.2 OLED / V4.3 OLED** target, and installation type:

- **Update:** app-only image at `0x10000`. Existing meshit messages and settings are preserved when the board/partition layout matches and **Erase device stays unchecked** in the installer.
- **First installation / start fresh:** complete 8 MiB (V3) or 16 MiB (V4) image at `0x0`, including the bootloader, partition table, OTA initialization, application, and cleared remaining flash. It replaces all stored data even when the installer’s erase checkbox is unchecked.

Confirm your board and, for a fresh install, data replacement. **Download & verify firmware** checks size and SHA-256 before enabling **Connect USB & install**. The USB tool identifies the processor family, not the board model. Browser access to the USB port happens only after the visitor clicks and selects it.

The companion webpage is embedded in the firmware. No separate filesystem upload is needed. After restart, the Heltec V3/V4 OLED shows its `mesit-…` AP name, eight-digit numeric password, and `http://192.168.4.1`. The password is a four-digit PIN repeated twice (for example, `48214821`) and stays the same across restarts and application-only firmware updates. If the supported display is absent or fails to initialize, the AP password is **`12345678`**. Join the setup Wi-Fi and open the displayed address; **no serial monitor is needed**. AP and Bluetooth names are `mesit-` plus the last four hexadecimal digits of the factory MAC, including leading zeroes. Existing eight-digit numeric passwords are preserved even if display detection changes; home Wi-Fi credentials are unchanged. Factory/full-erase installs remove saved credentials. Older firmware must be updated for this setup flow. After configuring your LAN and restarting, use `http://meshit.local`. After one minute on the LAN, the setup AP closes and the OLED switches to `meshit.local` and the LAN IP. Bluetooth follows the Form8 handoff: disconnect Wi-Fi clients, use Find radios / select / Connect in the companion, and pair ESP32 with the last six digits of the setup password (`345678` without a display). Authenticated Bluetooth pauses Wi-Fi until disconnect; the hotspot and saved LAN connection are then restored.

## RAK4631 / T114 installation

RAK4631 and T114 targets offer verified **UF2/HEX downloads**, not ESP browser flashing. Double-press Reset and read the **SoftDevice** line in `INFO_UF2.TXT` on the bootloader’s USB drive:

| Installed S140 | Select | UF2 |
| --- | --- | --- |
| 6.1.1 | RAK4631 / stock S140 6.1.1 | `meshit-rak4631-s140-6-1-1.uf2` |
| 7.3.0 | RAK4631 / S140 7.3.0 | `meshit-rak4631.uf2` |
| 7.3.0 | Heltec T114 / S140 7.3.0 | `meshit-heltec-t114.uf2` |

The bootloader version is a different number. An unknown or mismatched SoftDevice blocks downloading. After verification, save the UF2 and copy it to the bootloader drive. HEX is for programming tools that preserve the installed matching SoftDevice and bootloader.

Both file formats are checked for size, SHA-256, valid records, application start, vector table, overlapping data, and meshit’s reserved memory boundary at **`0x80000`**. Application images do not replace the SoftDevice or bootloader. The board/SoftDevice requirements come from the selected release’s own catalog, so hosting files cannot silently substitute another release’s layout. Changing the release, board, format, or SoftDevice clears any prepared download.

Use `meshit-android-debug.apk` from the same GitHub release, or build the iOS companion. RAK4631 and T114 have no Wi-Fi interface.

## Releases and caching

The bridge reads the newest **20 published GitHub releases** and offers those containing **`meshit-release.zip`**. Prereleases are hidden until enabled. The ZIP contains:

```text
esp32/       app-only/factory images, manifest, checksums
nrf52/       RAK4631 / T114 UF2/HEX, per-release target catalog, checksums
webflasher/  this uploadable PHP site
```

The Android APK and its checksum are separate release assets. Each full release ZIP also has a standalone SHA-256 checksum.

PHP caches GitHub responses and ZIP downloads for **five minutes** in the system temporary directory, outside the web root. It serves exact allowlisted archive members without extracting them. Limits are 64 MiB per archive, 512 archive entries, 20 MiB per firmware file, and 1 MiB per metadata file. Arbitrary URLs, repository names, and uploads are not accepted. Unauthenticated GitHub API rate limits apply.

ESP Web Tools is loaded from unpkg at pinned version **10.4.0**, after image verification. Firmware checksums detect corruption; they are not a signature independent of the GitHub release publisher and the hosting site.

## Build and publish

From the source repository:

```sh
npm ci
npm run webflasher:prepare
npm run build:firmware
npm run package:firmware
npm run build:android
npm run package:release
```

`dist/meshit-release.zip`, its checksum, the standalone APK, and its checksum are ready for GitHub Releases. The organized folder is `dist/release/`; `dist/release/webflasher/` can be uploaded directly to the host.

The **Release firmware** workflow supports both:

- Publishing a GitHub release for an existing version tag. It builds the tag’s exact commit and attaches the downloads.
- **Run workflow**, entering a numeric version such as `0.1.0`. It builds the selected commit, creates `v0.1.0.COMMIT-beta`, uploads all downloads to a draft, then publishes a prerelease. Existing tags pointing to different code and already-published generated beta tags are rejected.

CI builds all three firmware environments and Android/iOS companions, runs tests and Android lint, and saves artifacts without creating a release. iOS simulator ZIPs stay in workflow artifacts; they are not installable iPhone releases. Publishing needs only GitHub’s normal workflow token; no hosting secrets are used.

## Local checks

```sh
php -S 127.0.0.1:8849 -t webflasher
# Open http://127.0.0.1:8849

npm run test:webflasher
npm run test:targets
# After building and packaging firmware:
node --test tests/release-images.mjs
python3 -m unittest discover -s tests -p 'test_*.py'
```

The preview still contacts GitHub and needs published release assets. Before the first release, the page reports no builds; local packaging tests exercise real binaries without publishing anything.

Physical USB installation and post-update data retention still need hardware verification.

References: [ESP Web Tools](https://esphome.github.io/esp-web-tools/), [GitHub releases API](https://docs.github.com/en/rest/releases/releases#list-releases).
