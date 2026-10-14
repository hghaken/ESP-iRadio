#include "web.h"

#include <DNSServer.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

#include "battery.h"
#include "config.h"
#include "display.h"
#include "encoder.h"
#include "player.h"
#include "power.h"
#include <SD.h>
#include "sdcard.h"
#include "settings.h"
#include "version.h"
#include "weblog.h"
#include <esp_ota_ops.h>
#include <esp_system.h>

static WebServer server(80);
static DNSServer dns;
static bool captive = false;
static uint32_t restartAt = 0;

static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP iRadio</title>
<style>
:root{--bg:#f4f4f6;--card:#fff;--fg:#1d1d22;--mut:#6b6b76;--acc:#c62828;--line:#ddd;--rng:#1a73e8;--trk:#d3d3d8}
@media (prefers-color-scheme:dark){:root{--bg:#141418;--card:#1f1f25;--fg:#ececf1;--mut:#9a9aa5;--acc:#ef5350;--line:#33333b;--rng:#4c8dff;--trk:#3a3a44}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.4 system-ui,sans-serif}
main{max-width:960px;margin:0 auto;padding:16px}
.sub{font-weight:600;margin:14px 0 2px}h1{font-size:20px;margin:4px 0 12px}h2{font-size:15px;margin:0 0 10px;color:var(--mut);font-weight:600}
section{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin-bottom:12px}
.big{font-size:19px;font-weight:600}.mut{color:var(--mut)}.err{color:var(--acc)}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:8px}
select,input,textarea,button{font:inherit;color:inherit;background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:7px 9px}
select,input[type=text],input[type=password]{flex:1;min-width:0}input[type=number]{width:6.5em}textarea{width:100%;max-width:100%;resize:vertical;min-height:160px;font-family:monospace;white-space:pre;overflow-x:auto;overflow-wrap:normal;font-size:13px}
button{cursor:pointer;background:var(--acc);border-color:var(--acc);color:#fff;font-weight:600}
button.sec{background:transparent;color:var(--fg);border-color:var(--line)}
input[type=range]{flex:1;-webkit-appearance:none;appearance:none;height:6px;padding:0;border:0;border-radius:3px;background:linear-gradient(to right,var(--rng) var(--p,0%),var(--trk) var(--p,0%));outline:none;margin:8px 0}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:var(--rng);border:0;cursor:pointer}
input[type=range]::-moz-range-thumb{width:18px;height:18px;border-radius:50%;background:var(--rng);border:0;cursor:pointer}.pill{display:inline-block;padding:1px 8px;border-radius:99px;border:1px solid var(--line);font-size:13px}
</style></head><body><main>
<h1>ESP iRadio</h1>
<section><h2>Now playing</h2>
<div class="big" id="station">-</div><div id="title" class="mut">&nbsp;</div>
<div class="row"><span class="pill" id="state">-</span><span class="pill" id="bt">BT -</span><span class="pill" id="buf">buffer -</span></div>
<div id="error" class="err"></div>
<div class="row"><select id="sel"></select><button onclick="post('/api/play','station='+sel.value)">Play</button><button class="sec" onclick="post('/api/stop','')">Stop</button></div>
<div class="row"><span>Volume</span><input type="range" id="vol" min="0" max="100" oninput="volv.textContent=this.value" onchange="post('/api/volume','v='+this.value)"><span id="volv"></span></div>
<div class="mut" style="margin-top:14px">Stations, one per line: <code>Name|URL</code> (AAC or MP3, http, max. 60).</div>
<textarea id="stations" wrap="off" spellcheck="false"></textarea>
<div class="row"><button onclick="post('/api/stations','text='+encodeURIComponent(stations.value)).then(load)">Save</button></div>
</section>
<section><h2>Bluetooth devices</h2>
<div id="btdevs" class="mut">-</div>
<div class="row"><label><input type="checkbox" id="btauto" onchange="post('/api/btauto','on='+(this.checked?'1':'0'))"> Automatic: try the top device first, if it is off the next one</label></div>
<div class="mut" style="margin-top:6px">Only one device plays at a time. Automatic: you set the order with ↑; if the connection drops, it tries the list again from the top. If it is already playing on the speaker, it does not switch by itself when you turn on the headphones afterwards (use Try again then). Not automatic: it always uses the top device; Use puts a device at the top and restarts the radio (about 30 s).</div>
<div class="sub">Pair new device</div>
<div class="row"><input type="text" id="btname" placeholder="e.g. JBL Charge 5"><button onclick="btPair()">Pair new device</button></div>
<div class="mut">First put the device in pairing mode (JBL: hold the Bluetooth button until the light flashes quickly; Sony: power button ~7 s). The radio restarts, searches, connects to the first device whose name starts like this, and remembers it. The other devices stay paired.</div>
<div class="mut" style="margin-top:8px">Devices found: <span id="devs">-</span></div>
<div class="row"><label><input type="checkbox" id="coexwifi" onchange="post('/api/coex','wifi='+(this.checked?'1':'0'))"> Test: WiFi priority while streaming (until the next restart)</label><span class="mut" id="coexstat"></span></div>
<div class="row"><label><input type="checkbox" id="btlead" onchange="post('/api/btlead','on='+(this.checked?'1':'0'))"> Radio leads the Bluetooth link (no role switch; keeps WiFi steady next to some speakers)</label></div>
<div class="row"><label><input type="checkbox" id="bten" onchange="post('/api/btenable','on='+(this.checked?'1':'0'))"> Bluetooth on (connect to the chosen device)</label></div>
<div class="row"><label><input type="checkbox" id="line32" onchange="post('/api/line32','on='+(this.checked?'1':'0'))"> Line output: 32-bit I2S slots (off = 16-bit)</label></div>
<div class="mut">With Bluetooth off the radio plays through the line output (PCM5102A DAC).</div>
<div class="mut" style="margin-top:8px">Not connecting? Try again turns WiFi off for 30 seconds and calls the chosen device. Nothing is erased.</div>
<div class="row"><button class="sec" onclick="if(confirm('WiFi goes off for 30 seconds. The radio tries the chosen device again. Nothing is erased. The page will be gone for a moment. Continue?'))post('/api/btretry','')">Try again</button></div>
<div class="mut">Last resort: erase all pairings. This erases all devices on the radio and restarts; the Bluetooth address stays the same. Also remove ESP iRadio on the devices themselves (Sony app, JBL: clear the pairing memory).</div>
<div class="row"><button class="sec" onclick="if(confirm('Erase all Bluetooth pairings on the radio and restart? Every device will have to be paired again afterwards. Continue?'))post('/api/btforget','')">Erase all pairings</button></div>
</section>
<section><h2>WiFi</h2>
<div class="mut" id="wifi">-</div>
<div class="row"><select id="ssids"><option value="">Choose network</option></select><button class="sec" onclick="scan()">Search</button></div>
<div class="row" id="roamto" style="display:none"><button class="sec" onclick="roamTo()">Switch to this access point</button><span class="mut">Pick another access point of your own network; the page will be gone for a few seconds.</span></div>
<div class="row"><label><input type="checkbox" id="autoscan" onchange="post('/api/autoscan','on='+(this.checked?'1':'0'))"> Search networks when the page opens</label></div>
<div class="mut">Off by default. When on, the sound is interrupted for a few seconds each time this page is opened. In setup mode it always searches.</div>
<div class="row"><input type="text" id="ssid" placeholder="Network name"><input type="password" id="pass" placeholder="Password"></div>
<div class="row"><button onclick="if(confirm('Save and restart?'))post('/api/wifi','ssid='+encodeURIComponent(ssid.value)+'&pass='+encodeURIComponent(pass.value))">Save</button></div>
<div class="mut" style="margin-top:8px">Mesh: below this threshold the radio looks for a stronger access point of the same network and switches if it is at least 12 dB better. One minute after each connection it also checks once (at most every half hour). 0 = off. Only while the headphones are connected. The search interrupts the sound for a few seconds.</div>
<div class="row"><input type="number" id="roam" min="-90" max="0" step="1"><span class="mut">dBm</span><button class="sec" onclick="post('/api/roam','dbm='+encodeURIComponent($('roam').value))">Save threshold</button></div>
</section>
<section><h2>Diagnostics</h2><div style="overflow-x:auto"><table id="diag" style="border-collapse:collapse;width:100%;font-size:14px;white-space:nowrap"></table></div></section>
<section><h2>Log</h2>
<div class="row"><label><input type="checkbox" id="logon" onchange="logToggle(this.checked)"> Show live log</label><span class="mut" id="logstat"></span></div>
<div id="logui" style="display:none"><pre id="logbox" style="margin:8px 0 0;min-height:200px;max-height:60vh;overflow:auto;background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:8px;font:12px/1.35 monospace;white-space:pre"></pre>
<div class="row"><button class="sec" onclick="$('logbox').textContent=''">Clear window</button><button class="sec" onclick="logDownload()">Download log</button></div></div>
<div class="mut" style="margin-top:8px">Everything the radio also reports on the USB port. The radio always keeps the last ~64 kB (including from before the page was opened); after a restart it is gone. During a search window (WiFi off for 30 s) the window stands still; those lines show up afterwards.</div></section>
<section><h2>SD card</h2><div class="mut" id="sdinfo">-</div>
<div class="sub">Station logo</div>
<div class="row"><select id="logost" onchange="logoShow()"></select></div>
<div class="row"><canvas id="logoprev" width="64" height="64" style="width:96px;height:96px;image-rendering:pixelated;border:1px solid var(--line);border-radius:6px;background:#000;flex:0 0 auto"></canvas><input type="file" id="logofile" accept="image/*" onchange="logoPick()" style="flex:1;min-width:0"></div>
<div class="row"><button class="sec" onclick="logoSave()">Save logo</button><button class="sec" onclick="logoDel()">Delete logo</button><span class="mut" id="logostat"></span></div>
<div class="mut">Choose a picture (PNG, JPG); the browser turns it into 64x64 pixels. The logo appears on the left next to station and artist.</div>
<div class="sub">Settings</div>
<div class="row"><button class="sec" onclick="sdPost('/api/sdbackup','','sdbk')">Back up now</button><button class="sec" onclick="if(confirm('Restore the station list and settings from the card? WiFi stays as it is. The radio restarts.'))sdPost('/api/sdrestore','','sdbk')">Restore from SD</button><span class="mut" id="sdbk"></span></div>
<div class="mut" id="sdbkinfo">The radio writes stations.txt and settings.txt 10 s after every change (without the WiFi password).</div>
<div class="sub">Chart</div>
<div class="row"><select id="statday" onchange="statLoad()"></select><select id="statkey" onchange="drawChart()"><option value="battery_mV">Battery (V)</option><option value="in_kBps">Input (KB/s)</option><option value="buffer_kB">Buffer (KB)</option><option value="wifi_rssi">WiFi (dBm)</option><option value="underruns">Interruptions (total)</option><option value="bt_rssi_delta">Bluetooth signal (dB)</option><option value="heap">Free internal memory (bytes)</option></select></div>
<canvas id="chart" style="width:100%;height:200px;display:block;margin-top:8px"></canvas>
<div class="sub">Files</div>
<div id="sdfiles" class="mut">-</div>
<div class="row"><button class="sec" onclick="sdPost('/api/fwsave','','fwstat',1)">Keep current firmware</button><span class="mut" id="fwstat"></span></div>
<div class="mut">A firmware that has been running for 10 minutes is saved by the radio automatically (the newest 5). Downloading is slow next to Bluetooth.</div>
<div class="row" style="margin-top:14px"><button class="sec" onclick="sdFormat()">Format SD card</button><span class="mut" id="sdstat"></span></div>
</section>
<section><h2>System</h2><div class="mut" id="sys">-</div>
<div class="row"><label><input type="checkbox" id="flip" onchange="post('/api/flip','on='+(this.checked?'1':'0'))"> Rotate screen 180°</label></div>
<div class="row"><span>Brightness</span><input type="range" id="bl" min="0" max="100" step="5" oninput="if(+this.value<5)this.value=5;$('blv').textContent=this.value+'%'" onchange="blSave()"><span id="blv" class="mut" style="min-width:3em"></span></div>
<div class="row"><label><input type="checkbox" id="ngt" onchange="blSave()"> Night mode from</label><input type="time" id="ngtf" onchange="blSave()"><span>to</span><input type="time" id="ngtt" onchange="blSave()"><span>at</span><select id="ngtl" onchange="blSave()" style="flex:0 0 auto"><option value="0">off</option><option value="5">5%</option><option value="10">10%</option><option value="20">20%</option><option value="30">30%</option><option value="50">50%</option></select></div>
<div class="row"><span>Dim after</span><select id="idlm" onchange="blSave()" style="flex:0 0 auto"><option value="0">never</option><option value="1">1 min</option><option value="2">2 min</option><option value="5">5 min</option><option value="10">10 min</option><option value="15">15 min</option><option value="30">30 min</option><option value="60">60 min</option></select><span>to</span><select id="idll" onchange="blSave()" style="flex:0 0 auto"><option value="0">off</option><option value="5">5%</option><option value="10">10%</option><option value="20">20%</option><option value="30">30%</option><option value="50">50%</option></select></div>
<div class="mut"><span id="blnow"></span>The lowest level that applies wins. A new song, another station or a Bluetooth change makes the screen bright again, as does a change here. Night mode works once the clock is set.</div>
<div class="row"><label><input type="checkbox" id="batf" onchange="post('/api/batfitted','on='+(this.checked?'1':'0'))"> Battery fitted (off: no voltage or percentage, no low-battery shutdown)</label></div>
<div class="row"><label><input type="checkbox" id="scroll" onchange="post('/api/scroll','on='+(this.checked?'1':'0'))"> Scroll long station and title lines</label></div>
<div class="row"><button class="sec" onclick="if(confirm('Restart?'))post('/api/restart','')">Restart</button></div>
<div class="row" style="margin-top:14px"><input type="file" id="fw" accept=".bin" style="flex:1;min-width:0"><button class="sec" onclick="upload()">Firmware update</button></div>
<div class="mut" id="fwstatus">Choose <code>firmware.bin</code> (from <code>.pio/build/radio/</code>). The music stops while writing; the radio restarts afterwards.</div></section>
</main><script>
const $=id=>document.getElementById(id);let first=true;
function rangeFill(el){el.style.setProperty('--p',(el.value-el.min)/(el.max-el.min)*100+'%')}
document.addEventListener('input',e=>{if(e.target.type=='range')rangeFill(e.target)});
function post(u,b){return fetch(u,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b}).then(r=>r.text()).then(t=>{if(t&&t!='ok')alert(t);refresh()}).catch(()=>alert('The radio did not answer, so nothing was done. While no Bluetooth device is connected it turns WiFi off for up to 30 s to search; try again in a moment.'))}
function refresh(){fetch('/api/status').then(r=>r.json()).then(s=>{
 $('station').textContent=s.stationName||'No station';$('title').textContent=s.title||' ';
 $('state').textContent=s.state+(s.codec?' '+s.codec:'')+(s.muted?' - muted':'');$('bt').textContent='BT '+(s.bt?'connected':s.btEnabled?'not connected':'off');
 $('buf').textContent='buffer '+s.buffer+'%';$('error').textContent=s.error||'';
 $('devs').textContent=s.devices.length?s.devices.join(', '):(s.bt?s.btName+' (connected, not searched)':'- (filled while searching)');
 $('wifi').textContent=s.wifi.mode=='ap'?'Setup mode (access point). Choose your network below.':'Connected to '+s.wifi.ssid+' ('+s.wifi.ip+', '+s.wifi.rssi+' dBm'+(s.diag.channel?', channel '+s.diag.channel:'')+(s.diag.bssid?', ..'+s.diag.bssid.slice(-5):'')+(s.wifi.roam<0?', roam below '+s.wifi.roam+' dBm':'')+')';
 const d=s.diag,rows=[
  ['Stream input',d.inKBps+' KB/s ('+(s.codec=='MP3'?'320 kbps needs 40':'96 kbps needs 12')+')'],
  ['Output',s.lineOut?'line out (PCM5102A, I2S)':'Bluetooth'],
  ['Audio pull',d.btKBps+' KB/s (176 = real time)'+(d.silenceKBps?', silence padding '+d.silenceKBps+' KB/s':'')],
  ['Network buffer',d.bufferKB+' / '+d.bufferMaxKB+' kB (~'+(s.codec=='MP3'?Math.round(d.bufferKB/40):Math.round(d.bufferKB/12))+' s)'],
  ['PCM buffer',d.pcmKB+' kB'],
  ['Decoder',(s.codec||'-')+(s.sampleRate?' '+s.sampleRate+' Hz'+(s.sampleRate!=44100?' (resampled to 44.1 kHz)':''):'')],
  ['Interruptions',d.underruns+' buffer empty, '+d.reconnects+' reconnects'],
  ['WiFi',s.wifi.mode=='ap'?'setup mode':s.wifi.rssi+' dBm, access point '+d.bssid+', channel '+d.channel+(s.wifi.roam<0?', threshold '+s.wifi.roam+' dBm':'')],
  ['Memory','internal '+Math.round(s.heap/1024)+' kB free (min '+Math.round(d.minHeap/1024)+'), PSRAM '+Math.round(s.psram/1024)+' kB (min '+Math.round(d.psramMin/1024)+')'],
  ['Battery',(s.batteryMv?(s.batteryMv/1000).toFixed(2)+' V, '+s.batteryPct+'%':s.batFitted?'no reading':'not fitted')+' (pin '+d.batRawMv+' mV), '+(s.usbMv?'USB '+(s.usbMv/1000).toFixed(2)+' V':'no USB')+' (pin '+d.pwrRawMv+' mV)'],
  ['Bluetooth',(s.bt?'connected, signal '+(d.btRssiDelta==127?'?':(d.btRssiDelta>=-5?'good':d.btRssiDelta>=-15?'fair':'weak')+' ('+d.btRssiDelta+' dB vs. ideal)')+(d.btRole==0?', radio leads the link':d.btRole==1?', the device leads the link':''):'not connected')+', identity '+d.btIdentity+', pairings on the radio: '+(d.btBonds<0?'-':d.btBonds)+(d.btRefreshes?', reconnected '+d.btRefreshes+'x for WiFi':'')],
  ['Firmware',(d.version||'')+', built '+d.build+', running from slot '+d.partition],
  ['SD card',d.sd||'-'],
  ['Encoder',d.enc||'-'],
  ['Uptime',Math.floor(s.uptime/3600)+' h '+Math.floor(s.uptime%3600/60)+' min, last reset: '+({1:'power on',3:'software',4:'crash',5:'watchdog',6:'watchdog',7:'watchdog',8:'deep sleep',9:'voltage dip (brownout)'}[d.resetReason]||('code '+d.resetReason))+(d.brownouts?', '+d.brownouts+'x brownout in a row':'')]];
 $('diag').innerHTML=rows.map(r=>'<tr><td class="mut" style="padding:3px 10px 3px 0;white-space:nowrap;vertical-align:top">'+r[0]+'</td><td style="padding:3px 0">'+r[1]+'</td></tr>').join('');
 if(s.backlight)$('blnow').textContent='Now '+s.backlight.now+'%. ';$('sys').textContent=(s.batteryMv?'Battery '+(s.batteryMv/1000).toFixed(2)+' V ('+s.batteryPct+'%)':s.batFitted?'Battery: no reading (jumper open?)':'No battery fitted')+(s.usb?', USB power present':'')+'. Uptime '+Math.floor(s.uptime/60)+' min, heap '+Math.round(s.heap/1024)+' kB free (min '+Math.round(s.minHeap/1024)+' kB), PSRAM '+Math.round(s.psram/1024)+' kB free';
 if(first){first=false;stList=s.stations;lastSt=s.station>=0?s.station:s.last;setTimeout(sdRefresh,300);const sel=$('sel');sel.innerHTML='';s.stations.forEach((n,i)=>{const o=document.createElement('option');o.value=i;o.textContent=n;sel.appendChild(o)});
  sel.value=s.station>=0?s.station:s.last;$('vol').value=s.volume;$('volv').textContent=s.volume;$('ssid').value=s.wifi.ssid||'';$('roam').value=s.wifi.roam;$('autoscan').checked=!!s.wifi.autoScan;$('bten').checked=!!s.btEnabled;$('line32').checked=!!s.line32;$('batf').checked=!!s.batFitted;$('btlead').checked=!!s.btLead;$('flip').checked=!!s.flip;$('scroll').checked=!!s.scroll;const b=s.backlight;$('bl').value=b.b;$('blv').textContent=b.b+'%';$('ngt').checked=!!b.night;$('ngtf').value=b.from;$('ngtt').value=b.to;$('ngtl').value=b.nl;$('idlm').value=b.idle;$('idll').value=b.il}
 {const c=s.station>=0?s.station:s.last;if(c!=selShown&&document.activeElement!==$('sel'))$('sel').value=c;selShown=c}
 if(s.btDevices)btRender(s.btDevices,!!s.btAuto);if(document.activeElement!==$('coexwifi'))$('coexwifi').checked=!!s.coexWanted;$('coexstat').textContent=s.coexWanted?(s.coexActive?'active':'waiting for streaming'):'';document.querySelectorAll('input[type=range]').forEach(rangeFill);considerAutoScan(s);
})}
function load(){first=true;fetch('/api/stations').then(r=>r.text()).then(t=>$('stations').value=t);refresh()}
let autoScanDone=false;
function considerAutoScan(s){if(autoScanDone)return;autoScanDone=true;if(s.wifi.mode=='ap'||s.wifi.autoScan)scan()}
function upload(){const f=$('fw').files[0];if(!f){alert('Choose a firmware.bin first');return}
 if(!confirm('Install firmware '+f.name+' ('+Math.round(f.size/1024)+' kB)? The radio restarts afterwards.'))return;
 const fd=new FormData();fd.append('firmware',f);const x=new XMLHttpRequest();x.open('POST','/api/update');
 x.upload.onprogress=e=>{$('fwstatus').textContent='Uploading: '+Math.round(100*e.loaded/e.total)+'%'};
 x.onload=()=>{$('fwstatus').textContent=x.status==200?'Done, the radio is restarting...':x.responseText};
 x.onerror=()=>{$('fwstatus').textContent='Upload failed (connection)'};x.send(fd)}
function sdFormat(){if(!confirm('Format SD card (FAT32)? Only a card without a FAT file system is erased; a card that already has FAT is left as it is. A large card takes a minute or longer; the page will not respond for a while.'))return;
 const el=$('sdstat'),t0=Date.now();let done=false;el.textContent='Working... 0 s';
 const tick=setInterval(()=>{if(!done)el.textContent='Working... '+Math.round((Date.now()-t0)/1000)+' s'},1000);
 const poll=()=>fetch('/api/status',{cache:'no-store'}).then(r=>r.json()).then(s=>{const r=s.diag.sdFormat;if(r&&r!='busy'){done=true;clearInterval(tick);el.textContent=r}else setTimeout(poll,2000)}).catch(()=>setTimeout(poll,2000));
 fetch('/api/sdformat',{method:'POST'}).then(()=>setTimeout(poll,1500)).catch(()=>{done=true;clearInterval(tick);el.textContent='Radio not reachable'})}
let sd=null,statHdr=[],statRows=[],stList=[],lastSt=-1;
function kb(n){return n>=1e6?(n/1e6).toFixed(1)+' MB':Math.max(1,Math.round(n/1e3))+' kB'}
function fnv(s){let h=0x811c9dc5;for(const b of new TextEncoder().encode(s)){h^=b;h=Math.imul(h,0x01000193)>>>0}return h.toString(16).padStart(8,'0')}
function sdRefresh(){fetch('/api/sd',{cache:'no-store'}).then(r=>r.json()).then(j=>{sd=j;$('sdinfo').textContent=j.mounted?j.info+(j.time?'':' (clock not set yet)'):j.info;
 if(!j.mounted){$('sdfiles').textContent='-';return}
 $('sdbkinfo').textContent=j.backup?'Last backup: '+(j.backupTime||'?')+'. The radio writes it 10 s after every change (without the WiFi password).':'No backup on the card yet.';
 const ls=$('logost'),cur=ls.value;ls.innerHTML='';(stList||[]).forEach((n,i)=>{const o=document.createElement('option');o.value=i;o.textContent=n+(j.logos.includes(i)?'  (logo)':'');ls.appendChild(o)});ls.value=cur||(lastSt>=0?lastSt:0);logoShow();
 const sdd=$('statday'),cd=sdd.value;sdd.innerHTML='';j.stats.slice().reverse().forEach(f=>{const o=document.createElement('option');o.value=f.n;o.textContent=f.n.replace('.csv','');sdd.appendChild(o)});if(cd)sdd.value=cd;statLoad();
 const a=(dir,f)=>'<a href="/api/sdfile?p=/'+dir+'/'+encodeURIComponent(f.n)+'" download="'+f.n+'">'+f.n+'</a> ('+kb(f.s)+')';
 let h='<b>Log:</b> '+(j.log.length?j.log.slice().reverse().map(f=>a('log',f)).join(', '):'nothing yet')+'<br><b>Measurements:</b> '+(j.stats.length?j.stats.slice().reverse().map(f=>a('stats',f)).join(', '):'nothing yet')+'<br><b>Firmware:</b> ';
 h+=j.fw.length?j.fw.slice().reverse().map(f=>a('firmware',f)+(f.n==j.current?' (running now)':' <button class="sec" style="padding:2px 8px" onclick="fwBack(\''+f.n+'\')">Restore</button>')).join('<br>'):'no copy yet';
 $('sdfiles').innerHTML=h})}
function sdPost(u,b,id,poll){$(id).textContent='Working...';fetch(u,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b}).then(r=>r.text()).then(t=>{$(id).textContent=t=='ok'?'Done':t;if(poll)fwPoll(id);else sdRefresh()}).catch(()=>{$(id).textContent='Radio not reachable'})}
function fwPoll(id){const p=()=>fetch('/api/status',{cache:'no-store'}).then(r=>r.json()).then(s=>{const r=s.diag.sdFirmware;if(r&&r!='busy'){$(id).textContent=r;sdRefresh()}else{$(id).textContent='Working...';setTimeout(p,2000)}}).catch(()=>setTimeout(p,3000));setTimeout(p,1500)}
function fwBack(n){if(!confirm('Restore firmware '+n+' from the card? The music stops, the radio writes the other slot and restarts (about a minute).'))return;$('fwstat').textContent='Working, the radio will restart shortly...';fetch('/api/fwrestore',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'f='+encodeURIComponent(n)})}
function logoDraw(buf){const a=new Uint16Array(buf),n=Math.round(Math.sqrt(a.length)),t=document.createElement('canvas');t.width=t.height=n;const tx=t.getContext('2d'),im=tx.createImageData(n,n);for(let i=0;i<n*n;i++){const v=a[i];im.data[i*4]=(v>>11&31)*255/31;im.data[i*4+1]=(v>>5&63)*255/63;im.data[i*4+2]=(v&31)*255/31;im.data[i*4+3]=255}tx.putImageData(im,0,0);const x=$('logoprev').getContext('2d');x.imageSmoothingEnabled=false;x.drawImage(t,0,0,64,64)}
function logoClear(){const x=$('logoprev').getContext('2d');x.fillStyle='#000';x.fillRect(0,0,64,64)}
function logoShow(){logoClear();$('logostat').textContent='';const i=+$('logost').value;if(!sd||!sd.logos||!sd.logos.includes(i))return;fetch('/api/sdfile?p=/logos/'+fnv(stList[i])+'.565').then(r=>r.ok?r.arrayBuffer():null).then(b=>{if(b&&(b.byteLength==3200||b.byteLength==8192))logoDraw(b)})}
function logoPick(){const f=$('logofile').files[0];if(!f)return;const img=new Image();img.onload=()=>{const x=$('logoprev').getContext('2d');logoClear();const s=Math.min(64/img.width,64/img.height),w=img.width*s,h=img.height*s;x.imageSmoothingQuality='high';x.drawImage(img,(64-w)/2,(64-h)/2,w,h);URL.revokeObjectURL(img.src);$('logostat').textContent='Preview; not saved yet'};img.src=URL.createObjectURL(f)}
function logoSave(){if(!$('logofile').files[0]){$('logostat').textContent='Choose a picture first';return}const d=$('logoprev').getContext('2d').getImageData(0,0,64,64).data,a=new Uint16Array(4096);for(let i=0;i<4096;i++)a[i]=(d[i*4]>>3<<11)|(d[i*4+1]>>2<<5)|(d[i*4+2]>>3);
 const fd=new FormData();fd.append('logo',new Blob([a.buffer]),'logo.565');$('logostat').textContent='Saving...';fetch('/api/logo?i='+$('logost').value,{method:'POST',body:fd}).then(r=>r.text()).then(t=>{$('logostat').textContent=t=='ok'?'Saved':t;$('logofile').value='';sdRefresh()})}
