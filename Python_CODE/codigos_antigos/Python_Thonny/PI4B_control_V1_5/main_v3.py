"""
RB_PI4B_Main_WD.py — Ficheiro principal do PI_4B Control Panel para Windows.

Arquitetura: Flask (servidor web) + Flask-SocketIO (push em tempo real) +
threads de comunicação série. A interface local abre automaticamente no
browser padrão do Windows em modo kiosk, e a interface mobile é servida
pelo mesmo servidor para acesso via telemóvel/tablet na mesma rede.

Para correr:
    pip install flask flask-socketio pyserial
    python RB_PI4B_Main_WD.py

Instalar biblioteca de qr code (opcional, para o QR code na aba de definições):
    pip install qrcode[pil]
"""

# ===========================================================================
# IMPORTS
# ===========================================================================
import json
import os
import queue
import socket
import subprocess
import threading
import time
from pathlib import Path

from flask import Flask, render_template_string, request, jsonify, Response
from flask_socketio import SocketIO, emit
import serial


# ===========================================================================
# CONFIGURAÇÃO
# ===========================================================================
APP_HOST     = os.environ.get('PI4B_HOST', '0.0.0.0')
APP_PORT     = int(os.environ.get('PI4B_PORT', '5000'))
SERIAL_BAUD  = 115200
PASSWORD_DEV = '1234'

# Frases de handshake do ESP32
HANDSHAKE_LINES   = ('setup chamado: 1', 'Teste LED RGB',
                     'Teste MG90S Servo', 'Servos attached successfully')
HANDSHAKE_CONFIRM = 'Escolha um comando:'
READ_ERROR_THRESHOLD = 5

# Comandos série por modo de movimento
MODE_COMMANDS = {
    'Estatico':   'Ms',
    'Respiracao': 'Mr',
    'Coracao':    'Mb',
    'Humano':     'Mh',
    'Completo':   'Ma',
}


# ===========================================================================
# ESTADO PARTILHADO — única fonte de verdade para Flask + SocketIO
# ===========================================================================
_state_lock = threading.RLock()

state = {
    'devmode':       0,
    'serialhistory': [
        'Zona reservada para mensagens enviadas e recebidas pela serial.',
        'A comunicação será ligada mais tarde.',
    ],
    'start_state':   0,
    'stop_state':    1,
    'mode_state':    'Estatico',
}

serial_status = {
    'stage': 'disconnected',  # disconnected | connecting | connected | error
    'stop_reader': False,
}


def get_state():
    return state


def set_state(**kwargs):
    with _state_lock:
        state.update(kwargs)
    push_state_update()


def append_history(line):
    with _state_lock:
        state['serialhistory'].append(line)
    push_state_update()


def reset_state():
    with _state_lock:
        state['devmode']       = 0
        state['serialhistory'] = [
            'Zona reservada para mensagens enviadas e recebidas pela serial.',
            'A comunicação será ligada mais tarde.',
        ]
        state['start_state']   = 0
        state['stop_state']    = 1
        state['mode_state']    = 'Estatico'


# ===========================================================================
# FLASK + SOCKETIO
# ===========================================================================
app = Flask(__name__)
app.config['SECRET_KEY'] = 'pi4b-secret-key'
socketio = SocketIO(app, async_mode='threading', cors_allowed_origins='*')


def push_state_update():
    """Empurra o estado atual para TODOS os browsers ligados via SocketIO.
    Chamada sempre que o estado muda — substitui o ui.timer(0.5) que tínhamos."""
    socketio.emit('state_update', {
        'devmode':       state['devmode'],
        'start_state':   state['start_state'],
        'stop_state':    state['stop_state'],
        'mode_state':    state['mode_state'],
        'serial_stage':  serial_status['stage'],
        'history_tail':  state['serialhistory'][-200:],  # últimas 200 linhas
    })


def push_serial_stage():
    """Empurra só a mudança de estágio série (mais leve que push_state_update)."""
    socketio.emit('serial_stage', {'stage': serial_status['stage']})


# ===========================================================================
# COMUNICAÇÃO SÉRIE
# ===========================================================================
serial_rx_queue = queue.Queue()
serial_tx_queue = queue.Queue()

_ser           = None
_serial_ready  = False


def _detect_serial_port():
    """No Windows, tenta encontrar automaticamente a porta COM do ESP32
    usando pyserial. Recua para COM7 se não encontrar nada."""
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        # procura primeiro por descrições típicas de ESP32 / CH340 / CP210x
        for p in ports:
            desc = (p.description or '').lower()
            if any(k in desc for k in ('cp210', 'ch340', 'ch341', 'esp', 'usb serial')):
                return p.device
        # se não encontrar por descrição, devolve a primeira porta disponível
        if ports:
            return ports[0].device
    except Exception:
        pass
    return 'COM7'  # porta por defeito


SERIAL_PORT = _detect_serial_port()


def serial_connect():
    if serial_status['stage'] in ('connecting', 'connected'):
        return
    serial_status['stage']       = 'connecting'
    serial_status['stop_reader'] = False
    threading.Thread(target=serial_reader, daemon=True, name='serial-reader').start()
    threading.Thread(target=serial_writer, daemon=True, name='serial-writer').start()
    push_serial_stage()


def serial_disconnect():
    global _ser
    serial_status['stop_reader'] = True
    serial_status['stage']       = 'disconnected'
    if _ser and _ser.is_open:
        try:
            _ser.close()
        except Exception:
            pass
    append_history('[SÉRIE] Desligado pelo utilizador.')
    push_serial_stage()


