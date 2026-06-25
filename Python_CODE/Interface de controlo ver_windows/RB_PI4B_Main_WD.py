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

from flask import Flask, render_template_string, request, jsonify
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
shutdown_requested = False


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

                    # durante leituras sequenciais (reloadFromESP),
                    # coloca também na fila de resposta dedicada
                    if _esp_read_lock.locked():
                        try:
                            _esp_response_queue.put_nowait(data)
                        except Exception:
                            pass

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
        img.save(buf, 'PNG')
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
# ARMAZENAMENTO PERSISTENTE DE CONFIGURAÇÕES DE MOVIMENTOS (JSON)
# Ficheiro salvo na mesma pasta que o script, sobrevive a reinícios.
# ===========================================================================
MOVEMENTS_FILE = Path(__file__).parent / 'movements_config.json'

MOVEMENTS_DEFAULTS = {
    'curva1': {
        'a': 5, 'b': 2, 'xc': 3.6, 'yc': 3.6,
        'alpha_deg': 45, 'n_curva': 300, 'n_pontos': 10, 'sentido': 'ccw',
    },
    'curva2': {
        'altura': 20, 'largura': 12, 'n_curva': 300,
        'n_pontos': 10, 'theta_deg': 0,
    },
    'curva3': {
        'raio': 2, 'n_curva': 300, 'n_pontos': 10, 'sentido': 'ccw',
    },
    'respiracao': {
        'periodo': 4.0, 'espera_inicio': 0.5, 'espera_fim': 0.5,
        'ponto_inicio': 0,
        'pontos_x': [], 'pontos_y': [], 'pontos_z': [],
    },
    'batimento': {
        'periodo': 1.0, 'espera_inicio': 0.1, 'espera_fim': 0.1,
        'ponto_inicio': 0,
        'pontos_x': [], 'pontos_y': [], 'pontos_z': [],
    },
    'tosse': {
        'periodo': 0.5, 'espera_inicio': 0.2, 'espera_fim': 0.8,
        'ponto_inicio': 0,
        'pontos_x': [], 'pontos_y': [], 'pontos_z': [],
    },
    'vibracao_tosse': {
        'periodo': 0.2, 'espera_inicio': 0.1, 'espera_fim': 0.1,
        'ponto_inicio': 0,
        'pontos_x': [], 'pontos_y': [], 'pontos_z': [],
    },
}


def load_movements():
    """Lê as configurações do ficheiro JSON. Se não existir, devolve os defaults."""
    try:
        return json.loads(MOVEMENTS_FILE.read_text(encoding='utf-8'))
    except (FileNotFoundError, json.JSONDecodeError):
        return json.loads(json.dumps(MOVEMENTS_DEFAULTS))


def save_movements(data):
    """Guarda as configurações no ficheiro JSON."""
    MOVEMENTS_FILE.write_text(json.dumps(data, indent=2, ensure_ascii=False),
                              encoding='utf-8')


# Fila e lock para leitura sequencial do ESP32
_esp_response_queue = queue.Queue()
_esp_read_lock      = threading.Lock()


def _serial_read_response(timeout=5.0):
    """Aguarda uma resposta do ESP32 (bloqueia até timeout segundos).
    Chamado durante a leitura sequencial de configurações."""
    try:
        return _esp_response_queue.get(timeout=timeout)
    except queue.Empty:
        return None


def _send_and_wait(command, timeout=6.0):
    """Envia um comando e aguarda a resposta completa do ESP32.
    A resposta é considerada completa quando chega uma linha vazia
    ou uma linha que começa por 'Escolha um comando:' (prompt de menu)."""
    lines = []
    with _esp_read_lock:
        # limpa respostas pendentes anteriores
        while not _esp_response_queue.empty():
            try:
                _esp_response_queue.get_nowait()
            except queue.Empty:
                break

        if _ser and _ser.is_open:
            _ser.write(command.encode('utf-8'))

        deadline = time.time() + timeout
        while time.time() < deadline:
            line = _serial_read_response(timeout=0.5)
            if line is None:
                break
            lines.append(line)
            if 'Escolha um comando:' in line or line.strip() == '':
                break

    return lines