function logoDel(){if(!confirm('Delete the logo of this station?'))return;fetch('/api/logodel',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'i='+$('logost').value}).then(r=>r.text()).then(t=>{$('logostat').textContent=t=='ok'?'Deleted':t;sdRefresh()})}
function statLoad(){const f=$('statday').value;if(!f){statRows=[];drawChart();return}fetch('/api/sdfile?p=/stats/'+f,{cache:'no-store'}).then(r=>r.text()).then(t=>{const L=t.trim().split('\n');statHdr=L.shift().split(',');statRows=L.map(l=>l.split(','));drawChart()})}
function drawChart(){const c=$('chart'),r=window.devicePixelRatio||1,W=c.clientWidth*r,H=c.clientHeight*r;c.width=W;c.height=H;const x=c.getContext('2d');x.clearRect(0,0,W,H);
 const cs=getComputedStyle(document.body),fg=cs.getPropertyValue('--fg')||'#000',mut=cs.getPropertyValue('--mut')||'#888',acc=cs.getPropertyValue('--acc')||'#c00';
 const k=statHdr.indexOf($('statkey').value),ti=statHdr.indexOf('time');x.font=(11*r)+'px sans-serif';x.fillStyle=mut;
 const pts=[];if(k>=0)statRows.forEach(row=>{const t=row[ti]||'',m=+t.substr(11,2)*60+ +t.substr(14,2)+ +t.substr(17,2)/60;let v=+row[k];if(statHdr[k]=='battery_mV'){if(!v)return;v/=1000}if(!isNaN(m)&&!isNaN(v))pts.push([m,v])});
 if(pts.length<2){x.fillText(pts.length?'Too few measurements':'No measurement data',8*r,20*r);return}
 let lo=Math.min(...pts.map(p=>p[1])),hi=Math.max(...pts.map(p=>p[1]));if(hi==lo){hi+=1;lo-=1}const t0=pts[0][0],t1=pts[pts.length-1][0]||t0+1,L=40*r,B=H-18*r,T=8*r,R=W-6*r;
 const X=m=>L+(m-t0)/(t1-t0||1)*(R-L),Y=v=>B-(v-lo)/(hi-lo)*(B-T);
 x.strokeStyle=mut;x.lineWidth=r;x.beginPath();x.moveTo(L,T);x.lineTo(L,B);x.lineTo(R,B);x.stroke();
 const f=v=>Math.abs(hi-lo)<10?v.toFixed(2):Math.round(v);x.fillText(f(hi),2*r,T+8*r);x.fillText(f(lo),2*r,B);
 const hm=m=>String(Math.floor(m/60)).padStart(2,'0')+':'+String(Math.floor(m%60)).padStart(2,'0');x.fillText(hm(t0),L,H-4*r);x.fillText(hm(t1),R-30*r,H-4*r);
 x.strokeStyle=acc;x.lineWidth=1.5*r;x.beginPath();pts.forEach((p,i)=>{i?x.lineTo(X(p[0]),Y(p[1])):x.moveTo(X(p[0]),Y(p[1]))});x.stroke()}