def serial_reader():
    global _ser, _serial_ready
    consecutive_errors = 0
    try:
        _ser           = serial.Serial()
        _ser.port      = SERIAL_PORT
        _ser.baudrate  = SERIAL_BAUD
        _ser.timeout   = 0.1
        _ser.dtr       = False
        _ser.rts       = True
        _ser.open()
        time.sleep(0.1)
        _ser.rts = False
        time.sleep(0.1)
        _serial_ready = True
        append_history('[SÉRIE] Ligado. ESP32 deve ter feito reset agora.')

        while not serial_status['stop_reader']:
            try:
                data = _ser.readline().decode('ascii', errors='ignore').strip()
                if data:
                    consecutive_errors = 0
                    append_history(f'ESP32: {data}')

                    if HANDSHAKE_CONFIRM in data:
                        serial_status['stage'] = 'connected'
                        push_serial_stage()
                    elif any(h in data for h in HANDSHAKE_LINES):
                        if serial_status['stage'] != 'connected':
                            serial_status['stage'] = 'connecting'
                            push_serial_stage()

            except Exception as e:
                if serial_status['stop_reader']:
                    break
                consecutive_errors += 1
                append_history(f'[ERRO leitura] {e}')
                if consecutive_errors >= READ_ERROR_THRESHOLD:
                    serial_status['stage'] = 'error'
                    push_serial_stage()
                time.sleep(1)

    except Exception as e:
        serial_status['stage'] = 'error'
        append_history(f'[SÉRIE] Falha ao ligar: {e}')
        push_serial_stage()
    finally:
        serial_status['stage'] = 'error' if not serial_status['stop_reader'] else 'disconnected'
        push_serial_stage()


def serial_writer():
    while True:
        cmd = serial_tx_queue.get()
        if _ser and _ser.is_open:
            try:
                _ser.write(cmd.encode('utf-8'))
            except Exception as e:
                append_history(f'[ERRO envio] {e}')
        serial_tx_queue.task_done()


def send_serial(command):
    """Envia um comando ao ESP32 e regista-o no histórico."""
    serial_tx_queue.put(command)
    append_history(f'PI4B: {command}')


async def do_reconnect_async():
    """Duplo reset — resolve caracteres corrompidos na primeira ligação."""
    import asyncio
    serial_disconnect()
    await asyncio.sleep(0.3)
    serial_connect()
    await asyncio.sleep(0.5)
    serial_disconnect()
    await asyncio.sleep(0.3)
    serial_connect()


def do_reconnect_sync():
    serial_disconnect()
    time.sleep(0.3)
    serial_connect()
    time.sleep(0.5)
    serial_disconnect()
    time.sleep(0.3)
    serial_connect()


# ===========================================================================
# AÇÕES DE ALTO NÍVEL (chamadas pelo Flask quando um botão é clicado)
# ===========================================================================
def action_start():
    set_state(start_state=1, stop_state=0)
    send_serial('s')


def action_stop():
    set_state(stop_state=1, start_state=0)
    send_serial('p')


def action_select_mode(mode_name):
    previous = state['mode_state']
    set_state(mode_state=mode_name)
    if mode_name != previous:
        cmd = MODE_COMMANDS.get(mode_name)
        if cmd:
            send_serial(cmd)


def action_toggle_devmode(password=None):
    if state['devmode'] == 0:
        if password == PASSWORD_DEV:
            set_state(devmode=1)
            return True
        return False
    else:
        set_state(devmode=0)
        return True


# ===========================================================================
# UTILITÁRIOS (rede, QR code)
# ===========================================================================
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return '127.0.0.1'


def get_app_url():
    return f'http://{get_local_ip()}:{APP_PORT}'


def generate_qr_base64(data):
    try:
        import qrcode, io, base64
        qr = qrcode.QRCode(border=1, box_size=5)
        qr.add_data(data)
        qr.make(fit=True)
        img = qr.make_image(fill_color='black', back_color='white')
        buf = io.BytesIO()
        img.save(buf, format='PNG')
        return 'data:image/png;base64,' + base64.b64encode(buf.getvalue()).decode()
    except Exception:
        return None


def list_com_ports():
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        return [f'{p.device} — {p.description}' for p in ports] or ['Nenhuma porta encontrada.']
    except Exception as e:
        return [f'Erro: {e}']


def _run_powershell(command):
    """Corre um comando PowerShell e devolve o stdout."""
    result = subprocess.run(
        ['powershell', '-NoProfile', '-Command', command],
        capture_output=True, text=True, timeout=8
    )
    return result.stdout.strip()


def _run_netsh(args):
    """Corre um comando netsh com fallback de encoding para Windows PT."""
    raw = subprocess.run(['netsh'] + args, capture_output=True, timeout=5).stdout
    for enc in ('cp850', 'cp1252', 'utf-8'):
        try:
            return raw.decode(enc)
        except (UnicodeDecodeError, AttributeError):
            continue
    return raw.decode('utf-8', errors='replace')


def get_network_status():
    """Deteta estado da rede via PowerShell (não precisa de permissão de localização)."""
    try:
        out = _run_powershell(
            "Get-NetAdapter | Where-Object {$_.Status -eq 'Up'} | "
            "Select-Object Name, InterfaceDescription | Format-List"
        )
        wifi_up = lan_up = False
        for block in out.split('\n\n'):
            lower = block.lower()
            if any(k in lower for k in ('wi-fi', 'wireless', 'wlan')):
                wifi_up = True
            if any(k in lower for k in ('ethernet', 'lan')):
                lan_up = True

        if wifi_up:
            ssid = _run_powershell(
                "(Get-NetConnectionProfile | "
                "Where-Object {$_.InterfaceAlias -match 'Wi-Fi|Wireless|WLAN'}).Name"
            )
            return f'Wi-Fi ligado: {ssid}' if ssid else 'Wi-Fi ligado (SSID não identificado).'
        if lan_up:
            return 'Ligado por LAN (cabo de rede).'
        return 'Sem ligação de rede ativa.'
    except FileNotFoundError:
        return 'PowerShell não encontrado.'
    except Exception as e:
        return f'Erro ao verificar rede: {e}'


