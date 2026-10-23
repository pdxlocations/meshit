# Development and releases

## Local preview

Install [PlatformIO](https://platformio.org/), Node.js 22+, and a C++17 compiler. Then:

```sh
pio pkg install -e heltec-v3
npm ci
npm run dev
```

Open **http://127.0.0.1:8787**. The preview uses the actual C++ chat core and **simulates** rebroadcasts. It does not transmit radio traffic. Preview history lasts for the server process; browser drafts and the last received feed are saved locally.

## Firmware builds

```sh
npm run build:firmware
npm run package:firmware
```

Packages appear in `dist/firmware/` with a SHA-256 manifest. nRF52840 UF2 packages contain application firmware only. **Match the installed SoftDevice:** `meshit-rak4631.uf2` is S140 7.3.0; `meshit-rak4631-s140-6-1-1.uf2` is S140 6.1.1. The packager checks the application offset, MCU family, and reserved-storage boundary. T114 uses `meshit-heltec-t114.uf2` with S140 7.3.0. No bootloader replacement is needed or performed.

Heltec packages contain an app-only update (`meshit-heltec-v3.bin`, offset `0x10000`) and a full-flash fresh-install image (`meshit-heltec-v3-factory.bin`, offset `0x0`). The latter replaces saved data. The companion website is embedded in the application; neither path needs a separate filesystem upload. PlatformIO can also install directly:

```sh
pio run -e heltec-v3 -t upload
```

The nRF builds reserve `0x80000–0xECFFF` for meshit storage; linker limits prevent application overlap. Board pin mappings and the two RAK SoftDevice layouts take cues from the neighboring Form8 project. V4.2 OLED / GC1109, V4.3 OLED / KCT8103L, and T114 are included. See the [board profiles](Hardware-and-Presets.md) for display limitations and physical validation status.

## Release workflow

The [web flasher](../../webflasher/README.md) installs ESP firmware over browser USB and provides verified RAK4631 / T114 UF2/HEX downloads with explicit SoftDevice matching. Upload `webflasher/` to a PHP 8.1+ HTTPS host, as with Form8. It automatically discovers published builds from `pdxlocations/meshit`.

CI runs host and web flasher checks on branch pushes and pull requests without building firmware targets or the Android app. Release workflows build all firmware targets and the Android companion, and package release downloads. **Release firmware → Run workflow** accepts a numeric version and publishes a beta named `vVERSION.COMMIT-beta` after all checks pass. Publishing an existing tagged release also builds and attaches its downloads. The release tag is reserved at the selected commit before builds start, so later pushes do not change that release. Publishing uses that verified existing tag without passing an outdated commit as a new tag target. Tag pushes do not start a second CI build. Only the tag-reservation and publishing jobs receive repository write permission. A failed build may leave a reserved tag without a published release; rerunning the same version/commit reuses it.

For the equivalent local release bundle:

```sh
npm run build:android
npm run package:release
```

This uses the packaged firmware above and creates `dist/meshit-release.zip`, its SHA-256 checksum, and the standalone versioned Android APK/checksum (`meshit-android-v<version>-<commit>-debug.apk`). The ZIP contains `esp32/`, `nrf52/`, and the uploadable `webflasher/`. iOS builds are local-only in Xcode; GitHub Actions does not build or upload an iOS app. Signing an iPhone release requires separate provisioning.

## Validation and source map

Run `npm test` for core, simulator, and Node tests; `npm run test:webflasher` for the flasher and PHP bridge; and `npm run test:targets` for target consistency. See [verification](../VALIDATION.md) for build and hardware-test limits.

| Path | Purpose |
| --- | --- |
| `lib/`, `include/` | Shared application core and protocols |
| `src/`, `firmware/` | Board firmware and nRF layouts |
| `config/` | Radio preset and target catalogs |
| `web/` | Companion interface reused on radio and mobile |
| `webflasher/` | PHP-hosted firmware installer |
| `tools/` | Simulator, packaging, and mobile helpers |
| `tests/` | Core, transport, packaging, and flasher checks |

Full [flasher hosting and release instructions](../../webflasher/README.md) include PHP requirements, archive contents, and caching.

[Wiki home](Home.md)

## Maintain this wiki

Edit the Markdown sources in `docs/wiki/` and commit them with the repository. To publish the same pages to the native GitHub Wiki, clone its separate repository and export the sources:

```sh
git clone https://github.com/pdxlocations/meshit.wiki.git /tmp/meshit-wiki
python3 tools/export_wiki.py /tmp/meshit-wiki
git -C /tmp/meshit-wiki add '*.md'
git -C /tmp/meshit-wiki commit -m "Update meshit wiki"
git -C /tmp/meshit-wiki push
```

Create the first Home page in GitHub's Wiki UI before cloning a new wiki. The export converts page links to wiki routes and source/image links to the main repository. It does not commit or push automatically.

T-Deck release targets are `t-deck`, `t-deck-plus`, `t-deck-pro` (v1.0), `t-deck-pro-v1-1`, and `t-deck-pro-max`. Run a selected target with `pio run -e TARGET`. Release builds include all five; push/PR CI still runs checks without building firmware or Android.
