import {copyFile,mkdir} from 'node:fs/promises';
const root=new URL('../',import.meta.url);
await mkdir(new URL('webflasher/',root),{recursive:true});
await copyFile(new URL('config/nrf52-targets.json',root),new URL('webflasher/nrf52-targets.json',root));
console.log('Meshit web flasher board catalog prepared.');