let btShown='',selShown=-2;  // selShown: station the dropdown last followed (buttons can change it)
function btRender(list,auto){const k=JSON.stringify(list)+auto;if(k==btShown)return;btShown=k;$('btauto').checked=auto;
 $('btdevs').innerHTML=list.length?list.map((d,i)=>'<div class="row" style="margin-top:4px"><span style="flex:1;color:var(--fg)">'+(auto?(i+1)+'. ':'')+d.n.replace(/</g,'&lt;')+' <span class="mut">('+d.a.slice(-8)+')</span>'+(d.con?' <span class="pill">connected</span>':'')+(!auto&&d.sel?' <span class="pill">chosen</span>':'')+'</span>'+(auto?(i?'<button class="sec" onclick="post(\'/api/btup\',\'i='+i+'\')">↑</button>':''):(d.sel?'':'<button class="sec" onclick="btUse('+i+')">Use</button>'))+'<button class="sec" onclick="btDel('+i+')">Forget</button></div>').join(''):'No device remembered yet.'}
function btUse(i){if(confirm('Switch to this device? The radio restarts and connects to it (about 30 s).'))post('/api/btuse','i='+i)}
function btDel(i){if(confirm('Forget this device (erase the pairing on the radio)? To use it again later it has to be paired again.'))post('/api/btdel','i='+i)}
function btPair(){const n=$('btname').value.trim();if(!n){alert('Enter the name as the device announces itself, e.g. JBL Charge 5');return}
 if(confirm('Is '+n+' in pairing mode? The radio restarts and searches for a device whose name starts with: '+n))post('/api/btpair','name='+encodeURIComponent(n))}
