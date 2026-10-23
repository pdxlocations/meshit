import {existsSync} from 'node:fs';
import {readFile,writeFile} from 'node:fs/promises';
import {execFileSync} from 'node:child_process';
const manifest='android/app/src/main/AndroidManifest.xml';
if(existsSync(manifest)){
  let text=await readFile(manifest,'utf8');
  const permissions=[['android.permission.BLUETOOTH_SCAN','android:usesPermissionFlags="neverForLocation"'],['android.permission.BLUETOOTH_CONNECT',''],['android.permission.INTERNET',''],['android.permission.ACCESS_NETWORK_STATE','']];
  for(const [name,extra] of permissions)if(!text.includes('android:name="'+name+'"'))text=text.replace('<application',`<uses-permission android:name="${name}" ${extra}/>\n    <application`);
  if(!text.includes('android:usesCleartextTraffic='))text=text.replace('<application','<application android:usesCleartextTraffic="true"');
  await writeFile(manifest,text);
  const strings='android/app/src/main/res/values/strings.xml';let labels=await readFile(strings,'utf8');
  labels=labels.replace(/(<string name="(?:app_name|title_activity_main)">)[^<]*(<\/string>)/g,'$1meshit$2');await writeFile(strings,labels);
  const variables='android/variables.gradle';let vars=await readFile(variables,'utf8');vars=vars.replace(/minSdkVersion\s*=\s*\d+/,'minSdkVersion = 31');await writeFile(variables,vars);
  const info=JSON.parse(execFileSync('python3',['tools/build_info.py'],{encoding:'utf8'}));
  const gradle='android/app/build.gradle';let build=await readFile(gradle,'utf8');
  build=build.replace(/versionName\s+"[^"]*"/,`versionName ${JSON.stringify(info.version)}`);await writeFile(gradle,build);
  console.log('Android Bluetooth permissions configured; minimum Android 12.');
}
const plist='ios/App/App/Info.plist';
if(existsSync(plist)){
  const entries={CFBundleDisplayName:'meshit',NSBluetoothAlwaysUsageDescription:'Connect to your meshit radio for off-grid chat.',NSLocalNetworkUsageDescription:'Connect to your meshit radio on your local Wi-Fi network.',NSBonjourServices:['_http._tcp']};
  execFileSync('python3',['-c',`import plistlib,json,sys
p=sys.argv[1]
with open(p,'rb') as f: d=plistlib.load(f)
d.update(json.loads(sys.argv[2]))
d.setdefault('NSAppTransportSecurity',{})['NSAllowsLocalNetworking']=True
with open(p,'wb') as f: plistlib.dump(d,f)
`,plist,JSON.stringify(entries)]);
  console.log('iOS Bluetooth and local-network usage descriptions configured.');
  const project='ios/App/App.xcodeproj/project.pbxproj';let pbx=await readFile(project,'utf8');pbx=pbx.replace(/IPHONEOS_DEPLOYMENT_TARGET = [\d.]+;/g,'IPHONEOS_DEPLOYMENT_TARGET = 16.0;');await writeFile(project,pbx);
}
