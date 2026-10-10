import collections
import datetime
import json
import math
import pathlib
import threading
import time
from serial.tools import list_ports
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import serial

root = pathlib.Path(__file__).resolve().parent
logs = root / 'logs'
logs.mkdir(exist_ok=True)
previous = sorted(logs.glob('*.log'))
path = previous[-1] if previous else logs / (datetime.datetime.now().strftime('%Y%m%d-%H%M%S') + '.log')
lines = collections.deque(maxlen=300)
if path.exists():
    lines.extend(path.read_text(encoding='utf-8').splitlines()[-300:])
lock = threading.Lock()
state = {'connected': False, 'error': '', 'sample': 'Unlabeled', 'paused': False, 'total': 0.0, 'count': 0}
readings = collections.deque(maxlen=100)
state.update(ntu_total=0.0, ntu_count=0)
def estimate_ntu(voltage):
    return -1120.4 * voltage**2 + 5742.3 * voltage - 4352.9 if 2.5 <= voltage <= 4.2 else None
snapshot_path = root / 'restart_snapshot.json'
if snapshot_path.exists():
    saved = json.loads(snapshot_path.read_text(encoding='utf-8'))
    for key in ('sample', 'paused', 'total', 'count'):
        state[key] = saved.get(key, state[key])
    for row in saved.get('readings', []):
        ntu = estimate_ntu(row[1])
        readings.append(row[:4] + [ntu])
    # Historical NTU total may include more than the visible 100 rows.
    state['ntu_total'] = saved.get('ntu_total', sum(r[4] for r in readings if r[4] is not None))
    state['ntu_count'] = saved.get('ntu_count', sum(r[4] is not None for r in readings))
page = '''<!doctype html><meta charset="utf-8"><title>OASIS NTU live test</title>
<style>body{font:16px system-ui;background:#10202a;color:#edf6f8;margin:30px}input,button{font:inherit;padding:8px}h1{color:#6de0ca}table{border-collapse:collapse;width:100%;max-width:900px}th,td{padding:12px 16px;border-bottom:1px solid #34505e;text-align:right;font-variant-numeric:tabular-nums}th{background:#1c3543;position:sticky;top:0}th:first-child,td:first-child{text-align:left}td:nth-child(2){color:#6de0ca;font-weight:bold}#latest{font-size:36px;color:#6de0ca;margin:20px 0}tbody tr:nth-child(even){background:#142934}</style>
<h1>NTU sensor — live voltage log</h1>
<p>COM3 · 115200 baud · A0 direct · 5V ADC · Assumed ADS1115</p>
<p>Compare displayed voltage with your multimeter before interpreting readings. NTU is an uncalibrated DFRobot curve estimate: -1120.4V² + 5742.3V - 4352.9. Outside 2.5–4.2V, no NTU estimate is shown.</p>
<input id="label" placeholder="Sample: dispenser / tap / ..."><button onclick="mark()">Mark sample</button>
<p><button id="pause" onclick="control('pause')">Pause</button> <button id="resume" onclick="control('resume')">Resume</button></p><p id="average">Running average: waiting for readings</p><p id="status"></p><div id="latest">Waiting for reading...</div>
<p>DFRobot reference estimate only; uncalibrated for this module. Latest readings first.</p><table><thead><tr><th>Water sample</th><th>Current voltage (V)</th><th>DFRobot NTU estimate</th></tr></thead><tbody id="readings"></tbody></table>
<script>async function mark(){await fetch('/mark',{method:'POST',body:document.querySelector('#label').value})}
async function control(action){await fetch('/'+action,{method:'POST'});await update()}
async function update(){try{let d=await(await fetch('/data')).json();document.querySelector('#status').textContent=(!d.connected?'Reconnecting':d.paused?'Paused':'Collecting')+' | Sample: '+d.sample+(d.error?' | '+d.error:'');document.querySelector('#pause').disabled=d.paused;document.querySelector('#resume').disabled=!d.paused;document.querySelector('#average').textContent=d.count?'Running average: '+(d.total/d.count).toFixed(5)+' V ('+d.count+' readings)'+(d.ntu_count?' | Mean NTU estimate: '+(d.ntu_total/d.ntu_count).toFixed(1):' | NTU: outside curve range'):'Running average: reset — no readings';let body=document.querySelector('#readings');body.replaceChildren();for(let row of [...d.readings].reverse()){let tr=document.createElement('tr');[row[0],row[1],row[4]].forEach((value,i)=>{let td=document.createElement('td');td.textContent=i===0?value:i>=2?(value===null?'Outside curve range':Number(value).toFixed(1)):Number(value).toFixed(5);tr.appendChild(td)});body.appendChild(tr)}document.querySelector('#latest').textContent=d.readings.length?Number(d.readings[d.readings.length-1][1]).toFixed(5)+' V · '+(d.readings[d.readings.length-1][4]===null?'NTU outside curve range':Number(d.readings[d.readings.length-1][4]).toFixed(1)+' NTU (DFRobot estimate)'):d.paused?'Paused':'Waiting for reading...'}catch(e){document.querySelector('#status').textContent='Logger unavailable: '+e}}setInterval(update,1000);update()</script>'''