function blSave(){post('/api/backlight','b='+$('bl').value+'&night='+($('ngt').checked?1:0)+'&from='+$('ngtf').value+'&to='+$('ngtt').value+'&nl='+$('ngtl').value+'&idle='+$('idlm').value+'&il='+$('idll').value)}
let aps=[];
function scan(){$('ssids').innerHTML='<option>(searching...)</option>';$('roamto').style.display='none';fetch('/api/scan').then(r=>r.json()).then(l=>{const s=$('ssids');if(l&&l.error){s.innerHTML='<option>'+l.error+'</option>';return}aps=l.sort((a,b)=>b.rssi-a.rssi);s.innerHTML='<option value="">Choose network</option>';
 aps.forEach((n,i)=>{const o=document.createElement('option');o.value=i;o.textContent=n.ssid+' ('+n.rssi+' dBm, channel '+n.ch+', ..'+n.bssid.slice(-5)+')'+(n.cur?' - connected':'');s.appendChild(o)});
 s.onchange=()=>{const n=aps[s.value];$('roamto').style.display='none';if(!n)return;$('ssid').value=n.ssid;if(n.same&&!n.cur)$('roamto').style.display=''}})}
function roamTo(){const n=aps[$('ssids').value];if(n&&confirm('Switch to access point '+n.bssid+' (channel '+n.ch+', '+n.rssi+' dBm)?'))post('/api/roamto','bssid='+n.bssid+'&ch='+n.ch)}
let logOn=false,logNext=0,logTimer=null;
function logToggle(on){logOn=on;try{localStorage.setItem('iradioLog',on?'1':'0')}catch(e){}
 $('logui').style.display=on?'block':'none';clearTimeout(logTimer);if(on)logPoll();else $('logstat').textContent=''}