def get_available_networks():
    """Lista redes Wi-Fi disponíveis via netsh (requer serviços de localização no Windows 11)."""
    try:
        out = _run_netsh(['wlan', 'show', 'networks'])
        if 'location permission' in out.lower() or 'location services' in out.lower():
            return [
                'Windows está a bloquear a listagem de redes Wi-Fi.',
                'Ativa em: Definições > Privacidade > Localização > Apps de desktop.',
            ]
        if not out.strip():
            return ['Sem adaptador Wi-Fi disponível.']
        names = []
        for line in out.splitlines():
            if ':' not in line:
                continue
            key, _, value = line.partition(':')
            key = key.strip().lower()
            value = value.strip()
            if key.startswith('ssid') and key[4:].strip().isdigit() and value:
                names.append(value)
        return names or ['Nenhuma rede Wi-Fi encontrada nas proximidades.']
    except FileNotFoundError:
        return ['netsh não encontrado.']
    except Exception as e:
        return [f'Erro: {e}']


# ===========================================================================
# HTML — TEMPLATES INLINE
# Dois templates: KIOSK (ecrã 7" do Pi) e MOBILE (telemóvel/tablet).
# O SocketIO.js é incluído localmente para funcionar offline.
# ===========================================================================

# JavaScript partilhado pelas duas interfaces (lógica de SocketIO + ações)
_JS_SHARED = """
<script src="https://cdn.socket.io/4.7.5/socket.io.min.js"></script>
<script>
const socket = io();

// recebe atualização completa de estado
socket.on('state_update', (s) => {
    applyState(s);
});

// recebe só mudança de estágio da série
socket.on('serial_stage', (data) => {
    applySerialStage(data.stage);
});

function applyState(s) {
    // modo DEV/CLIENTE
    const isDev = s.devmode === 1;
    const modeBtn = document.getElementById('modeBtn');
    if (modeBtn) {
        modeBtn.textContent = isDev ? 'MODO DEV' : 'MODO CLIENTE';
        modeBtn.className = 'btn-mode ' + (isDev ? 'dev' : 'cliente');
    }
    document.querySelectorAll('.dev-only').forEach(el => {
        el.style.display = isDev ? '' : 'none';
    });

    // start / stop
    const startBtn = document.getElementById('startBtn');
    const stopBtn  = document.getElementById('stopBtn');
    if (startBtn) startBtn.className = 'btn-big ' + (s.start_state === 1 ? 'on-green'  : 'off-green');
    if (stopBtn)  stopBtn.className  = 'btn-big ' + (s.stop_state  === 1 ? 'on-red'    : 'off-red');

    // modo de movimento
    document.querySelectorAll('.mode-option').forEach(btn => {
        btn.classList.toggle('mode-selected', btn.dataset.mode === s.mode_state);
    });

    // histórico serial
    const mon = document.getElementById('serialMonitor');
    if (mon) {
        const tail = (s.history_tail || []).join('\\n');
        if (mon.value !== tail) {
            mon.value = tail;
            if (window.autoScroll) mon.scrollTop = mon.scrollHeight;
        }
    }

    applySerialStage(s.serial_stage);
}

function applySerialStage(stage) {
    document.querySelectorAll('.conn-indicator').forEach(btn => {
        btn.dataset.stage = stage;
        const map = {
            disconnected: {icon: '🔗', cls: 'conn-off',       tip: 'Desligado'},
            connecting:   {icon: '↻',  cls: 'conn-connecting', tip: 'A ligar...'},
            connected:    {icon: '🔗', cls: 'conn-on',         tip: 'Ligado'},
            error:        {icon: '⚠',  cls: 'conn-error',      tip: 'Erro de ligação'},
        };
        const m = map[stage] || map.disconnected;
        btn.textContent  = m.icon;
        btn.className    = 'conn-indicator ' + m.cls;
        btn.title        = m.tip;
    });
}

window.autoScroll = true;

function toggleAutoScroll() {
    window.autoScroll = !window.autoScroll;
    const btn = document.getElementById('scrollBtn');
    if (btn) {
        btn.classList.toggle('active', window.autoScroll);
        btn.title = 'Auto-scroll: ' + (window.autoScroll ? 'ON' : 'OFF');
    }
}

function sendAction(action, body={}) {
    fetch('/' + action, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(body),
    }).catch(err => console.error('Erro:', err));
}

function onModeBtn() {
    const isDev = document.getElementById('modeBtn')
                    .classList.contains('dev');
    if (isDev) {
        sendAction('toggle_devmode');
    } else {
        document.getElementById('passwordModal').style.display = 'flex';
        document.getElementById('passwordInput').value = '';
        document.getElementById('passwordInput').focus();
    }
}

function confirmPassword() {
    const pw = document.getElementById('passwordInput').value;
    sendAction('toggle_devmode', {password: pw});
    document.getElementById('passwordModal').style.display = 'none';
}

function openModeDialog() {
    document.getElementById('modeModal').style.display = 'flex';
}

function closeModeDialog() {
    document.getElementById('modeModal').style.display = 'none';
}

function selectMode(modeName) {
    sendAction('select_mode', {mode: modeName});
    closeModeDialog();
}

function onConnClick(btn) {
    const stage = btn.dataset.stage || 'disconnected';
    const isDev = document.getElementById('modeBtn')
                    .classList.contains('dev');
    if (isDev) {
        if (stage === 'connecting' || stage === 'error') {
            document.getElementById('connMenu').style.display = 'flex';
        } else if (stage === 'connected') {
            sendAction('serial_disconnect');
        } else {
            sendAction('serial_connect');
        }
    } else {
        if (stage === 'error') {
            sendAction('serial_reconnect');
        }
    }
}

function closeConnMenu() {
    document.getElementById('connMenu').style.display = 'none';
}

// fecha modais ao clicar fora
document.addEventListener('click', (e) => {
    const connMenu = document.getElementById('connMenu');
    if (connMenu && !connMenu.contains(e.target) &&
        !e.target.classList.contains('conn-indicator')) {
        connMenu.style.display = 'none';
    }
});

// fecha modal de password com Enter
document.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
        const pwModal = document.getElementById('passwordModal');
        if (pwModal && pwModal.style.display === 'flex') confirmPassword();
    }
});
</script>
"""