def record(text):
    entry = datetime.datetime.now().astimezone().isoformat(timespec='seconds') + ' ' + text
    with lock:
        fields = text.split(',')
        if len(fields) == 6:
            try:
                values = [float(v) for v in fields[:5]]
            except ValueError:
                pass
            else:
                if state['paused']:
                    return
                ntu = estimate_ntu(values[2])
                readings.append([state['sample'], *values[2:5], ntu])
                if ntu is not None:
                    state['ntu_total'] += ntu
                    state['ntu_count'] += 1
                entry += ',NTU_estimate=' + (f'{ntu:.3f}' if ntu is not None else 'OUTSIDE_CURVE_RANGE')
                state['total'] += values[2]
                state['count'] += 1
        lines.append(entry)
        with path.open('a', encoding='utf-8') as f:
            f.write(entry + '\n')

def reader():
    last_error = None
    while True:
        port = None
        try:
            candidates = [p.device for p in list_ports.comports()
                          if p.vid == 0x10C4 and p.pid == 0xEA60 and p.serial_number == '0001']
            device = candidates[0] if len(candidates) == 1 else 'COM3'
            port = serial.Serial()
            port.port = device
            port.baudrate = 115200
            port.timeout = 1
            port.dtr = False
            port.rts = False
            port.open()
            with lock:
                state.update(connected=True, error='', port=device)
            last_error = None
            record('SERIAL CONNECTED: ' + device)
            while True:
                line = port.readline().decode('utf-8', errors='replace').strip()
                if line:
                    record(line)
        except (serial.SerialException, OSError) as e:
            error = str(e)
            with lock:
                state.update(connected=False, error='Reconnecting automatically: ' + error)
            if error != last_error:
                record('SERIAL DISCONNECTED: ' + error)
                last_error = error
        finally:
            if port is not None:
                try:
                    port.close()
                except (serial.SerialException, OSError):
                    pass
        time.sleep(2)

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass
    def do_GET(self):
        if self.path == '/data':
            with lock:
                data = dict(state, lines=list(lines), readings=list(readings), path=str(path))
            body = json.dumps(data).encode()
            kind = 'application/json'
        else:
            body = page.encode()
            kind = 'text/html; charset=utf-8'
        self.send_response(200)
        self.send_header('Content-Type', kind)
        self.send_header('Cache-Control', 'no-store')
        self.end_headers()
        self.wfile.write(body)
    def do_POST(self):
        n = min(int(self.headers.get('Content-Length', 0)), 200)
        label = self.rfile.read(n).decode('utf-8', errors='replace').replace('\n', ' ').replace('\r', ' ')
        if self.path in ('/pause', '/resume'):
            with lock:
                state['paused'] = self.path == '/pause'
                if not state['paused']:
                    state['ntu_total'] = 0.0
                    state['ntu_count'] = 0
                    state['total'] = 0.0
                    state['count'] = 0
                    readings.clear()
            record('PAUSED / READINGS PRESERVED' if state['paused'] else 'RESUMED / READINGS AND AVERAGES RESET')
        elif self.path == '/mark':
            with lock:
                state['sample'] = label
            record('SAMPLE: ' + label)
        else:
            self.send_error(404)
            return
        self.send_response(204)
        self.end_headers()

server = ThreadingHTTPServer(('127.0.0.1', 8123), Handler)
threading.Thread(target=reader, daemon=True).start()
print('Live log: http://127.0.0.1:8123 ; file: ' + str(path), flush=True)
server.serve_forever()