# ===========================================================================
# HTML — TEMPLATES (carregados dos ficheiros .html separados)
# RB_PI4B_Main_WD_template_PC.html  — interface desktop/kiosk
# RB_PI4B_Main_WD_template_MB.html  — interface mobile/tablet
# ===========================================================================
def _load_template(filename):
    """Lê o ficheiro HTML template na mesma pasta que este script."""
    path = Path(__file__).parent / filename
    try:
        return path.read_text(encoding='utf-8')
    except FileNotFoundError:
        return (f'<h1>Template não encontrado: {filename}</h1>'
                f'<p>Certifica-te que o ficheiro está na mesma pasta que RB_PI4B_Main_WD.py</p>')

TEMPLATE_KIOSK  = _load_template('RB_PI4B_Main_WD_template_PC.html')
TEMPLATE_MOBILE = _load_template('RB_PI4B_Main_WD_template_MB.html')



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


def schedule_app_shutdown(delay=3.0):
    global shutdown_requested
    shutdown_requested = True

    def _notify_clients():
        for _ in range(6):
            socketio.emit('app_shutdown', {'shutdown': True})
            time.sleep(0.25)

    socketio.start_background_task(_notify_clients)

    def _shutdown():
        time.sleep(delay)
        serial_status['stop_reader'] = True
        try:
            if _ser and _ser.is_open:
                _ser.close()
        except Exception:
            pass
        os._exit(0)

    threading.Thread(target=_shutdown, daemon=True, name='app-shutdown').start()


@app.route('/desligar', methods=['POST'])
def route_desligar():
    """Encerra o programa completo, equivalente a parar com Ctrl+C."""
    schedule_app_shutdown()
    return jsonify({'ok': True})


@app.route('/shutdown_status')
def route_shutdown_status():
    return jsonify({'shutdown': shutdown_requested})


@app.route('/sair', methods=['POST'])
def route_sair():
    """Encerra o programa completamente (só modo DEV)."""
    if state['devmode'] != 1:
        return jsonify({'ok': False, 'error': 'Apenas disponível em modo DEV'}), 403

    schedule_app_shutdown()
    return jsonify({'ok': True})


@app.route('/configuracoes_movimentos')
def route_configuracoes_movimentos():
    """Página de configurações de movimentos — acedida pelo mobile via link."""
    app_url = get_app_url()
    qr_img  = generate_qr_base64(app_url + '/mobile')
    return render_template_string(TEMPLATE_KIOSK,
                                  app_url=app_url, qr_img=qr_img)


@app.route('/movements/config', methods=['GET'])
def route_movements_get():
    """Devolve as configurações de movimentos guardadas em JSON."""
    return jsonify(load_movements())


@app.route('/movements/config', methods=['POST'])
def route_movements_save():
    """Guarda as configurações de movimentos no ficheiro JSON."""
    data = request.get_json(force=True, silent=True) or {}
    existing = load_movements()
    existing.update(data)
    save_movements(existing)
    return jsonify({'ok': True})