# CSS partilhado
_CSS_SHARED = """
:root {
    --bg:          #f0f0f0;
    --panel:       #ffffff;
    --border:      #cccccc;
    --text:        #333333;
    --accent:      #337ab7;
    --accent-dark: #225a8a;
    --green-on:    #28a745;
    --green-off:   #1f7a33;
    --red-on:      #d9534f;
    --red-off:     #9f3b38;
    --yellow:      #ffd84d;
    --gray:        #6c757d;
    --radius:      10px;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: Arial, sans-serif; background: var(--bg); color: var(--text); }

/* Botão de modo */
.btn-mode {
    font-weight: bold; font-size: 15px; padding: 10px 18px;
    border-radius: 14px; border: 2px solid; cursor: pointer; min-width: 140px;
}
.btn-mode.cliente { background: var(--accent); color: white; border-color: #8ec5ff; }
.btn-mode.dev     { background: var(--yellow); color: black; border-color: #d9534f; }

/* Botões grandes Start/Stop */
.btn-big {
    font-weight: bold; font-size: 18px; padding: 16px 8px;
    border-radius: var(--radius); border: 2px solid black;
    cursor: pointer; width: 140px; height: 70px;
}
.btn-big.on-green  { background: var(--green-on);  color: white; box-shadow: 0 0 12px 3px rgba(40,167,69,.7); }
.btn-big.off-green { background: var(--green-off); color: #ccc; }
.btn-big.on-red    { background: var(--red-on);    color: white; box-shadow: 0 0 12px 3px rgba(217,83,79,.7); }
.btn-big.off-red   { background: var(--red-off);   color: #ccc; }

/* Botão Mode */
.btn-mode-action {
    font-weight: bold; font-size: 18px; padding: 16px 8px;
    border-radius: var(--radius); border: 2px solid black;
    background: var(--accent); color: white;
    cursor: pointer; width: 140px; height: 70px;
}

/* Indicador de ligação série */
.conn-indicator {
    font-size: 18px; padding: 6px 10px; border-radius: 8px;
    border: none; cursor: pointer; font-weight: bold;
    width: 40px; height: 40px; display: flex; align-items: center; justify-content: center;
}
.conn-off        { background: #e0e0e0; color: #888; }
.conn-connecting { background: #fff3cd; color: #b8860b;
                   animation: blink 1s infinite; }
.conn-on         { background: #e6f7ea; color: var(--green-on); }
.conn-error      { background: #fdeaea; color: var(--red-on);
                   animation: blink 1s infinite; }
@keyframes blink {
    0%,100% { opacity:1; } 50% { opacity:0.3; }
}

/* Botões de modo de movimento */
.mode-option {
    padding: 12px 8px; border-radius: var(--radius);
    border: 2px solid black; cursor: pointer; font-weight: bold;
    background: #d1d5db; color: black; font-size: 14px;
    transition: background .15s;
}
.mode-option.mode-selected {
    background: var(--yellow); border-color: black;
}

/* Monitor serial */
.serial-monitor {
    width: 100%; font-family: monospace; font-size: 13px;
    background: #1e1e1e; color: #d4d4d4; border: none;
    border-radius: 8px; padding: 10px; resize: none;
}

/* Botão de controlo */
.btn-ctrl {
    padding: 10px 16px; border-radius: 8px; border: none;
    cursor: pointer; font-weight: bold; font-size: 14px; color: white;
}
.btn-ctrl.danger  { background: #d9534f; }
.btn-ctrl.dark    { background: black;
                    box-shadow: 0 0 0 2px white, 0 0 0 4px black; }
.btn-ctrl.gray    { background: var(--gray); }
.btn-ctrl.blue    { background: var(--accent); }
.btn-ctrl.scroll-btn { background: #e0e0e0; color: #555; }
.btn-ctrl.scroll-btn.active { background: var(--accent); color: white; }

/* Modais */
.modal-overlay {
    display: none; position: fixed; inset: 0;
    background: rgba(0,0,0,.45);
    align-items: center; justify-content: center; z-index: 1000;
}
.modal-card {
    background: white; border-radius: 16px; padding: 24px;
    box-shadow: 0 18px 50px rgba(0,0,0,.25);
    max-width: 95vw;
}
.modal-card input {
    width: 100%; padding: 10px; border: 1px solid #ccc;
    border-radius: 8px; font-size: 16px; margin: 10px 0;
}
"""

