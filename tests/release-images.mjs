// Run after packaging: validate actual release artifacts using the browser's verifier.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {webcrypto} from 'node:crypto';
import {validateTargets,validateNrfTargets,verifyImage,verifyNrfImage,nrfCompatibility} from '../webflasher/model.js';
const root=new URL('../dist/firmware/',import.meta.url);
const arrayBuffer=buffer=>buffer.buffer.slice(buffer.byteOffset,buffer.byteOffset+buffer.byteLength);
test('actual packaged ESP app/factory and all nRF layouts pass browser verification',async()=>{
  const manifest=JSON.parse(await readFile(new URL('manifest.json',root))),catalog=JSON.parse(await readFile(new URL('nrf52-targets.json',root))),sums=await readFile(new URL('SHA256SUMS-NRF52',root),'utf8');
  const assets=[];
  for(const t of manifest)for(const image of [t,t.factory])assets.push({name:image.file,bytes:image.bytes});
  for(const t of catalog)for(const ext of ['uf2','hex']){const name=`meshit-${t.environment}.${ext}`,body=await readFile(new URL(name,root));assets.push({name,bytes:body.byteLength});}
  const release={assets},targets=validateTargets(manifest,release),nrf=validateNrfTargets(catalog,sums,release);
  const espCatalog=JSON.parse(await readFile(new URL('../config/firmware-targets.json',import.meta.url)));
  const nrfCatalog=JSON.parse(await readFile(new URL('../config/nrf52-targets.json',import.meta.url)));
  assert.deepEqual(targets.map(t=>t.environment).sort(),espCatalog.map(t=>t.environment).sort());
  assert.deepEqual(nrf.map(t=>t.environment).sort(),nrfCatalog.map(t=>t.environment).sort());
  for(const t of targets){
    for(const image of [t,t.factory])await verifyImage(arrayBuffer(await readFile(new URL(image.file,root))),image,webcrypto);
    const app=await readFile(new URL(t.file,root)),factory=await readFile(new URL(t.factory.file,root));
    assert.equal(factory.length,t.flashMiB*1024*1024);assert.deepEqual(factory.subarray(65536,65536+app.length),app);
    assert.ok(factory.subarray(65536+app.length).every(byte=>byte===255),'fresh installation clears saved storage');
  }
  for(const t of nrf){
    assert.equal(nrfCompatibility(t,t.softdevice).ok,true);
    for(const image of [t.uf2,t.hex])await verifyNrfImage(arrayBuffer(await readFile(new URL(image.file,root))),image,t,t.softdevice,webcrypto);
  }
});
