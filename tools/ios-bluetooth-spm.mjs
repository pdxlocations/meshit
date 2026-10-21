// Adapt the installed MIT-licensed plugin's Objective-C registration to Swift SPM.
// Generated sources stay in ios/; node_modules is never modified.
import {existsSync} from 'node:fs';
import {readFile,writeFile,mkdir,readdir,copyFile} from 'node:fs/promises';
const packagePath='ios/App/CapApp-SPM';
if(existsSync(packagePath)){
  const source='node_modules/@capacitor-community/bluetooth-le',target=packagePath+'/Sources/MeshitBluetooth';await mkdir(target,{recursive:true});
  for(const file of await readdir(source+'/ios/Plugin'))if(file.endsWith('.swift')){const text=await readFile(source+'/ios/Plugin/'+file,'utf8');await writeFile(target+'/'+file,'import UIKit\n'+text);}
  const objc=await readFile(source+'/ios/Plugin/Plugin.m','utf8');const methods=[...objc.matchAll(/CAP_PLUGIN_METHOD\((\w+),\s*(\w+)\)/g)].map(m=>`CAPPluginMethod(name: "${m[1]}", returnType: ${m[2]})`);
  let plugin=await readFile(target+'/Plugin.swift','utf8');plugin=plugin.replace('public class BluetoothLe: CAPPlugin {',`public class BluetoothLe: CAPPlugin, CAPBridgedPlugin {
    public let identifier = "BluetoothLe"
    public let jsName = "BluetoothLe"
    public let pluginMethods: [CAPPluginMethod] = [${methods.join(',\n        ')}]
`);await writeFile(target+'/Plugin.swift',plugin);
  const {version}=JSON.parse(await readFile('node_modules/@capacitor/core/package.json','utf8'));
  await writeFile(packagePath+'/Package.swift',`// swift-tools-version: 5.9
import PackageDescription
let package = Package(
    name: "CapApp-SPM", platforms: [.iOS(.v16)],
    products: [.library(name: "CapApp-SPM", targets: ["CapApp-SPM"])],
    dependencies: [.package(url: "https://github.com/ionic-team/capacitor-swift-pm.git", exact: "${version}")],
    targets: [
        .target(name: "MeshitBluetooth", dependencies: [.product(name: "Capacitor", package: "capacitor-swift-pm")]),
        .target(name: "CapApp-SPM", dependencies: ["MeshitBluetooth", .product(name: "Capacitor", package: "capacitor-swift-pm"), .product(name: "Cordova", package: "capacitor-swift-pm")])
    ]
)
`);
  await writeFile(packagePath+'/Sources/CapApp-SPM/CapApp-SPM.swift','@_exported import MeshitBluetooth\npublic let isCapacitorApp = true\n');
  let delegate=await readFile('ios/App/App/AppDelegate.swift','utf8');if(!delegate.includes('import CapApp_SPM'))delegate=delegate.replace('import Capacitor','import Capacitor\nimport CapApp_SPM');
  if(!delegate.includes('_ = BluetoothLe.self'))delegate=delegate.replace('// Override point for customization after application launch.','_ = BluetoothLe.self // Keep the dynamically registered plugin linked.');
  await writeFile('ios/App/App/AppDelegate.swift',delegate);
  const license=(await readdir(source)).find(f=>/^licen[cs]e/i.test(f));if(license)await copyFile(source+'/'+license,packagePath+'/BLUETOOTH-LICENSE');
  console.log('iOS Bluetooth Swift package and Capacitor registration prepared.');
}