# Template da interface KIOSK (ecrã 7" do Pi)
TEMPLATE_KIOSK = """<!DOCTYPE html>
<html lang="pt">
<head>
<meta charset="UTF-8">
<title>PI_4B Control Panel</title>
<style>
""" + _CSS_SHARED + """
body { padding: 12px; height: 100vh; overflow: hidden; }
.layout { display: flex; gap: 12px; height: calc(100vh - 24px); }

/* Coluna esquerda */
.left-col {
    flex: 3; display: flex; flex-direction: column;
    justify-content: space-between;
}
.top-row  { display: flex; align-items: center; gap: 10px; }
.mid-row  { display: flex; justify-content: center; align-items: center; gap: 16px; }
.bot-row  { display: flex; gap: 10px; }

/* Coluna direita — monitor serial (só visível em modo DEV) */
.right-col {
    flex: 2; display: flex; flex-direction: column;
    background: white; border-radius: var(--radius);
    padding: 10px; gap: 8px; min-height: 0;
}
.monitor-header {
    display: flex; align-items: center; justify-content: space-between;
}
.serial-monitor { flex: 1; min-height: 0; }

/* Painel de definições (canto inferior direito, posição fixa) */
.settings-wrap {
    position: fixed; bottom: 16px; right: 16px;
    align-items: flex-end; display: flex; flex-direction: column; z-index: 500;
}
.settings-panel {
    background: #f0f0f0; border: 1px solid #ccc; border-radius: 10px;
    padding: 8px; min-width: 180px;
    margin-bottom: -52px; padding-bottom: 60px; z-index: 1;
}
.settings-panel.hidden { display: none; }
.settings-panel .btn-settings-item {
    display: flex; align-items: center; gap: 8px; width: 100%;
    padding: 8px 10px; border-radius: 8px; border: none; background: var(--accent);
    color: white; font-weight: bold; cursor: pointer; margin-bottom: 6px;
    font-size: 14px;
}
.settings-panel .subpanel.hidden { display: none; }

/* QR code */
.qr-section { text-align: center; margin-bottom: 8px; }
.qr-section img { width: 130px; height: 130px; }
.qr-url { font-size: 10px; color: #555; word-break: break-all; margin-top: 4px; }

/* Separador */
hr { border: none; border-top: 1px solid #ccc; margin: 6px 0; }

/* Menu de conexão */
.conn-menu {
    display: none; flex-direction: column; gap: 6px;
    position: absolute; top: 50px; left: 0;
    background: white; border: 1px solid #ccc;
    border-radius: 10px; padding: 8px; min-width: 160px;
    box-shadow: 0 4px 12px rgba(0,0,0,.15); z-index: 600;
}
.conn-menu-wrap { position: relative; }
</style>
</head>
<body>
<div class="layout">

  <!-- COLUNA ESQUERDA -->
  <div class="left-col">

    <!-- Linha do topo: Modo + Indicador de ligação -->
    <div class="top-row">
      <button id="modeBtn" class="btn-mode cliente" onclick="onModeBtn()">MODO CLIENTE</button>
      <div class="conn-menu-wrap">
        <button class="conn-indicator conn-off" onclick="onConnClick(this)" title="Desligado">🔗</button>
        <div class="conn-menu" id="connMenu">
          <button class="btn-ctrl blue" onclick="sendAction('serial_reconnect'); closeConnMenu();">Tentar reconectar</button>
          <button class="btn-ctrl danger" onclick="sendAction('serial_disconnect'); closeConnMenu();">Desligar COM</button>
        </div>
      </div>
    </div>

    <!-- Linha do meio: Start / Mode / Stop -->
    <div class="mid-row">
      <button id="startBtn" class="btn-big off-green" onclick="sendAction('start')">Start</button>
      <button class="btn-mode-action" onclick="openModeDialog()">Mode</button>
      <button id="stopBtn"  class="btn-big on-red"   onclick="sendAction('stop')">Stop</button>
    </div>

    <!-- Linha do fundo: Desligar + Sair (DEV only) -->
    <div class="bot-row">
      <button class="btn-ctrl danger" onclick="sendAction('desligar')">Desligar</button>
      <button class="btn-ctrl dark dev-only" style="display:none"
              onclick="if(confirm('Encerrar o programa completamente?')) sendAction('sair')">Sair</button>
    </div>
  </div>

  <!-- COLUNA DIREITA: Monitor Serial (só no modo DEV) -->
  <div class="right-col dev-only" style="display:none">
    <div class="monitor-header">
      <strong>Monitor Serial</strong>
      <div style="display:flex; gap:6px">
        <button id="scrollBtn" class="btn-ctrl scroll-btn active"
                onclick="toggleAutoScroll()" title="Auto-scroll: ON">⬇</button>
        <button class="conn-indicator conn-off" onclick="onConnClick(this)" title="Desligado">🔗</button>
      </div>
    </div>
    <textarea id="serialMonitor" class="serial-monitor" readonly></textarea>
    <div style="display:flex; gap:8px">
      <input id="serialInput" type="text" placeholder="Escrever comando..."
             style="flex:1; padding:8px; border-radius:8px; border:1px solid #ccc; font-size:14px;"
             onkeydown="if(event.key==='Enter') sendManual()">
      <button class="btn-ctrl blue" onclick="sendManual()">Enviar</button>
    </div>
  </div>

</div>

<!-- PAINEL DE DEFINIÇÕES (canto inferior direito) -->
<div class="settings-wrap">
  <div class="settings-panel hidden" id="settingsPanel">

    <!-- QR Code + URL -->
    <div id="qrSubpanel">
      <div class="qr-section">
        {% if qr_img %}
          <img src="{{ qr_img }}" alt="QR code">
        {% else %}
          <small style="color:#888">QR code indisponível<br>(pip install qrcode[pil])</small>
        {% endif %}
        <div class="qr-url">{{ app_url }}</div>
      </div>
      <hr>
      <!-- Menu principal das definições -->
      <div id="defMenu">
        <button class="btn-settings-item" onclick="showSubpanel('comSubpanel')">🔌 Porta COM</button>
        <button class="btn-settings-item" onclick="showSubpanel('redeSubpanel')">📶 Rede</button>
        <button class="btn-settings-item" onclick="showSubpanel('configSubpanel')">⚙ Configurações</button>
      </div>
    </div>

    <!-- Subpainel: Porta COM -->
    <div id="comSubpanel" class="subpanel hidden">
      <strong style="font-size:13px">Portas COM disponíveis</strong>
      <div id="comList" style="margin-top:6px; font-size:12px">A procurar...</div>
    </div>

    <!-- Subpainel: Rede -->
    <div id="redeSubpanel" class="subpanel hidden">
      <strong style="font-size:13px">Estado da rede</strong>
      <div id="redeStatus" style="margin-top:4px; font-size:12px"></div>
      <strong style="font-size:13px; margin-top:8px; display:block">Redes disponíveis</strong>
      <div id="redeList" style="margin-top:4px; font-size:12px">A procurar...</div>
    </div>

    <!-- Subpainel: Configurações (placeholder) -->
    <div id="configSubpanel" class="subpanel hidden">
      <strong style="font-size:13px">Configurações</strong>
      <div style="font-size:12px; margin-top:6px; color:#888">Em desenvolvimento.</div>
    </div>

  </div>

  <button class="btn-ctrl gray" id="defBtn"
          onclick="toggleSettings()" style="width:150px; height:40px; position:relative; z-index:2;">
    ⚙ Definições
  </button>
</div>

<!-- MODAL: Password DEV -->
<div class="modal-overlay" id="passwordModal">
  <div class="modal-card" style="width:320px">
    <h3 style="margin-bottom:8px">Acesso DEV</h3>
    <p>Introduza a password DEV:</p>
    <input id="passwordInput" type="password" placeholder="Password">
    <div style="display:flex; justify-content:flex-end; gap:8px; margin-top:8px">
      <button class="btn-ctrl gray" onclick="document.getElementById('passwordModal').style.display='none'">Cancelar</button>
      <button class="btn-ctrl blue" onclick="confirmPassword()">Entrar</button>
    </div>
  </div>
</div>

<!-- MODAL: Selecionar Modo -->
<div class="modal-overlay" id="modeModal" onclick="if(event.target.id==='modeModal') closeModeDialog()">
  <div class="modal-card">
    <h3 style="margin-bottom:16px">Selecionar Modo</h3>
    <div style="display:flex; gap:10px; flex-wrap:wrap; justify-content:center">
      <div style="display:flex; flex-direction:column; align-items:center; gap:4px">
        <button class="mode-option" data-mode="Estatico" onclick="selectMode('Estatico')">Estático</button>
      </div>
      <div style="display:flex; flex-direction:column; align-items:center; gap:4px">
        <button class="mode-option" data-mode="Respiracao" onclick="selectMode('Respiracao')">Respiração</button>
      </div>
      <div style="display:flex; flex-direction:column; align-items:center; gap:4px">
        <button class="mode-option" data-mode="Coracao" onclick="selectMode('Coracao')">Coração</button>
      </div>
      <div style="display:flex; flex-direction:column; align-items:center; gap:4px">
        <button class="mode-option" data-mode="Humano" onclick="selectMode('Humano')">Humano</button>
        <small style="color:#555">(Resp+Bati)</small>
      </div>
      <div style="display:flex; flex-direction:column; align-items:center; gap:4px">
        <button class="mode-option" data-mode="Completo" onclick="selectMode('Completo')">Completo</button>
        <small style="color:#555">(Resp+Bati+Tosse)</small>
      </div>
    </div>
  </div>
</div>

""" + _JS_SHARED + """
<script>
function sendManual() {
    const input = document.getElementById('serialInput');
    const cmd = input.value.trim();
    if (!cmd) return;
    sendAction('send_serial', {command: cmd});
    input.value = '';
}

// --- Painel de definições ---
let settingsView = 'closed'; // closed | menu | com | rede | config

function toggleSettings() {
    if (settingsView === 'closed') {
        settingsView = 'menu';
    } else if (settingsView === 'menu') {
        settingsView = 'closed';
    } else {
        settingsView = 'menu'; // voltar ao menu principal
    }
    applySettingsView();
}

function showSubpanel(subId) {
    settingsView = subId.replace('Subpanel', '');
    applySettingsView();
    if (subId === 'comSubpanel')   loadComPorts();
    if (subId === 'redeSubpanel')  loadRede();
}

function applySettingsView() {
    const panel  = document.getElementById('settingsPanel');
    const qr     = document.getElementById('qrSubpanel');
    const defMenu = document.getElementById('defMenu');
    const subpanels = ['comSubpanel', 'redeSubpanel', 'configSubpanel'];

    panel.classList.toggle('hidden', settingsView === 'closed');
    const isMenu = settingsView === 'menu';
    qr.style.display = isMenu ? '' : 'none';

    subpanels.forEach(id => {
        document.getElementById(id).classList.toggle('hidden',
            settingsView !== id.replace('Subpanel', ''));
    });
}

function loadComPorts() {
    document.getElementById('comList').textContent = 'A procurar...';
    fetch('/info/com_ports').then(r => r.json()).then(data => {
        document.getElementById('comList').innerHTML =
            data.map(l => `<div style="margin:2px 0">${l}</div>`).join('');
    });
}

function loadRede() {
    document.getElementById('redeStatus').textContent = 'A verificar...';
    document.getElementById('redeList').textContent   = 'A procurar redes...';
    fetch('/info/network').then(r => r.json()).then(data => {
        document.getElementById('redeStatus').textContent = data.status;
        document.getElementById('redeList').innerHTML =
            data.networks.map(n => `<div style="margin:2px 0">${n}</div>`).join('');
    });
}

// aplica estado inicial ao carregar a página
fetch('/state').then(r => r.json()).then(applyState);
</script>
</body>
</html>
"""

