import {mkdir,copyFile,readFile,writeFile,rm} from 'node:fs/promises';
import {build} from 'esbuild';
import {execFileSync} from 'node:child_process';
await rm('dist/mobile',{recursive:true,force:true});await mkdir('dist/mobile',{recursive:true});
for(const file of ['index.html','style.css'])await copyFile('web/'+file,'dist/mobile/'+file);
await build({entryPoints:['web/app.js'],outdir:'dist/mobile',bundle:true,format:'esm',splitting:true,minify:true,target:['chrome100','safari16'],entryNames:'app',chunkNames:'chunks/[name]-[hash]'});
// Bundled relative resources work in both Capacitor's Android and iOS local origins.
const html=await readFile('dist/mobile/index.html','utf8');await writeFile('dist/mobile/index.html',html.replaceAll('href="/style.css"','href="./style.css"').replaceAll('src="/app.js"','src="./app.js"'));
await writeFile('dist/mobile/build-info.json',execFileSync('python3',['tools/build_info.py']));
console.log('Android/iOS companion assets bundled in dist/mobile.');
