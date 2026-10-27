#pragma once
const char DASHBOARD[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Vehicle Health Computer</title><style>
:root{color-scheme:dark}body{font:16px system-ui;background:#0b1422;color:#e8edf5;max-width:1040px;margin:auto;padding:24px}h1{font-size:32px;margin-bottom:8px}.muted{color:#a8bdd2}#mode{color:#58dfc0;font-weight:bold}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:14px}article,section{background:#142338;border:1px solid #29405b;border-radius:14px;padding:20px;margin:16px 0}article{margin:0}.value{font-size:32px;margin:12px 0}.tag{color:#f4cf83}button{padding:12px;border:0;border-radius:8px;background:#58dfc0;color:#08251f;font-weight:bold}small{color:#a8bdd2}#error{color:#ffa7a7}a{color:#58dfc0}</style>
<header><p id="mode">CONNECTING</p><h1>Vehicle Health Computer</h1><p class="muted">Local OBD-II telemetry • ESP32-S3</p></header>
<p>Parked testing only. Read-only diagnostics still send CAN requests. No fault clearing or vehicle controls.</p>
<p id="error" role="status"></p><div id="cards" class="grid"></div>
<section><h2>Diagnostics</h2><p id="mil">Waiting for monitor status</p><button id="scan">Read stored DTCs</button><p id="dtcs">Not scanned this session</p><small>A DTC identifies a reported condition, not a failed part. No codes does not prove a healthy vehicle.</small></section>
<section><h2>Connection</h2><p id="status"></p><p class="muted">ECU voltage is reported by the ECU. It is not a direct battery measurement. Unsupported or stale values are never shown as live.</p></section>
<script>
const $=id=>document.getElementById(id);let token='';
$('scan').onclick=async()=>{try{let r=await fetch('/api/dtc',{method:'POST',headers:{'X-Session-Token':token}});$('dtcs').textContent=await r.text()}catch(e){$('error').textContent='Dashboard disconnected'}};
async function refresh(){try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();token=s.token;$('error').textContent='';$('mode').textContent=s.mode;$('status').textContent=s.status+' | ECU '+s.ecu+' | TX '+s.tx+' | RX '+s.rx+' | timeouts '+s.timeouts;
$('cards').replaceChildren(...s.metrics.map(m=>{let a=document.createElement('article');let t=document.createElement('div');t.textContent=m.name;let v=document.createElement('div');v.className='value';v.textContent=m.valid?m.value.toFixed(1)+' '+m.unit:'—';let f=document.createElement('small');f.textContent=!m.supported?'Unsupported / not discovered':m.valid?'Updated '+(m.age/1000).toFixed(1)+' s ago':'Waiting / stale';a.append(t,v,f);return a}));
$('mil').textContent=s.monitorValid?'MIL '+(s.mil?'ON':'OFF')+' • Reported stored DTC count: '+s.dtcCount:'Monitor status unavailable / stale';$('dtcs').textContent=s.dtcStatus+(s.dtcs.length?' • '+s.dtcs.join(', '):'');$('scan').disabled=!s.canScan;
}catch(e){$('error').textContent='Connection lost — displayed values are no longer live';$('cards').replaceChildren();$('mil').textContent='Monitor status unavailable';$('scan').disabled=true}finally{setTimeout(refresh,1000)}}refresh();
</script></html>)HTML";