# Template da interface MOBILE (telemóvel/tablet)
TEMPLATE_MOBILE = """<!DOCTYPE html>
<html lang="pt">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
<title>PI_4B Control Panel — Mobile</title>
<style>
""" + _CSS_SHARED + """
body { padding: 16px; }
.app  { max-width: 480px; margin: 0 auto; display: grid; gap: 14px; }
.card {
    background: var(--panel); border: 1px solid var(--border);
    border-radius: 20px; padding: 16px;
    box-shadow: 0 4px 8px rgba(0,0,0,.1);
}
.section-title {
    font-size: 13px; font-weight: bold; text-transform: uppercase;
    letter-spacing: .06em; color: #666; margin-bottom: 10px;
}
.brand { display: flex; align-items: center; gap: 12px; }
.brand h1 { font-size: 22px; }
.brand p  { font-size: 13px; color: #666; margin-top: 2px; }

/* Linha de topo: modo + conexão */
.top-row { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; }

/* Grid de botões */
.btn-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
.btn-grid.single { grid-template-columns: 1fr; }
.btn-grid .btn-big { width: 100%; height: 60px; font-size: 16px; }
.btn-grid .btn-mode-action { width: 100%; height: 60px; font-size: 16px; }

/* Monitor serial */
.serial-monitor { height: 220px; }

/* Conn menu mobile */
.conn-menu-wrap { position: relative; }
.conn-menu {
    display: none; flex-direction: column; gap: 6px;
    position: absolute; top: 50px; left: 0;
    background: white; border: 1px solid #ccc;
    border-radius: 14px; padding: 10px; min-width: 180px;
    box-shadow: 0 4px 16px rgba(0,0,0,.18); z-index: 600;
}
</style>
</head>
<body>
<main class="app">

  <!-- Cabeçalho -->
  <section class="brand">
    <div>
      <h1>PI_4B Control</h1>
      <p>Interface remota</p>
    </div>
  </section>

  <!-- Modo + Ligação -->
  <section class="card">
    <div class="section-title">Controlo</div>
    <div class="top-row">
      <button id="modeBtn" class="btn-mode cliente" onclick="onModeBtn()">MODO CLIENTE</button>
      <div class="conn-menu-wrap">
        <button class="conn-indicator conn-off" onclick="onConnClick(this)" title="Desligado">🔗</button>
        <div class="conn-menu" id="connMenu">
          <button class="btn-ctrl blue" onclick="sendAction('serial_reconnect'); closeConnMenu();">Tentar reconectar</button>
          <button class="btn-ctrl danger dev-only" style="display:none"
                  onclick="sendAction('serial_disconnect'); closeConnMenu();">Desligar COM</button>
        </div>
      </div>
    </div>
  </section>

  <!-- Start / Mode / Stop -->
  <section class="card">
    <div class="section-title">Ações</div>
    <div class="btn-grid">
      <button id="startBtn" class="btn-big off-green" onclick="sendAction('start')">Start</button>
      <button id="stopBtn"  class="btn-big on-red"   onclick="sendAction('stop')">Stop</button>
      <button class="btn-mode-action" style="grid-column:span 2"
              onclick="openModeDialog()">Mode</button>
    </div>
  </section>

  <!-- Monitor serial (só DEV) -->
  <section class="card dev-only" style="display:none">
    <div class="section-title" style="display:flex; justify-content:space-between">
      Monitor Serial
      <button id="scrollBtn" class="btn-ctrl scroll-btn active"
              onclick="toggleAutoScroll()" title="Auto-scroll: ON">⬇</button>
    </div>
    <textarea id="serialMonitor" class="serial-monitor" readonly></textarea>
    <div style="display:flex; gap:8px; margin-top:8px">
      <input id="serialInput" type="text" placeholder="Comando..."
             style="flex:1; padding:10px; border-radius:10px; border:1px solid #ccc; font-size:15px;"
             onkeydown="if(event.key==='Enter') sendManual()">
      <button class="btn-ctrl blue" onclick="sendManual()">Enviar</button>
    </div>
  </section>

  <!-- Desligar / Sair -->
  <section class="card">
    <div class="btn-grid">
      <button class="btn-ctrl danger" style="padding:14px"
              onclick="sendAction('desligar')">Desligar</button>
      <button class="btn-ctrl dark dev-only" style="padding:14px; display:none"
              onclick="if(confirm('Encerrar o programa completamente?')) sendAction('sair')">Sair</button>
    </div>
  </section>

</main>

<!-- MODAL: Password DEV -->
<div class="modal-overlay" id="passwordModal">
  <div class="modal-card" style="width:100%; max-width:360px">
    <h3 style="margin-bottom:8px">Acesso DEV</h3>
    <input id="passwordInput" type="password" placeholder="Password DEV">
    <div style="display:flex; gap:8px; justify-content:flex-end; margin-top:8px">
      <button class="btn-ctrl gray"
              onclick="document.getElementById('passwordModal').style.display='none'">Cancelar</button>
      <button class="btn-ctrl blue" onclick="confirmPassword()">Entrar</button>
    </div>
  </div>
</div>

<!-- MODAL: Selecionar Modo -->
<div class="modal-overlay" id="modeModal" onclick="if(event.target.id==='modeModal') closeModeDialog()">
  <div class="modal-card" style="width:100%; max-width:480px">
    <h3 style="margin-bottom:16px">Selecionar Modo</h3>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:10px">
      <button class="mode-option" data-mode="Estatico"   onclick="selectMode('Estatico')">Estático</button>
      <button class="mode-option" data-mode="Respiracao" onclick="selectMode('Respiracao')">Respiração</button>
      <button class="mode-option" data-mode="Coracao"    onclick="selectMode('Coracao')">Coração</button>
      <div style="display:flex; flex-direction:column; align-items:center; gap:4px">
        <button class="mode-option" style="width:100%" data-mode="Humano" onclick="selectMode('Humano')">Humano</button>
        <small style="color:#555">(Resp+Bati)</small>
      </div>
      <div style="display:flex; flex-direction:column; align-items:center; gap:4px">
        <button class="mode-option" style="width:100%" data-mode="Completo" onclick="selectMode('Completo')">Completo</button>
        <small style="color:#555">(Resp+Bati+Tosse)</small>
      </div>
    </div>
  </div>
</div>

""" + _JS_SHARED + """
<script>
function sendManual() {
    const input = document.getElementById('serialInput');
    const cmd = input.value.trim();
    if (!cmd) return;
    sendAction('send_serial', {command: cmd});
    input.value = '';
}

// redirect automático para desktop se não for mobile em portrait
function checkDevice() {
    const isMobile = /Android|iPhone|iPad|iPod|Mobile/i.test(navigator.userAgent);
    const isPortrait = window.innerHeight > window.innerWidth;
    if (!isMobile || !isPortrait) window.location.replace('/');
}
checkDevice();
window.addEventListener('resize', checkDevice);

fetch('/state').then(r => r.json()).then(applyState);
</script>
</body>
</html>
"""