async function logPoll(){clearTimeout(logTimer);if(!logOn)return;let more=false;
 try{const r=await fetch('/api/log?since='+logNext,{cache:'no-store'});const t=await r.text();const b=$('logbox');
  const atEnd=b.scrollTop+b.clientHeight>=b.scrollHeight-30;let add='';
  if(r.headers.get('X-Log-Restart')=='1'&&logNext>0)add='\n[--- radio restarted ---]\n';
  else if(r.headers.get('X-Log-Skipped')=='1'&&logNext>0)add='\n[... older lines skipped ...]\n';
  add+=t;if(add){b.textContent+=add;if(b.textContent.length>400000)b.textContent=b.textContent.slice(-300000);if(atEnd)b.scrollTop=b.scrollHeight}
  const n=parseInt(r.headers.get('X-Log-Next'));if(!isNaN(n))logNext=n;more=r.headers.get('X-Log-More')=='1';$('logstat').textContent=''}
 catch(e){$('logstat').textContent='(radio briefly unreachable)'}
 logTimer=setTimeout(logPoll,more?50:1500)}
function logDownload(){fetch('/api/log?since=0&max=65536',{cache:'no-store'}).then(r=>r.text()).then(t=>{const a=document.createElement('a');
 a.href=URL.createObjectURL(new Blob([t],{type:'text/plain'}));a.download='esp-iradio-log.txt';a.click()})}
load();setInterval(refresh,5000);
try{if(localStorage.getItem('iradioLog')=='1'){$('logon').checked=true;logToggle(true)}}catch(e){}
</script></body></html>)HTML";

static String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);  // one allocation instead of one per character
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if ((uint8_t)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
    else out += c;
  }
  return out;
}

// GET /api/log?since=N[&max=M]: log text from position N (0 = oldest kept). Headers:
// X-Log-Next (position to ask for next), X-Log-More (1 = more waiting), X-Log-Skipped (part
// was overwritten), X-Log-Restart (N lies beyond the end: the radio restarted).
static void handleLog() {
  uint32_t since = server.hasArg("since") ? strtoul(server.arg("since").c_str(), nullptr, 10) : 0;
  size_t maxBytes = server.hasArg("max") ? constrain(server.arg("max").toInt(), 256, WEBLOG_BYTES) : 8192;
  char* out = (char*)ps_malloc(maxBytes);
  if (out == nullptr) {
    server.send(503, "text/plain", "out of memory");
    return;
  }
  size_t len = 0;
  bool skipped = false, restarted = false;
  uint32_t next = weblogRead(since, out, maxBytes, &len, &skipped, &restarted);
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Log-Next", String(next));
  server.sendHeader("X-Log-More", next < weblogEnd() ? "1" : "0");
  server.sendHeader("X-Log-Skipped", skipped ? "1" : "0");
  server.sendHeader("X-Log-Restart", restarted ? "1" : "0");
  server.send_P(200, "text/plain; charset=utf-8", out, len);
  free(out);
}

