// Host Chromium reference diagnostic. Built-in WebSocket requires Node >=22.
// Never used as a companion server or as evidence of Xbox compatibility.
import {spawn} from 'node:child_process';
import {mkdtempSync, rmSync, writeFileSync, mkdirSync, existsSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {setTimeout as delay} from 'node:timers/promises';

const selfTest = process.argv.includes('--self-test');
const out = process.argv.find(a => a.startsWith('--output='))?.slice(9);
if (!out || existsSync(out)) throw Error('Supply --output=NEW_DIRECTORY');
mkdirSync(out, {recursive:true, mode:0o700});
const profile = mkdtempSync(join(tmpdir(), 'leanback-chromium-'));
const args = ['--headless', '--disable-dev-shm-usage', '--no-first-run',
  '--remote-debugging-port=0', `--user-data-dir=${profile}`, 'about:blank'];
const proxy = process.env.HTTPS_PROXY || process.env.https_proxy;
if (proxy) args.unshift(`--proxy-server=${proxy}`);
// Explicit opt-in for locked-down disposable containers lacking Chromium's sandbox.
if (process.argv.includes('--container-no-sandbox')) args.unshift('--no-sandbox');
const browser = spawn(process.env.CHROMIUM || 'chromium', args, {stdio:['ignore','ignore','pipe']});
let ws;
const report = {platform:'host-only', selfTest, renderingVerified:false, playbackVerified:false,
  errors:[], networkFailures:[], httpErrors:[]};
const scrub = s => String(s).replace(/https?:\/\/[^\s"')]+/g, '[URL]').slice(0,500);
try {
  let stderr = '';
  const endpoint = await new Promise((resolve,reject) => {
    const timer = setTimeout(()=>reject(Error('No DevTools endpoint within 15s')),15000);
    browser.on('error',reject);
    browser.on('exit',code=>{clearTimeout(timer);reject(Error(`Chromium exited ${code}: ${scrub(stderr)}`));});
    browser.stderr.on('data',chunk=>{
      stderr = (stderr+chunk).slice(-12000);
      const match = stderr.match(/DevTools listening on (ws:\/\/[^\s]+)/);
      if(match) {clearTimeout(timer);resolve(match[1]);}
    });
  });
  ws = new WebSocket(endpoint);
  await new Promise((resolve,reject)=>{ws.onopen=resolve;ws.onerror=reject;});
  let seq=0;
  const pending=new Map();
  ws.onmessage = e => {
    const m=JSON.parse(e.data);
    if(m.id) {const p=pending.get(m.id);if(p){pending.delete(m.id);m.error?p.reject(Error(JSON.stringify(m.error))):p.resolve(m.result);}return;}
    if(m.method==='Runtime.exceptionThrown') report.errors.push(scrub(m.params.exceptionDetails.exception?.description || m.params.exceptionDetails.text));
    if(m.method==='Network.loadingFailed') report.networkFailures.push({type:m.params.type,error:m.params.errorText});
    if(m.method==='Network.responseReceived' && m.params.response.status>=400)
      report.httpErrors.push({host:new URL(m.params.response.url).hostname,status:m.params.response.status,type:m.params.type});
  };
  function call(method,params={},sessionId) {
    return new Promise((resolve,reject)=>{
      const id=++seq;
      const timer=setTimeout(()=>{pending.delete(id);reject(Error(`CDP timeout: ${method}`));},10000);
      pending.set(id,{resolve:r=>{clearTimeout(timer);resolve(r);},reject:r=>{clearTimeout(timer);reject(r);}});
      ws.send(JSON.stringify({id,method,params,...(sessionId?{sessionId}:{})}));
    });
  }
  report.browser=await call('Browser.getVersion');
  const {targetId}=await call('Target.createTarget',{url:'about:blank'});
  const {sessionId}=await call('Target.attachToTarget',{targetId,flatten:true});
  const page=(m,p={})=>call(m,p,sessionId);
  await page('Page.enable'); await page('Runtime.enable'); await page('Network.enable');
  if(process.argv.includes('--tv')) await page('Network.setUserAgentOverride',{userAgent:
    'Mozilla/5.0 (X11; Linux x86_64) Cobalt/27.lts.3.1040976-gold (unlike Gecko) v8/13.8.258.54-jit gles Starboard/18, LeanbackProbe_DESKTOP_x8664_2026/0.1'});
  report.navigation=await page('Page.navigate',{url:selfTest?'about:blank':'https://www.youtube.com/tv'});
  if(selfTest) {
    await page('Page.setDocumentContent',{frameId:report.navigation.frameId,
      html:'<!doctype html><title>Probe fixture</title><h1>Probe fixture</h1>'});
    await page('Runtime.evaluate',{expression:'setTimeout(()=>{throw Error("fixture exception")},100)'});
  }
  await delay(selfTest?1000:20000);
  const result=await page('Runtime.evaluate',{returnByValue:true,expression:`JSON.stringify({
    origin:location.origin,title:document.title,readyState:document.readyState,
    elements:document.getElementsByTagName('*').length,videoElements:document.querySelectorAll('video').length,
    apis:Object.fromEntries(['Promise','fetch','WebAssembly','MediaSource','SourceBuffer','WebSocket','Worker','URL','TextDecoder'].map(k=>[k,typeof globalThis[k]])),
    indexedDB:typeof indexedDB,serviceWorker:typeof navigator.serviceWorker,
    eme:typeof navigator.requestMediaKeySystemAccess,gamepad:typeof navigator.getGamepads,
    codecs:['video/mp4; codecs="avc1.42E01E"','audio/mp4; codecs="mp4a.40.2"','video/webm; codecs="vp9"','video/mp4; codecs="av01.0.04M.08"'].map(type=>({type,mse:!!globalThis.MediaSource && MediaSource.isTypeSupported(type),video:document.createElement('video').canPlayType(type)}))
  })`});
  report.features=JSON.parse(result.result.value);
  report.featureContext=report.navigation.errorText?'browser-error-document':selfTest?'local-fixture':'unverified-remote-document';
  if(selfTest && (!report.errors.some(e=>e.includes('fixture exception')) || report.features.title!=='Probe fixture' || report.features.elements<4)) throw Error('Fixture did not execute as expected');
  report.instrumentationPassed=selfTest;
  // These counters and feature detections never assert Leanback usability.
} catch(e) { report.harnessError=scrub(e.message);process.exitCode=2; }
finally {
  ws?.close(); browser.kill('SIGTERM');
  await Promise.race([new Promise(r=>browser.once('exit',r)),delay(2000)]);
  if(browser.exitCode===null) browser.kill('SIGKILL');
  rmSync(profile,{recursive:true,force:true});
  writeFileSync(join(out,'browser.json'),JSON.stringify(report,null,2)+'\n');
  console.log(JSON.stringify(report,null,2));
}
if(!selfTest && (report.navigation?.errorText || report.networkFailures.length)) process.exitCode=2;