# ===========================================================================
# FLASK ROUTES
# ===========================================================================
@app.route('/')
def index():
    app_url = get_app_url()
    qr_img  = generate_qr_base64(app_url + '/mobile')
    return render_template_string(TEMPLATE_KIOSK,
                                  app_url=app_url, qr_img=qr_img)


@app.route('/mobile')
def mobile():
    return render_template_string(TEMPLATE_MOBILE)


@app.route('/state')
def get_state_route():
    """Devolve o estado completo — chamado pelos browsers ao carregar a página."""
    return jsonify({
        'devmode':     state['devmode'],
        'start_state': state['start_state'],
        'stop_state':  state['stop_state'],
        'mode_state':  state['mode_state'],
        'serial_stage':serial_status['stage'],
        'history_tail':state['serialhistory'][-200:],
    })


@app.route('/info/com_ports')
def info_com_ports():
    return jsonify(list_com_ports())


@app.route('/info/network')
def info_network():
    return jsonify({
        'status':   get_network_status(),
        'networks': get_available_networks(),
    })


@app.route('/start', methods=['POST'])
def route_start():
    action_start()
    return jsonify({'ok': True})


@app.route('/stop', methods=['POST'])
def route_stop():
    action_stop()
    return jsonify({'ok': True})


@app.route('/select_mode', methods=['POST'])
def route_select_mode():
    data = request.get_json(force=True, silent=True) or {}
    action_select_mode(data.get('mode', 'Estatico'))
    return jsonify({'ok': True})