static void handleStatus() {
  PlayerStatus p = playerStatus();
  BatteryInfo b = batteryRead();
  bool ap = WiFi.getMode() == WIFI_AP;
  String j;
  j.reserve(2048 + 48 * settings.stations.size());  // one block (PSRAM) instead of ~45 reallocs
  j = "{";
  j += "\"state\":\"" + String(playStateName(p.state)) + "\"";
  j += ",\"station\":" + String(p.station);
  j += ",\"last\":" + String(settings.lastStation);
  j += ",\"stationName\":\"" + jsonEscape(p.stationName) + "\"";
  j += ",\"title\":\"" + jsonEscape(p.title) + "\"";
  j += ",\"codec\":\"" + p.codec + "\"";
  j += ",\"error\":\"" + jsonEscape(p.error) + "\"";
  j += ",\"buffer\":" + String(p.bufferPercent);
  j += ",\"sampleRate\":" + String(p.sampleRate);
  j += ",\"diag\":{\"inKBps\":" + String(p.inKBps) + ",\"btKBps\":" + String(p.btKBps);
  j += ",\"silenceKBps\":" + String(p.silenceKBps) + ",\"bufferKB\":" + String(p.bufferKB);
  j += ",\"bufferMaxKB\":" + String(NET_BUFFER_BYTES / 1024) + ",\"pcmKB\":" + String(p.pcmBufferKB);
  j += ",\"underruns\":" + String(p.underruns) + ",\"reconnects\":" + String(p.reconnects);
  j += ",\"bitrateKbps\":" + String(p.bitrateKbps) + ",\"btRssiDelta\":" + String(p.btRssiDelta) + ",\"btBonds\":" + String(p.btBonds) + ",\"btRole\":" + String(p.btRole) +
       ",\"btRefreshes\":" + String(playerBtRefreshes());
  j += ",\"bssid\":\"" + (ap ? String("") : WiFi.BSSIDstr()) + "\",\"channel\":" + String(ap ? 0 : WiFi.channel());
  j += ",\"batRawMv\":" + String(b.rawBatMv) + ",\"pwrRawMv\":" + String(b.rawPwrMv);
  j += ",\"minHeap\":" + String(ESP.getMinFreeHeap()) + ",\"psramMin\":" + String(ESP.getMinFreePsram());
  j += ",\"btIdentity\":" + String(settings.btIdentity);
  j += ",\"enc\":\"" + jsonEscape(encoderStatus()) + "\"";
  j += ",\"sd\":\"" + jsonEscape(sdInfo()) + "\",\"sdFormat\":\"" + jsonEscape(sdFormatResult()) + "\"";
  j += ",\"sdFirmware\":\"" + jsonEscape(sdFirmwareResult()) + "\"";
  j += ",\"resetReason\":" + String((int)esp_reset_reason()) + ",\"brownouts\":" + String(powerBrownouts());
  const esp_partition_t* part = esp_ota_get_running_partition();
  // __DATE__/__TIME__ come from this translation unit, so this line stays in the
  // build when only the A2DP library patch changes and Diagnostics must show a new time.
  j += ",\"partition\":\"" + String(part ? part->label : "?") + "\",\"build\":\"" + String(buildStamp) +
       "\",\"version\":\"" + String(firmwareVersion) + "\"}";
  j += ",\"bt\":" + String(p.btConnected ? "true" : "false");
  j += ",\"btEnabled\":" + String(p.btEnabled ? "true" : "false");
  j += ",\"lineOut\":" + String(p.lineOut ? "true" : "false");
  j += ",\"line32\":" + String(settings.line32 ? "true" : "false");
  j += ",\"btLead\":" + String(settings.btLead ? "true" : "false");
  j += ",\"coexWanted\":" + String(playerCoexWifiWanted() ? "true" : "false") + ",\"coexActive\":" +
       String(playerCoexWifiActive() ? "true" : "false");
  j += ",\"flip\":" + String(settings.displayFlip ? "true" : "false");
  j += ",\"scroll\":" + String(settings.scrollTitles ? "true" : "false");
  j += ",\"muted\":" + String(playerMuted() ? "true" : "false");
  char from[6], to[6];
  snprintf(from, sizeof(from), "%02d:%02d", settings.nightFrom / 60, settings.nightFrom % 60);
  snprintf(to, sizeof(to), "%02d:%02d", settings.nightTo / 60, settings.nightTo % 60);
  j += ",\"backlight\":{\"b\":" + String(settings.backlight) + ",\"night\":" + String(settings.nightDim ? "true" : "false") +
       ",\"from\":\"" + from + "\",\"to\":\"" + to + "\",\"nl\":" + String(settings.nightLevel) +
       ",\"idle\":" + String(settings.idleMinutes) + ",\"il\":" + String(settings.idleLevel) +
       ",\"now\":" + String(displayBacklightPercent()) + "}";
  j += ",\"btName\":\"" + jsonEscape(settings.btName) + "\"";
  j += ",\"volume\":" + String(settings.volume);
  j += ",\"batteryMv\":" + String(b.millivolts);
  j += ",\"batteryPct\":" + String(b.percent);
  j += ",\"batFitted\":" + String(settings.batteryFitted ? "true" : "false");
  j += ",\"usb\":" + String(b.usb ? "true" : "false");
  j += ",\"usbMv\":" + String(b.usbMv);
  j += ",\"uptime\":" + String(millis() / 1000);
  j += ",\"heap\":" + String(ESP.getFreeHeap());
  j += ",\"minHeap\":" + String(ESP.getMinFreeHeap());
  j += ",\"psram\":" + String(ESP.getFreePsram());
  j += ",\"wifi\":{\"mode\":\"" + String(ap ? "ap" : "sta") + "\"";
  j += ",\"ssid\":\"" + jsonEscape(settings.wifiSsid) + "\"";  // the password is never sent
  j += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  j += ",\"rssi\":" + String(ap ? 0 : WiFi.RSSI());
  j += ",\"roam\":" + String(settings.wifiRoamDbm);
  j += ",\"autoScan\":" + String(settings.wifiAutoScan ? "true" : "false") + "}";
  j += ",\"stations\":[";
  for (size_t i = 0; i < settings.stations.size(); i++)
    j += String(i ? "," : "") + "\"" + jsonEscape(settings.stations[i].name) + "\"";
  j += "],\"btDevices\":[";
  std::vector<BtDeviceInfo> known = playerBtDevices();
  for (size_t i = 0; i < known.size(); i++)
    j += String(i ? "," : "") + "{\"n\":\"" + jsonEscape(known[i].name) + "\",\"a\":\"" + known[i].addr +
         "\",\"sel\":" + (known[i].selected ? "true" : "false") + ",\"con\":" + (known[i].connected ? "true" : "false") + "}";
  j += "],\"btAuto\":" + String(playerBtAuto() ? "true" : "false");
  j += ",\"devices\":[";
  std::vector<String> devs = playerSeenDevices();
  for (size_t i = 0; i < devs.size(); i++) j += String(i ? "," : "") + "\"" + jsonEscape(devs[i]) + "\"";
  j += "]}";
  server.send(200, "application/json", j);
}

static void handlePlay() {
  int i = server.arg("station").toInt();
  if (i < 0 || i >= (int)settings.stations.size()) { server.send(400, "text/plain", "Unknown station"); return; }
  settings.lastStation = i;
  settings.autoPlay = true;
  settingsSave();
  playerPlay(i);
  server.send(200, "text/plain", "ok");
}

static void handleStop() {
  settings.autoPlay = false;
  settingsSave();
  playerStop();
  server.send(200, "text/plain", "ok");
}

static void handleVolume() {
  settings.volume = constrain(server.arg("v").toInt(), 0, 100);
  settingsSaveVolume();
  playerSetVolume(settings.volume);
  if (playerMuted()) playerSetMuted(false);  // moving the volume means: let me hear it
  server.send(200, "text/plain", "ok");
}

static void handleStations() {
  std::vector<Station> list;
  if (!settingsParseStations(server.arg("text"), list)) {
    server.send(400, "text/plain", "No valid station found (format: Name|http://...)");
    return;
  }
  // Keep the current station across the edit: find it again in the new list by URL,
  // else by name, else fall back to the first entry. Resume playback if it was on.
  String curUrl, curName;
  if (settings.lastStation >= 0 && settings.lastStation < (int)settings.stations.size()) {
    curUrl = settings.stations[settings.lastStation].url;
    curName = settings.stations[settings.lastStation].name;
  }
  playerStop();  // the player reads the list; stop before replacing it
  delay(300);
  settings.stations = list;
  int idx = 0, byName = -1;
  for (int i = 0; i < (int)list.size(); i++) {
    if (list[i].url == curUrl) { idx = i; byName = -1; break; }
    if (byName < 0 && list[i].name == curName) byName = i;
  }
  if (byName >= 0) idx = byName;
  settings.lastStation = idx;
  settingsSave();
  if (settings.autoPlay) playerPlay(idx);  // was playing: carry on with the same station
  server.send(200, "text/plain", "ok");
}

static void handleBt() {
  String name = server.arg("name");
  name.trim();
  if (name.isEmpty()) { server.send(400, "text/plain", "Name is empty"); return; }
  settings.btName = name;
  settingsSave();
  server.send(200, "text/plain", "ok");
  restartAt = millis() + 1000;
}

static void handleBtRetry() {
  if (captive) {
    server.send(400, "text/plain", "Bluetooth is off in setup mode.");
    return;
  }
  if (playerBtConnected()) {
    server.send(200, "text/plain", "The headphones are already connected. Nothing was erased.");
    return;
  }
  playerRequestQuietPage();
  server.send(200, "text/plain", "WiFi goes off for 30 seconds. The page will be gone for a moment; nothing is erased.");
}

// Use another remembered device: select it and restart (the boot pages it directly).
static void handleBtUse() {
  int i = server.arg("i").toInt();
  std::vector<BtDeviceInfo> list = playerBtDevices();
  if (captive || i < 0 || i >= (int)list.size() || !playerBtUseDevice(i)) {
    server.send(400, "text/plain", "Unknown device");
    return;
  }
  settings.btName = list[i].name;  // discovery fallback matches this device too
  settingsSave();
  server.send(200, "text/plain", "ok");
  restartAt = millis() + 1000;
}

