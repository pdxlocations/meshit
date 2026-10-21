import {existsSync} from 'node:fs';
import {resolve} from 'node:path';
import {execFileSync} from 'node:child_process';
const platform=process.argv[2];if(!['android','ios'].includes(platform))throw new Error('Choose android or ios');
if(!existsSync(platform)){
  if(platform==='ios'){
    // Capacitor 7's add command lowercases "SPM" before a case-sensitive check.
    // Use the same CLI task with an explicit package-manager choice.
    const {loadConfig}=await import('@capacitor/cli/dist/config.js');const {addCommand}=await import('@capacitor/cli/dist/tasks/add.js');
    const config=await loadConfig();config.ios.packageManager=Promise.resolve('SPM');
    config.cli.assets.ios.platformTemplateArchive='ios-spm-template.tar.gz';
    config.cli.assets.ios.platformTemplateArchiveAbs=resolve(config.cli.assetsDirAbs,'ios-spm-template.tar.gz');
    await addCommand(config,'ios');
  }else execFileSync('npx',['cap','add','android'],{stdio:'inherit'});
}
execFileSync('node',['tools/configure-mobile.mjs'],{stdio:'inherit'});
execFileSync('npx',['cap','sync',platform],{stdio:'inherit'});
if(platform==='ios')execFileSync('node',['tools/ios-bluetooth-spm.mjs'],{stdio:'inherit'});
