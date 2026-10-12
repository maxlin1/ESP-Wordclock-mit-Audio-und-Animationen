#pragma once

#include <Arduino.h>

// Web page for the extras: http://<clock-ip>:81/extras
static const char EXTRAS_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Wortuhr – Sound &amp; Animationen</title>
<style>
:root{--bg:#f4f4f2;--card:#fff;--text:#1d1d1f;--muted:#6b6b70;--line:#e2e2e0;--accent:#2f6fde;--ok:#1e8e3e;--warn:#c5221f}
@media (prefers-color-scheme:dark){:root{--bg:#161618;--card:#222226;--text:#ededf0;--muted:#a0a0a8;--line:#34343a;--accent:#6ea0ff;--ok:#5bc27a;--warn:#ff7b72}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:15px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}
header{padding:20px 16px 8px;max-width:760px;margin:0 auto}
header h1{margin:0;font-size:22px}
header p{margin:4px 0 0;color:var(--muted)}
header a{color:var(--accent)}
main{max-width:760px;margin:0 auto;padding:8px 16px 40px}
section{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin:12px 0}
h2{font-size:17px;margin:0 0 12px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:8px 0;border-top:1px solid var(--line)}
.row:first-of-type{border-top:0}
.row label{flex:1}
.row small{display:block;color:var(--muted)}
input,select,button{font:inherit;color:inherit}
input[type=number],input[type=text],select{background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:6px 8px;min-width:0}
input[type=number]{width:84px}
input[type=range]{width:180px}
button{background:var(--accent);color:#fff;border:0;border-radius:8px;padding:8px 14px;cursor:pointer}
button.secondary{background:transparent;color:var(--accent);border:1px solid var(--line)}
button.danger{background:transparent;color:var(--warn);border:1px solid var(--line)}
.buttons{display:flex;flex-wrap:wrap;gap:8px;margin-top:12px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:8px}
.grid button{background:var(--bg);color:var(--text);border:1px solid var(--line);text-align:left}
.status{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:8px}
.status div{background:var(--bg);border-radius:8px;padding:10px}
.status b{display:block;font-size:18px}
.status span{color:var(--muted);font-size:13px}
.weekdays{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:8px}
.weekdays label{display:flex;flex-direction:column;gap:4px;color:var(--muted);font-size:13px}
table{width:100%;border-collapse:collapse;font-size:14px}
th{text-align:left;color:var(--muted);font-weight:normal;padding:4px}
td{padding:4px;vertical-align:top}
td input,td select{width:100%}
.events-wrap{overflow-x:auto}
.events-wrap table{min-width:680px}
#toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%);background:var(--text);color:var(--bg);padding:10px 16px;border-radius:8px;opacity:0;transition:opacity .2s;pointer-events:none}
#toast.show{opacity:1}
</style>
</head>
<body>
<header>
<h1>Sound &amp; Animationen</h1>
<p>Erweiterungen der Wortuhr · <a id="home" href="/">zur Uhr-Hauptseite</a></p>
</header>
<main>

<section>
<h2>Status</h2>
<div class="status">
<div><b id="st-df">–</b><span>MP3-Player</span></div>
<div><b id="st-temp">–</b><span>Temperatur</span></div>
<div><b id="st-hum">–</b><span>Luftfeuchte</span></div>
<div><b id="st-press">–</b><span>Luftdruck</span></div>
</div>
</section>

<section>
<h2>Sound</h2>
<div class="row"><label for="sound">Sound eingeschaltet</label><input type="checkbox" id="sound"></div>
<div class="row"><label for="volume">Lautstärke <small id="volume-value"></small></label><input type="range" id="volume" min="0" max="100" step="5"></div>
<div class="row"><label for="volumeByTime">Lautstärke nach Tageszeit<small>nachts leiser, tagsüber lauter</small></label><input type="checkbox" id="volumeByTime"></div>
<div class="row"><label for="weekendLater">Am Wochenende 2 Stunden später laut</label><input type="checkbox" id="weekendLater"></div>
<div class="row"><label for="quiet">Nachtruhe (keine Töne)</label><input type="checkbox" id="quiet"></div>
<div class="row"><label>Nachtruhe von / bis (Uhr)</label><span><input type="number" id="quietFrom" min="0" max="23"> – <input type="number" id="quietTo" min="0" max="23"></span></div>
<div class="row"><label for="voice">Sprecher für Ansagen</label><select id="voice"><option value="0">Vicki</option><option value="1">Hans</option></select></div>
<div class="row"><label for="startupSound">Ton beim Einschalten</label><input type="checkbox" id="startupSound"></div>
</section>

<section>
<h2>Stundengong</h2>
<div class="row"><label for="chime">Stundengong eingeschaltet</label><input type="checkbox" id="chime"></div>
<div class="row"><label for="singleGong">Nur ein Schlag<small>statt so vieler Schläge wie die Stunde</small></label><input type="checkbox" id="singleGong"></div>
<div class="row"><label for="voice24h">Sprecher sagen die Stunde im 24-Stunden-Format</label><input type="checkbox" id="voice24h"></div>
<div class="weekdays" id="weekdays"></div>
</section>

<section>
<h2>Animationen &amp; Ereignisse</h2>
<div class="row"><label for="timeAnimations">Uhrzeit-Animationen<small>z.&nbsp;B. „Z0800“ um 8:00 Uhr</small></label><input type="checkbox" id="timeAnimations"></div>
<div class="row"><label for="events">Ereignisse anzeigen<small>Geburtstage, Weihnachten, Silvester …</small></label><input type="checkbox" id="events"></div>
<p style="color:var(--muted);margin:12px 0 8px">Animation antippen zum Abspielen:</p>
<div class="grid" id="animations"></div>
</section>

<section>
<h2>Ausprobieren</h2>
<div class="buttons">
<button onclick="call('/api/extras/announce?what=time')">Uhrzeit ansagen</button>
<button onclick="call('/api/extras/announce?what=date')">Datum ansagen</button>
<button onclick="call('/api/extras/announce?what=climate')">Raumklima ansagen</button>
<button class="secondary" onclick="call('/api/extras/stop')">Stopp</button>
</div>
<div class="row" style="margin-top:12px"><label for="track">Sound-Nummer abspielen<small>z.&nbsp;B. 700 = OK, 720 = Happy Birthday</small></label><span><input type="number" id="track" min="1" max="2999" value="700"> <button onclick="call('/api/extras/play?track='+$('track').value)">▶</button></span></div>
</section>

<section>
<h2>Ereignisse</h2>
<p style="color:var(--muted);margin-top:0">„YY“ im Text wird durch das Alter ersetzt, wenn ein Jahr eingetragen ist. Intervall: alle wie viele Minuten das Ereignis an diesem Tag gezeigt wird.</p>
<div class="events-wrap"><table>
<thead><tr><th style="width:62px">Tag</th><th style="width:62px">Monat</th><th style="width:76px">Jahr</th><th>Text</th><th style="width:76px">Sound</th><th style="width:120px">Animation vorher</th><th style="width:120px">nachher</th><th style="width:70px">Intervall</th><th></th></tr></thead>
<tbody id="event-rows"></tbody>
</table></div>
<div class="buttons">
<button class="secondary" onclick="addEvent()">+ Ereignis</button>
<button onclick="saveEvents()">Ereignisse speichern</button>
<label class="secondary" style="padding:8px 14px;border:1px solid var(--line);border-radius:8px;cursor:pointer;color:var(--accent)">events.json importieren<input type="file" id="import" accept=".json" style="display:none"></label>
</div>
</section>

<section>
<h2>Hardware</h2>
<div class="row"><label for="rxPin">DFPlayer TX → ESP (GPIO)</label><input type="number" id="rxPin" min="0" max="255"></div>
<div class="row"><label for="txPin">ESP → DFPlayer RX (GPIO)</label><input type="number" id="txPin" min="0" max="255"></div>
<div class="row"><label for="busyPin">DFPlayer BUSY (GPIO)<small>255 = nicht angeschlossen</small></label><input type="number" id="busyPin" min="0" max="255"></div>
<div class="row"><label for="altitude">Höhe über dem Meer (m)<small>für den Luftdruck auf Meereshöhe</small></label><input type="number" id="altitude" min="-500" max="9000"></div>
<div class="row"><label for="temperatureOffset">Temperatur-Korrektur (°C)</label><input type="number" id="temperatureOffset" min="-20" max="20" step="0.1"></div>
<div class="buttons"><button class="secondary" onclick="call('/api/extras/dfreset')">MP3-Player neu starten</button></div>
</section>

</main>
<div id="toast"></div>
<script>
const $=id=>document.getElementById(id);
const days=["Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag"];
const checks=["sound","volumeByTime","weekendLater","quiet","startupSound","chime","singleGong","voice24h","timeAnimations","events"];
const numbers=["quietFrom","quietTo","rxPin","txPin","busyPin","altitude","temperatureOffset"];
let data=null,saveTimer=null;

function toast(t){const e=$("toast");e.textContent=t;e.classList.add("show");clearTimeout(e._t);e._t=setTimeout(()=>e.classList.remove("show"),1800);}
async function call(url){try{const r=await fetch(url);const j=await r.json();toast(j.ok?"OK":(j.error||"Fehler"));}catch(e){toast("Keine Verbindung");}}

function animOptions(selected){
  let h='<option value="">–</option>';
  for(const a of data.animations){h+=`<option${a===selected?" selected":""}>${a}</option>`;}
  return h;
}
function esc(s){return String(s??"").replace(/&/g,"&amp;").replace(/"/g,"&quot;").replace(/</g,"&lt;");}

function eventRow(e){
  const tr=document.createElement("tr");
  tr.innerHTML=`<td><input type="number" min="1" max="31" data-k="day" value="${e.day}"></td>
<td><input type="number" min="1" max="12" data-k="month" value="${e.month}"></td>
<td><input type="number" min="0" max="2999" data-k="year" value="${e.year||""}" placeholder="jedes"></td>
<td><input type="text" maxlength="80" data-k="text" value="${esc(e.text)}"></td>
<td><input type="number" min="0" max="2999" data-k="sound" value="${e.sound||""}"></td>
<td><select data-k="pre">${animOptions(e.pre)}</select></td>
<td><select data-k="post">${animOptions(e.post)}</select></td>
<td><input type="number" min="1" max="255" data-k="interval" value="${e.interval||60}"></td>
<td><button class="danger" title="löschen">✕</button></td>`;
  tr.querySelector("button").onclick=()=>tr.remove();
  $("event-rows").appendChild(tr);
}
function addEvent(){eventRow({day:1,month:1,year:0,text:"",sound:0,pre:"",post:"",interval:60});}
function collectEvents(){
  return [...$("event-rows").children].map(tr=>{
    const e={};tr.querySelectorAll("[data-k]").forEach(i=>{const k=i.dataset.k;e[k]=(i.type==="number")?Number(i.value||0):i.value;});return e;});
}
async function postJson(url,body){
  try{const r=await fetch(url,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});const j=await r.json();toast(j.ok?"Gespeichert":(j.error||"Fehler"));return j.ok;}catch(e){toast("Keine Verbindung");return false;}
}
function saveEvents(){postJson("/api/extras/events",collectEvents());}

function collectSettings(){
  const s={};
  for(const k of checks)s[k]=$(k).checked;
  for(const k of numbers)s[k]=Number($(k).value);
  s.volume=Number($("volume").value);
  s.voice=Number($("voice").value);
  s.chimeSet=days.map((_,i)=>Number($("day"+i).value));
  return s;
}
function scheduleSave(){clearTimeout(saveTimer);saveTimer=setTimeout(()=>postJson("/api/extras/settings",collectSettings()),400);}

function render(){
  const s=data.settings;
  for(const k of checks)$(k).checked=!!s[k];
  for(const k of numbers)$(k).value=s[k];
  $("volume").value=s.volume;$("volume-value").textContent=s.volume+" %";
  $("voice").value=s.voice;
  const wd=$("weekdays");wd.innerHTML="";
  days.forEach((d,i)=>{
    const l=document.createElement("label");l.textContent=d;
    const sel=document.createElement("select");sel.id="day"+i;
    data.soundSets.forEach((n,j)=>{const o=document.createElement("option");o.value=j;o.textContent=n;sel.appendChild(o);});
    sel.value=s.chimeSet[i];sel.onchange=scheduleSave;l.appendChild(sel);wd.appendChild(l);
  });
  const g=$("animations");g.innerHTML="";
  for(const a of data.animations){const b=document.createElement("button");b.textContent=a;b.onclick=()=>call("/api/extras/animation?name="+encodeURIComponent(a));g.appendChild(b);}
  $("event-rows").innerHTML="";data.events.forEach(eventRow);
  renderStatus();
}
function renderStatus(){
  const st=data.status,c=st.climate||{};
  $("st-df").textContent=st.dfplayer?(st.playing?"spielt":"bereit"):"aus";
  $("st-temp").textContent=c.temperature!=null?c.temperature.toFixed(1)+" °C":"–";
  $("st-hum").textContent=c.humidity!=null?Math.round(c.humidity)+" %":"–";
  $("st-press").textContent=c.pressure!=null?Math.round(c.pressure)+" hPa":"–";
}
async function load(){
  try{data=await (await fetch("/api/extras")).json();render();}catch(e){toast("Keine Verbindung zur Uhr");}
}
async function refreshStatus(){
  try{const d=await (await fetch("/api/extras")).json();data.status=d.status;renderStatus();}catch(e){}
}

for(const k of checks)$(k).addEventListener("change",scheduleSave);
for(const k of numbers)$(k).addEventListener("change",scheduleSave);
$("voice").addEventListener("change",scheduleSave);
$("volume").addEventListener("input",()=>{$("volume-value").textContent=$("volume").value+" %";});
$("volume").addEventListener("change",scheduleSave);
$("import").addEventListener("change",async ev=>{
  const f=ev.target.files[0];if(!f)return;
  try{const j=JSON.parse(await f.text());if(await postJson("/api/extras/events",j))load();}catch(e){toast("Datei ist kein gültiges JSON");}
  ev.target.value="";
});
$("home").href="http://"+location.hostname+"/";
load();setInterval(refreshStatus,15000);
</script>
</body>
</html>
)rawliteral";