// Pair a new device by name: no selection, the boot discovers it, it is remembered on connect.
static void handleBtPair() {
  String name = server.arg("name");
  name.trim();
  if (captive || name.isEmpty()) { server.send(400, "text/plain", "Name is empty"); return; }
  playerBtPairNew();
  settings.btName = name;
  settingsSave();
  server.send(200, "text/plain", "ok");
  restartAt = millis() + 1000;
}

static void handleBtDel() {
  bool wasSelected = playerBtForgetDevice(server.arg("i").toInt());
  server.send(200, "text/plain", "ok");
  if (wasSelected) restartAt = millis() + 1000;  // its link was dropped, or the list is empty
}

static void handleBtForget() {
  playerForgetHeadphones();
  server.send(200, "text/plain", "ok");
  restartAt = millis() + 1000;
}

// Firmware update over WiFi: multipart upload of firmware.bin into the inactive OTA
// slot (Update.h), then a clean restart. Used by the web page and by
// `pio run -t upload` (curl, see platformio.ini).
static bool updateOk = false;
static volatile uint32_t uploadActivity = 0;  // millis() of the last upload chunk, 0 = no upload

bool webUploadActive() { return uploadActivity != 0; }

// The WebServer waits forever for the next byte of an upload whose peer silently
// vanished (WiFi hiccup mid-upload; seen twice with Bluetooth audio active): the
// loop task hangs, no web page, no restart. This guard restarts the radio when an
// upload makes no progress for 30 s.
static void otaGuardTask(void*) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (uploadActivity && millis() - uploadActivity > 30000) {
      Log.println("[ota] upload stalled for 30 s, restarting");
      Update.abort();
      ESP.restart();
    }
  }
}

static uint32_t updateTotal = 0;

static void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    Log.printf("[ota] start: %s\n", up.filename.c_str());
    // Free the radio for the transfer: with A2DP audio streaming the WiFi link is
    // too weak for a 1.8 MB upload. The headphones reconnect after the restart.
    playerShutdown();
    // Without this the library heartbeat pages the Bluetooth device every 10 s and loop()
    // opens a paging window (WiFi off for 30 s) once the link has been down for 15 s: the
    // upload then stalled at ~23 % (2026-10-10). Resumed below when the update fails.
    playerHoldBluetooth(true);
    uploadActivity = millis();
    updateTotal = server.clientContentLength();  // file plus ~200 bytes of multipart framing
    displaySetUpdate(UpdatePhase::Receiving, 0, updateTotal, "via WiFi");
    updateOk = Update.begin(UPDATE_SIZE_UNKNOWN);
    if (!updateOk) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_WRITE && updateOk) {
    uploadActivity = millis();
    if (Update.write(up.buf, up.currentSize) != up.currentSize) {
      Update.printError(Serial);
      updateOk = false;
    }
    displaySetUpdate(UpdatePhase::Receiving, up.totalSize + up.currentSize, updateTotal, "via WiFi");
  } else if (up.status == UPLOAD_FILE_END && updateOk) {
    updateOk = Update.end(true);
    if (updateOk) Log.printf("[ota] done: %u bytes\n", up.totalSize);
    else Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    updateOk = false;
    Log.println("[ota] aborted");
  }
}

static void handleUpdateDone() {
  uploadActivity = 0;
  if (updateOk) {
    displaySetUpdate(UpdatePhase::Done, Update.progress(), Update.progress(), "via WiFi");
    server.send(200, "text/plain", "ok");
    restartAt = millis() + 1500;
  } else {
    displaySetUpdate(UpdatePhase::Failed, 0, 0, "via WiFi");
    playerHoldBluetooth(false);
    String err = Update.errorString();
    server.send(500, "text/plain", "Update failed: " + err);
    playerPlay(settings.lastStation);  // resume what was playing
  }
}

static void handleRoam() {
  if (!server.hasArg("dbm")) { server.send(400, "text/plain", "Threshold missing"); return; }
  int dbm = server.arg("dbm").toInt();
  if (dbm != 0 && (dbm < -90 || dbm > -40)) {
    server.send(400, "text/plain", "Threshold must be 0 (off), or between -90 and -40 dBm.");
    return;
  }
  settings.wifiRoamDbm = dbm;
  settingsSave();
  Log.printf("[wifi] roam threshold %d dBm\n", dbm);
  server.send(200, "text/plain", "ok");
}

static void handleAutoScan() {
  String on = server.arg("on");
  if (on != "0" && on != "1") {
    server.send(400, "text/plain", "Choose 1 (on) or 0 (off).");
    return;
  }
  settings.wifiAutoScan = on == "1";
  settingsSave();
  Log.printf("[wifi] page auto-scan %s\n", settings.wifiAutoScan ? "on" : "off");
  server.send(200, "text/plain", "ok");
}

static void handleWifi() {
  String ssid = server.arg("ssid");
  ssid.trim();
  if (ssid.isEmpty()) { server.send(400, "text/plain", "Network name is empty"); return; }
  settings.wifiSsid = ssid;
  settings.wifiPass = server.arg("pass");
  settingsSave();
  server.send(200, "text/plain", "ok");
  restartAt = millis() + 1000;
}

static void handleScan() {
  // An all-channel scan takes the shared radio. While the headphones are still
  // connecting that is the radio the page needs. With Bluetooth switched off (or in
  // setup mode) there is no link to protect, so the scan runs.
  if (!captive && settings.btEnabled && !playerBtConnected()) {
    server.send(200, "application/json", "{\"error\":\"The headphones are still connecting; try again shortly (or switch Bluetooth off).\"}");
    return;
  }
  // Every access point is listed, also each point of a mesh network: "same" marks a
  // point of the network the radio is on (the page offers a hop to it), "cur" the
  // point it is associated with now.
  bool sta = !captive && WiFi.status() == WL_CONNECTED && WiFi.BSSID() != nullptr;
  uint8_t curBssid[6] = {};
  String curSsid;
  if (sta) {
    memcpy(curBssid, WiFi.BSSID(), 6);
    curSsid = WiFi.SSID();
  }
  int n = WiFi.scanNetworks();
  String j = "[";
  bool curSeen = false;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i).isEmpty()) continue;
    const uint8_t* b = WiFi.BSSID(i);
    bool same = sta && WiFi.SSID(i) == curSsid;
    bool cur = same && b && memcmp(b, curBssid, 6) == 0;
    curSeen = curSeen || cur;
    if (j.length() > 1) j += ",";
    j += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
         ",\"ch\":" + String(WiFi.channel(i)) + ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"" +
         (same ? ",\"same\":true" : "") + (cur ? ",\"cur\":true" : "") + "}";
  }
  // A scan while associated often misses the own access point: add it from the link.
  if (sta && !curSeen) {
    if (j.length() > 1) j += ",";
    j += "{\"ssid\":\"" + jsonEscape(curSsid) + "\",\"rssi\":" + String(WiFi.RSSI()) +
         ",\"ch\":" + String(WiFi.channel()) + ",\"bssid\":\"" + WiFi.BSSIDstr() +
         "\",\"same\":true,\"cur\":true}";
  }
  j += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", j);
}

// Download a file from the SD card (log, measurements, backup, logo, firmware copy).
static void handleSdFile() {
  String p = server.arg("p");
  if (!sdPathAllowed(p)) { server.send(404, "text/plain", "Not found"); return; }
  File f = SD.open(p, FILE_READ);
  if (!f || f.isDirectory()) { server.send(404, "text/plain", "Not found"); return; }
  String type = p.endsWith(".csv") ? "text/csv" : p.endsWith(".log") || p.endsWith(".txt") ? "text/plain; charset=utf-8"
                                                                                            : "application/octet-stream";
  server.streamFile(f, type);
  f.close();
}

// Station logo upload: 64x64 RGB565 (8192 bytes) made by the page (multipart, like the OTA upload).
static uint8_t* logoUpload = nullptr;
static size_t logoUploadLen = 0;
static const size_t LOGO_UPLOAD_MAX = SD_LOGO_MAX * SD_LOGO_MAX * 2;

