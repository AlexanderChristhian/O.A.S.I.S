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
def friend_ntu(voltage):
    if voltage <= 0:
        return 3000.0
    return max(0.0, min(3000.0, 1.0 + 5783.0 * math.log(3.22 / voltage)))
page = """<!doctype html><meta charset="utf-8"><title>PH-4502C voltage test</title>
<style>body{font:18px system-ui;background:#10202a;color:#edf6f8;margin:32px}#voltage{font-size:72px;color:#6de0ca}table{border-collapse:collapse;width:550px}th,td{padding:10px;border-bottom:1px solid #34505e;text-align:right;font-variant-numeric:tabular-nums}button{padding:10px;font:inherit}</style>
<h1>PH-4502C — live sensor voltage</h1>
<p>Po → 20kΩ → GPIO34 junction → 10kΩ → GND · Internal ESP32 ADC</p>
<p>Connect the electrode to the module BNC socket. Remove any temporary BNC short with USB unplugged.<br>Verify Po-to-GND voltage with a multimeter. Keep the module and connectors dry.</p>
<div id="voltage">Waiting...</div><p id="average"></p><p id="status"></p>
<p><input id="label" placeholder="Sample name"><button onclick="mark()">Mark sample</button></p><button id="pause" onclick="control('pause')">Pause</button> <button id="resume" onclick="control('resume')">Resume / clear</button>
<p>Po is estimated as GPIO34 voltage × 3 for the 20kΩ/10kΩ divider. Verify with a multimeter. Certified buffers are needed before converting voltage to pH.</p>
<table><thead><tr><th>Estimated Po voltage (V)</th><th>GPIO34 voltage (V)</th><th>Water sample</th></tr></thead><tbody id="rows"></tbody></table>
<script>async function mark(){await fetch('/mark',{method:'POST',body:document.querySelector('#label').value});update()}
async function control(action){await fetch('/'+action,{method:'POST'});update()}
async function update(){try{let d=await(await fetch('/data')).json();document.querySelector('#status').textContent=(!d.connected?'Reconnecting':d.paused?'Paused':'Live')+(d.error?' | '+d.error:'')+(d.measurement_status==='DIRECT_ADC_LIMIT'?' | ADC limit: check wiring':'');document.querySelector('#pause').disabled=d.paused;document.querySelector('#resume').disabled=!d.paused;let rows=document.querySelector('#rows');rows.replaceChildren();for(let row of [...d.readings].reverse()){let tr=document.createElement('tr');for(let v of [row[1].toFixed(4),row[2].toFixed(4),row[0]]){let td=document.createElement('td');td.textContent=v;tr.appendChild(td)}rows.appendChild(tr)}if(d.readings.length){let v=d.readings[d.readings.length-1][1];document.querySelector('#voltage').textContent=v.toFixed(4)+' V (Po estimate)';document.querySelector('#average').textContent='Running average: '+(d.total/d.count).toFixed(4)+' V ('+d.count+' readings)'}}catch(e){document.querySelector('#status').textContent='Logger unavailable'}}setInterval(update,300);update()</script>"""

def record(text):
    entry = datetime.datetime.now().astimezone().isoformat(timespec='seconds') + ' ' + text
    with lock:
        fields = text.split(',')
        if len(fields) == 6 and fields[5].startswith('DIRECT_'):
            try:
                values = [float(v) for v in fields[:5]]
            except ValueError:
                pass
            else:
                if state['paused']:
                    return
                ntu = None
                friend = None
                readings.append([state['sample'], *values[2:5], ntu, friend])
                if ntu is not None:
                    state['ntu_total'] += ntu
                    state['ntu_count'] += 1
                entry += ',Sample=' + state['sample']
                state['total'] += values[2]
                state['count'] += 1
                state['measurement_status'] = fields[5]
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