@app.route('/movements/reload_from_esp', methods=['POST'])
def route_movements_reload():
    """Lê sequencialmente os dados de cada movimento diretamente do ESP32.
    Envia lr → lê resposta → lb → lê resposta → lt → lê resposta.
    Atualiza o JSON guardado com os dados recebidos."""
    if serial_status['stage'] != 'connected':
        return jsonify({'ok': False, 'error': 'ESP32 não está ligado.'}), 400

    config = load_movements()
    erros  = []

    comandos = [
        ('lr', 'respiracao'),
        ('lb', 'batimento'),
        ('lt', 'tosse'),
    ]

    for cmd, chave in comandos:
        append_history(f'PI4B: {cmd}')
        linhas = _send_and_wait(cmd, timeout=6.0)

        if not linhas:
            erros.append(f'Sem resposta para {cmd}')
            continue

        # tenta interpretar as linhas de resposta do ESP32
        # formato esperado (a confirmar com o firmware real):
        # periodo:4.0
        # espera_inicio:0.5
        # espera_fim:0.5
        # ponto_inicio:0
        # x:1.23,2.34,3.45,...
        # y:0.12,0.23,...
        # z:0.00,0.00,...
        parsed = {}
        for linha in linhas:
            if ':' not in linha:
                continue
            chave_linha, _, valor_linha = linha.partition(':')
            chave_linha = chave_linha.strip().lower()
            valor_linha = valor_linha.strip()
            if chave_linha in ('periodo', 'espera_inicio', 'espera_fim'):
                try:
                    parsed[chave_linha] = float(valor_linha)
                except ValueError:
                    pass
            elif chave_linha == 'ponto_inicio':
                try:
                    parsed[chave_linha] = int(valor_linha)
                except ValueError:
                    pass
            elif chave_linha in ('x', 'y', 'z'):
                try:
                    parsed[f'pontos_{chave_linha}'] = [
                        float(v) for v in valor_linha.split(',') if v.strip()
                    ]
                except ValueError:
                    pass

        if parsed:
            config[chave].update(parsed)
            append_history(f'ESP32: dados {chave} recebidos ({len(parsed)} campos)')
        else:
            erros.append(f'Resposta de {cmd} não reconhecida')

    save_movements(config)
    return jsonify({
        'ok': len(erros) == 0,
        'config': config,
        'erros': erros,
    })


@app.route('/movements/calcular', methods=['POST'])
def route_movements_calcular():
    """Executa o calculo_movimentos.py com os parâmetros atuais e
    devolve os gráficos como base64 e os pontos calculados."""
    try:
        import calculo_movimentos as cm
        data    = request.get_json(force=True, silent=True) or {}
        config  = load_movements()

        params_c1 = {**config.get('curva1', {}), **data.get('curva1', {})}
        params_c2 = {**config.get('curva2', {}), **data.get('curva2', {})}
        params_c3 = {**config.get('curva3', {}), **data.get('curva3', {})}

        resultado = cm.gerar_graficos(params_c1, params_c2, params_c3)

        # guarda os pontos calculados nas configurações dos movimentos
        config['respiracao']['pontos_x'] = resultado['pontos_c1']['x']
        config['respiracao']['pontos_y'] = resultado['pontos_c1']['y']
        config['respiracao']['pontos_z'] = resultado['pontos_c1']['z']
        config['batimento']['pontos_x']  = resultado['pontos_c1']['x']
        config['batimento']['pontos_y']  = resultado['pontos_c1']['y']
        config['batimento']['pontos_z']  = resultado['pontos_c1']['z']
        config['tosse']['pontos_x']      = resultado['pontos_c2']['x']
        config['tosse']['pontos_y']      = resultado['pontos_c2']['y']
        config['tosse']['pontos_z']      = resultado['pontos_c2']['z']
        config['vibracao_tosse']['pontos_x'] = resultado['pontos_c3']['x']
        config['vibracao_tosse']['pontos_y'] = resultado['pontos_c3']['y']
        config['vibracao_tosse']['pontos_z'] = resultado['pontos_c3']['z']
        save_movements(config)

        return jsonify({'ok': True,
                        'graficos':  resultado['graficos'],
                        'pontos_c1': resultado['pontos_c1'],
                        'pontos_c2': resultado['pontos_c2'],
                        'pontos_c3': resultado['pontos_c3']})

    except ImportError:
        return jsonify({'ok': False,
                        'error': 'calculo_movimentos.py não encontrado na mesma pasta.'}), 500
    except Exception as e:
        return jsonify({'ok': False, 'error': str(e)}), 500


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