static void handleLogoUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    if (!logoUpload) logoUpload = (uint8_t*)malloc(LOGO_UPLOAD_MAX);  // PSRAM, kept
    logoUploadLen = 0;
  } else if (up.status == UPLOAD_FILE_WRITE && logoUpload) {
    size_t room = logoUploadLen < LOGO_UPLOAD_MAX ? LOGO_UPLOAD_MAX - logoUploadLen : 0;
    size_t n = up.currentSize < room ? up.currentSize : room;
    memcpy(logoUpload + logoUploadLen, up.buf, n);
    logoUploadLen += up.currentSize;  // counts everything, so an oversized file is refused
  }
}

static void handleLogoDone() {
  bool ok = logoUpload && sdLogoSave(server.arg("i").toInt(), logoUpload, logoUploadLen);
  server.send(ok ? 200 : 400, "text/plain", ok ? "ok" : "Saving failed (no card or wrong format)");
}

// Hop to another access point of the current network (picked from the scan list).
static void handleRoamTo() {
  unsigned int b[6];
  int ch = server.arg("ch").toInt();
  if (captive || ch < 1 || ch > 14 ||
      sscanf(server.arg("bssid").c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) {
    server.send(400, "text/plain", "Invalid access point");
    return;
  }
  uint8_t bssid[6];
  for (int i = 0; i < 6; i++) bssid[i] = (uint8_t)b[i];
  wifiRequestRoam(bssid, ch);
  server.send(200, "text/plain", "ok");
}

void webBegin(bool setupMode) {
  captive = setupMode;
  if (captive) dns.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", PAGE); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/log", HTTP_GET, handleLog);
  server.on("/api/stations", HTTP_GET, [] { server.send(200, "text/plain", settingsStationsText()); });
  server.on("/api/play", HTTP_POST, handlePlay);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/api/volume", HTTP_POST, handleVolume);
  server.on("/api/stations", HTTP_POST, handleStations);
  server.on("/api/bt", HTTP_POST, handleBt);
  server.on("/api/btenable", HTTP_POST, [] {
    bool on = server.arg("on") == "1";
    settings.btEnabled = on;
    settingsSave();
    playerSetBluetoothEnabled(on);
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/flip", HTTP_POST, [] {
    settings.displayFlip = server.arg("on") == "1";
    settingsSave();
    displaySetFlip(settings.displayFlip);
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/scroll", HTTP_POST, [] {
    settings.scrollTitles = server.arg("on") == "1";
    settingsSave();  // the display task reads settings.scrollTitles on every tick
    server.send(200, "text/plain", "ok");
  });
  // Backlight: normal level, night mode and idle dimming (any subset of the fields).
  server.on("/api/backlight", HTTP_POST, [] {
    auto minutes = [](const String& hhmm, int fallback) {
      int c = hhmm.indexOf(':');
      return c > 0 ? constrain(hhmm.substring(0, c).toInt() * 60 + hhmm.substring(c + 1).toInt(), 0, 24 * 60 - 1) : fallback;
    };
    if (server.hasArg("b")) settings.backlight = constrain(server.arg("b").toInt(), 5, 100);
    if (server.hasArg("night")) settings.nightDim = server.arg("night") == "1";
    if (server.hasArg("from")) settings.nightFrom = minutes(server.arg("from"), settings.nightFrom);
    if (server.hasArg("to")) settings.nightTo = minutes(server.arg("to"), settings.nightTo);
    if (server.hasArg("nl")) settings.nightLevel = constrain(server.arg("nl").toInt(), 0, 100);
    if (server.hasArg("idle")) settings.idleMinutes = constrain(server.arg("idle").toInt(), 0, 240);
    if (server.hasArg("il")) settings.idleLevel = constrain(server.arg("il").toInt(), 0, 100);
    settingsSave();
    displayWake();
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/btretry", HTTP_POST, handleBtRetry);
  server.on("/api/btuse", HTTP_POST, handleBtUse);
  server.on("/api/btpair", HTTP_POST, handleBtPair);
  server.on("/api/btdel", HTTP_POST, handleBtDel);
  server.on("/api/batfitted", HTTP_POST, [] {  // a Li-Po is connected (stored)
    settings.batteryFitted = server.arg("on") == "1";
    settingsSave();
    batteryRefresh();  // the display shows the change at once
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/line32", HTTP_POST, [] {  // line output slot width, stored
    settings.line32 = server.arg("on") == "1";
    settingsSave();
    playerSetLineSlots32(settings.line32);
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/linetest", HTTP_POST, [] {  // test: tone=0/1 fmt=i2s/msb bits=16/32
    playerLineTest(server.arg("tone") == "1", server.arg("fmt") == "msb",
                   server.hasArg("bits") ? server.arg("bits").toInt() : 16);
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/btpause", HTTP_POST, [] {  // test: s = seconds without Bluetooth audio
    bool ok = playerBtPause(server.hasArg("s") ? server.arg("s").toInt() : 10);
    server.send(ok ? 200 : 409, "text/plain", ok ? "ok" : "not possible now");
  });
  server.on("/api/coex", HTTP_POST, [] {
    playerSetCoexWifi(server.arg("wifi") == "1");
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/btlead", HTTP_POST, [] {
    settings.btLead = server.arg("on") == "1";
    settingsSave();
    playerApplyLinkPolicy();  // for the next connection
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/btauto", HTTP_POST, [] {
    playerBtSetAuto(server.arg("on") == "1");
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/btup", HTTP_POST, [] {
    server.send(200, "text/plain", playerBtMoveUp(server.arg("i").toInt()) ? "ok" : "Cannot move higher");
  });
  server.on("/api/btforget", HTTP_POST, handleBtForget);
  server.on("/api/btidentity", HTTP_POST, [] {  // diagnostics: pick the Bluetooth address variant, then restart
    settings.btIdentity = constrain(server.arg("v").toInt(), 0, 200);
    settingsSave();
    server.send(200, "text/plain", "ok");
    restartAt = millis() + 1000;
  });
  server.on("/api/wifi", HTTP_POST, handleWifi);
  server.on("/api/roam", HTTP_POST, handleRoam);
  server.on("/api/autoscan", HTTP_POST, handleAutoScan);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/roamto", HTTP_POST, handleRoamTo);
  server.on("/api/sd", HTTP_GET, [] { server.send(200, "application/json", sdListJson()); });
  server.on("/api/sdfile", HTTP_GET, handleSdFile);
  server.on("/api/logo", HTTP_POST, handleLogoDone, handleLogoUpload);
  server.on("/api/logodel", HTTP_POST, [] {
    server.send(200, "text/plain", sdLogoDelete(server.arg("i").toInt()) ? "ok" : "No logo deleted");
  });
  server.on("/api/sdbackup", HTTP_POST, [] {
    server.send(200, "text/plain", sdBackupNow() ? "ok" : "Backup failed (no card?)");
  });
  server.on("/api/sdrestore", HTTP_POST, [] {
    bool ok = false;
    String msg = sdRestoreSettings(&ok);
    server.send(ok ? 200 : 400, "text/plain", msg);
    if (ok) restartAt = millis() + 1500;
  });
  server.on("/api/fwsave", HTTP_POST, [] {
    sdRequestFirmwareSave();  // copied in steps by sdLoop()
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/fwrestore", HTTP_POST, [] {
    sdRequestFirmwareRestore(server.arg("f"));  // sdLoop() flashes it and restarts
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/sdformat", HTTP_POST, [] {
    sdRequestFormat();  // runs in loop() after this answer
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  server.on("/api/restart", HTTP_POST, [] {
    server.send(200, "text/plain", "ok");
    restartAt = millis() + 1000;
  });
  server.onNotFound([] {
    if (captive) {  // send phones' connectivity checks to the setup page
      server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });
  server.begin();
  xTaskCreatePinnedToCore(otaGuardTask, "otaguard", 2048, nullptr, 1, nullptr, 1);
}

void webLoop() {
  if (captive) dns.processNextRequest();
  server.handleClient();
  if (restartAt && (int32_t)(millis() - restartAt) > 0) {  // wrap-safe
    playerShutdown();
    ESP.restart();
  }
}