@app.route('/toggle_devmode', methods=['POST'])
def route_toggle_devmode():
    data = request.get_json(force=True, silent=True) or {}
    ok = action_toggle_devmode(data.get('password'))
    return jsonify({'ok': ok})


@app.route('/send_serial', methods=['POST'])
def route_send_serial():
    data = request.get_json(force=True, silent=True) or {}
    cmd  = (data.get('command') or '').strip()
    if cmd:
        send_serial(cmd)
    return jsonify({'ok': True})


@app.route('/serial_connect', methods=['POST'])
def route_serial_connect():
    serial_connect()
    return jsonify({'ok': True})


@app.route('/serial_disconnect', methods=['POST'])
def route_serial_disconnect():
    serial_disconnect()
    return jsonify({'ok': True})


@app.route('/serial_reconnect', methods=['POST'])
def route_serial_reconnect():
    threading.Thread(target=do_reconnect_sync, daemon=True).start()
    return jsonify({'ok': True})


@app.route('/desligar', methods=['POST'])
def route_desligar():
    """Fecha a janela do browser. O servidor Flask continua a correr."""
    try:
        # fecha o browser pelo título da janela no Windows
        subprocess.run(
            ['powershell', '-Command',
             'Get-Process | Where-Object {$_.MainWindowTitle -like "*PI_4B*"} | Stop-Process -Force'],
            capture_output=True, timeout=5
        )
    except Exception:
        pass
    return jsonify({'ok': True})


@app.route('/sair', methods=['POST'])
def route_sair():
    """Encerra o programa completamente (só modo DEV)."""
    if state['devmode'] != 1:
        return jsonify({'ok': False, 'error': 'Apenas disponível em modo DEV'}), 403

    def _shutdown():
        time.sleep(0.5)
        os._exit(0)

    threading.Thread(target=_shutdown, daemon=True).start()
    return jsonify({'ok': True})


# ===========================================================================
# ARRANQUE DO BROWSER (WINDOWS — modo de teste, sem kiosk)
# ===========================================================================
def open_browser():
    """Abre a interface no browser padrão do Windows, sem modo kiosk,
    para que possas fechar e navegar à vontade durante os testes."""
    time.sleep(1.5)
    import webbrowser
    webbrowser.open(f'http://127.0.0.1:{APP_PORT}/')


# ===========================================================================
# MAIN
# ===========================================================================
if __name__ == '__main__':
    reset_state()

    # Arranca threads de comunicação série
    serial_status['stage'] = 'connecting'
    threading.Thread(target=serial_reader, daemon=True, name='serial-reader').start()
    threading.Thread(target=serial_writer, daemon=True, name='serial-writer').start()

    # Abre o browser normalmente (sem kiosk — versão de teste Windows)
    threading.Thread(target=open_browser, daemon=True, name='browser').start()

    print(f'PI4B: Servidor a arrancar em http://127.0.0.1:{APP_PORT}')
    print(f'PI4B: Interface mobile em http://{get_local_ip()}:{APP_PORT}/mobile')

    socketio.run(
        app,
        host=APP_HOST,
        port=APP_PORT,
        debug=False,
        allow_unsafe_werkzeug=True,
    )