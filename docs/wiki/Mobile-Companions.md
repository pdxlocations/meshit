# Android and iOS companions

Install `meshit-android-debug.apk` from [the selected release](https://github.com/pdxlocations/meshit/releases). It is a debug APK; iOS requires a locally built and provisioned app. The bundled UI opens offline, while sending needs Bluetooth or ESP local Wi-Fi. RAK4631 and T114 support Bluetooth and have no Wi-Fi.

## Build the companions

```sh
npm run mobile:android
npm run mobile:ios
```

The scripts generate native projects, configure Bluetooth/local-network access, and reuse `web/`. Android requires Android 12+, Java 21, and an Android SDK. iOS requires iOS 16+ and Xcode. The iOS setup uses Swift Package Manager and adapts the installed Bluetooth plugin's registration; CocoaPods is unnecessary. After web changes, use `npm run mobile:sync` to regenerate assets and the iOS Bluetooth adapter.

Open `android/` in Android Studio or `ios/App/App.xcodeproj` in Xcode. Native projects and build products are generated and ignored by Git. Signing, distribution, and real-phone BLE testing are separate release steps. Bluetooth cannot be verified in an iOS simulator.
[Wiki home](Home.md)
