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
pio run -e heltec-v3 -e rak4631 -e rak4631-s140-6-1-1
npm run package:firmware
```

Packages appear in `dist/firmware/` with a SHA-256 manifest. RAK UF2 packages contain application firmware only. **Match the installed SoftDevice:** `meshit-rak4631.uf2` is S140 7.3.0; `meshit-rak4631-s140-6-1-1.uf2` is S140 6.1.1. The packager checks the application offset, MCU family, and reserved-storage boundary. No bootloader replacement is needed or performed.

Heltec packages contain an app-only update (`meshit-heltec-v3.bin`, offset `0x10000`) and a full-flash fresh-install image (`meshit-heltec-v3-factory.bin`, offset `0x0`). The latter replaces saved data. The companion website is embedded in the application; neither path needs a separate filesystem upload. PlatformIO can also install directly:

```sh
pio run -e heltec-v3 -t upload
```

The nRF builds reserve `0x80000–0xECFFF` for Meshit storage; linker limits prevent application overlap. Board pin mappings and the two RAK SoftDevice layouts take cues from the neighboring Form8 project. Other Form8 board targets have not been ported yet.

## Release workflow

The [web flasher](../../webflasher/README.md) installs ESP firmware over browser USB and provides verified RAK UF2/HEX downloads with explicit SoftDevice matching. Upload `webflasher/` to a PHP 8.1+ HTTPS host, as with Form8. It automatically discovers published builds from `pdxlocations/meshit`.

GitHub workflows run CI on pushes and pull requests, build all firmware plus Android/iOS companions, and package release downloads. **Release firmware → Run workflow** accepts a numeric version and publishes a beta named `vVERSION.COMMIT-beta` after all checks pass. Publishing an existing tagged release also builds and attaches its downloads. Only the publishing job receives repository write permission.

For the equivalent local release bundle:

```sh
npm run build:android
npm run package:release
```

This uses the packaged firmware above and creates `dist/meshit-release.zip`, its SHA-256 checksum, and the standalone Android APK/checksum. The ZIP contains `esp32/`, `nrf52/`, and the uploadable `webflasher/`. iOS simulator builds remain CI artifacts; signing an iPhone release requires separate provisioning.

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
git -C /tmp/meshit-wiki commit -m "Update Meshit wiki"
git -C /tmp/meshit-wiki push
```

Create the first Home page in GitHub's Wiki UI before cloning a new wiki. The export converts page links to wiki routes and source/image links to the main repository. It does not commit or push automatically.
