"""
from nicegui import ui

ui.label('Hello NiceGUI')
ui.run(native=True, reload=False, host='0.0.0.0', port=8081)
"""
import asyncio
from nicegui import ui, app, run

PASSWORD = '1234'

# ---------------------------------------------------------------------------
# Estado global partilhado entre TODOS os clientes (janela + browser)
# Guardado em app.storage.general — persiste e é visível por todos.
# ---------------------------------------------------------------------------
def get_state():
    s = app.storage.general
    if 'devmode' not in s:
        s['devmode'] = 0
    if 'serialhistory' not in s:
        s['serialhistory'] = [
            'Zona reservada para mensagens enviadas e recebidas pela serial.',
            'A comunicação será ligada mais tarde.',
        ]
    if 'settings_view' not in s:
        # 'closed' | 'menu' | 'com' | 'rede'
        s['settings_view'] = 'closed'
    if 'start_state' not in s:
        s['start_state'] = 0
    if 'stop_state' not in s:
        s['stop_state'] = 1
    if 'mode_state' not in s:
        s['mode_state'] = 'Estatico'
    if 'mode_dialog_open' not in s:
        s['mode_dialog_open'] = False
    return s


def reset_state():
    app.storage.general['devmode'] = 0
    app.storage.general['serialhistory'] = [
        'Zona reservada para mensagens enviadas e recebidas pela serial.',
        'A comunicação será ligada mais tarde.',
    ]
    app.storage.general['settings_view'] = 'closed'
    app.storage.general['start_state'] = 0
    app.storage.general['stop_state'] = 1
    app.storage.general['mode_state'] = 'Estatico'
    app.storage.general['mode_dialog_open'] = False


def printpi(args):
    print('PI4B:', args)


def close_native_window():
    if getattr(app, 'native', None) and app.native.main_window:
        app.native.main_window.destroy()


def shutdown_app():
    app.shutdown()


def get_app_url():
    import socket
    port = 8081
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        local_ip = s.getsockname()[0]
        s.close()
        return f'http://{local_ip}:{port}'
    except Exception:
        return f'http://localhost:{port}'


def generate_qr_code_base64(data):
    try:
        import qrcode
        import io
        import base64

        qr = qrcode.QRCode(border=1, box_size=6)
        qr.add_data(data)
        qr.make(fit=True)
        img = qr.make_image(fill_color='black', back_color='white')

        buffer = io.BytesIO()
        img.save(buffer, format='PNG')
        encoded = base64.b64encode(buffer.getvalue()).decode('ascii')
        return f'data:image/png;base64,{encoded}'
    except ImportError:
        return None
    except Exception as e:
        print('Erro ao gerar QR code:', e)
        return None


def list_com_ports():
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            return ['Nenhuma porta COM encontrada.']
        return [f'{p.device} — {p.description}' for p in ports]
    except ImportError:
        return ['Biblioteca pyserial não instalada (pip install pyserial).']
    except Exception as e:
        return [f'Erro ao listar portas COM: {e}']


def _run_netsh(args):
    import subprocess
    raw = subprocess.run(['netsh'] + args, capture_output=True, timeout=5).stdout

    for encoding in ('cp850', 'cp1252', 'utf-8', 'utf-16-le'):
        try:
            return raw.decode(encoding)
        except (UnicodeDecodeError, AttributeError):
            continue
    return raw.decode('utf-8', errors='replace')


def _run_powershell(command):
    import subprocess
    result = subprocess.run(
        ['powershell', '-NoProfile', '-Command', command],
        capture_output=True, text=True, timeout=8
    )
    return result.stdout.strip()


def get_network_status():
    try:
        ps_adapters = _run_powershell(
            "Get-NetAdapter | Where-Object {$_.Status -eq 'Up'} | "
            "Select-Object Name, InterfaceDescription | Format-List"
        )

        wifi_up = False
        lan_up = False
        for block in ps_adapters.split('\n\n'):
            lower = block.lower()
            if 'wi-fi' in lower or 'wireless' in lower or 'wlan' in lower:
                wifi_up = True
            if 'ethernet' in lower or 'lan' in lower:
                lan_up = True

        if wifi_up:
            ssid = _run_powershell(
                "(Get-NetConnectionProfile | "
                "Where-Object {$_.InterfaceAlias -match 'Wi-Fi|Wireless|WLAN'}).Name"
            )
            if ssid:
                return [f'Wi-Fi ligado: {ssid}']
            return ['Wi-Fi ligado (nome da rede não identificado).']

        if lan_up:
            return ['Ligado por LAN (cabo de rede).']

        return ['Nenhuma ligação de rede ativa detetada.']

    except FileNotFoundError:
        return ['Não foi possível determinar o estado da rede neste sistema.']
    except Exception as e:
        return [f'Erro ao verificar rede: {e}']


def get_available_networks():
    try:
        output = _run_netsh(['wlan', 'show', 'networks'])

        if 'location permission' in output.lower() or 'location services' in output.lower():
            return [
                'O Windows está a bloquear a procura de redes.',
                'Ativa em: Definições > Privacidade e segurança > Localização > permite que apps de desktop acedam à localização.',
            ]

        if not output.strip():
            return ['Sem adaptador Wi-Fi disponível para procurar redes.']

        names = []
        for raw_line in output.splitlines():
            if ':' not in raw_line:
                continue
            key, _, value = raw_line.partition(':')
            key = key.strip().lower()
            value = value.strip()

            if key.startswith('ssid') and key[4:].strip().isdigit() and value:
                names.append(value)

        if not names:
            return ['Nenhuma rede Wi-Fi encontrada nas proximidades.']
        return names

    except FileNotFoundError:
        return ['Não foi possível procurar redes neste sistema.']
    except Exception as e:
        return [f'Erro ao procurar redes: {e}']


@ui.page('/')
def main_page():
    ui.add_head_html('''
    <style>
        body, .nicegui-content {background-color: white;}
        .mode-btn-default {
            background: #d1d5db !important;
            color: black !important;
            border: 2px solid black !important;
            border-radius: 12px !important;
            font-weight: bold !important;
            box-shadow: none !important;
        }
        .mode-btn-selected {
            background: linear-gradient(135deg, #ffe066 0%, #ffc93c 45%, #ff9f1c 100%) !important;
            color: black !important;
            border: 3px solid #c53030 !important;
            border-radius: 12px !important;
            font-weight: bold !important;
            box-shadow: 0 0 14px 4px rgba(255, 190, 60, 0.95) !important;
        }
    </style>
    ''')

    state = get_state()

    def apply_start_stop_styles():
        s = get_state()

        if s['start_state'] == 1:
            start_button.style(
                'background-color: #28a745 !important; color: white !important; '
                'border: 2px solid black !important; border-radius: 10px; '
                'font-weight: bold; font-size: 18px; width: 140px; height: 70px; '
                'box-shadow: 0 0 12px 3px rgba(40,167,69,0.85); opacity: 1;'
            )
        else:
            start_button.style(
                'background-color: #1f7a33 !important; color: white !important; '
                'border: 2px solid black !important; border-radius: 10px; '
                'font-weight: bold; font-size: 18px; width: 140px; height: 70px; '
                'box-shadow: none; opacity: 0.65;'
            )

        if s['stop_state'] == 1:
            stop_button.style(
                'background-color: #d9534f !important; color: white !important; '
                'border: 2px solid black !important; border-radius: 10px; '
                'font-weight: bold; font-size: 18px; width: 140px; height: 70px; '
                'box-shadow: 0 0 12px 3px rgba(217,83,79,0.85); opacity: 1;'
            )
        else:
            stop_button.style(
                'background-color: #9f3b38 !important; color: white !important; '
                'border: 2px solid black !important; border-radius: 10px; '
                'font-weight: bold; font-size: 18px; width: 140px; height: 70px; '
                'box-shadow: none; opacity: 0.65;'
            )

    with ui.dialog() as password_dialog, ui.card():
        ui.label('Acesso DEV').style('font-size: 18px; font-weight: bold;')
        ui.label('Introduza a password DEV:')
        password_input = ui.input(password=True).props('outlined autofocus').on('keydown.enter', lambda e: confirm_password())

        def confirm_password():
            if password_input.value == PASSWORD:
                get_state()['devmode'] = 1
                print('Modo DEV ativado')
                password_dialog.close()
                apply_visual_state()
            else:
                print('Password DEV incorreta')
                ui.notify('Password DEV incorreta', color='negative')

        with ui.row().classes('w-full justify-end'):
            ui.button('Entrar', on_click=confirm_password)

    with ui.dialog() as mode_dialog, ui.card().style('padding: 20px; width: auto; max-width: 95vw;'):
        ui.label('Selecionar Modo').style('font-size: 18px; font-weight: bold; margin-bottom: 8px;')

        mode_buttons = {}

        with ui.row().classes('no-wrap').style('gap: 10px; width: max-content;'):

            def select_mode(mode_name):
                get_state()['mode_state'] = mode_name
                print('Modo selecionado:', mode_name)
                mode_dialog.close()

            with ui.column().classes('items-center'):
                mode_buttons['Estatico'] = ui.button(
                    'Estático',
                    on_click=lambda: select_mode('Estatico'),
                    color=None,
                ).classes('mode-btn-default').style('width: 110px; height: 80px; font-size: 14px;')

            with ui.column().classes('items-center'):
                mode_buttons['Respiracao'] = ui.button(
                    'Respiração',
                    on_click=lambda: select_mode('Respiracao'),
                    color=None,
                ).classes('mode-btn-default').style('width: 110px; height: 80px; font-size: 14px;')

            with ui.column().classes('items-center'):
                mode_buttons['Coracao'] = ui.button(
                    'Coração',
                    on_click=lambda: select_mode('Coracao'),
                    color=None,
                ).classes('mode-btn-default').style('width: 110px; height: 80px; font-size: 14px;')

            with ui.column().classes('items-center'):
                mode_buttons['Humano'] = ui.button(
                    'Humano',
                    on_click=lambda: select_mode('Humano'),
                    color=None,
                ).classes('mode-btn-default').style('width: 110px; height: 80px; font-size: 14px;')
                ui.label('(Resp+Bati)').style('font-size: 11px; color: #555;')

            with ui.column().classes('items-center'):
                mode_buttons['Completo'] = ui.button(
                    'Completo',
                    on_click=lambda: select_mode('Completo'),
                    color=None,
                ).classes('mode-btn-default').style('width: 110px; height: 80px; font-size: 14px;')
                ui.label('(Resp+Bati+Tosse)').style('font-size: 11px; color: #555;')

        def apply_mode_dialog_styles():
            current = get_state()['mode_state']
            for name, btn in mode_buttons.items():
                btn.classes(remove='mode-btn-default mode-btn-selected')
                if name == current:
                    btn.classes(add='mode-btn-selected')
                else:
                    btn.classes(add='mode-btn-default')

    with ui.row().classes('w-full no-wrap').style(
        'height: 100vh; padding: 12px; box-sizing: border-box; gap: 12px;'
    ):
        with ui.column().style('flex: 3; height: 100%; justify-content: space-between;'):
            with ui.column().classes('items-start'):

                def toggle_mode():
                    if get_state()['devmode'] == 0:
                        password_input.value = ''
                        password_dialog.open()
                    else:
                        get_state()['devmode'] = 0
                        print('Modo CLIENTE ativado')
                        apply_visual_state()

                mode_button = ui.button('Modo CLIENTE', on_click=toggle_mode, color=None)

            with ui.row().classes('w-full justify-center items-start').style('gap: 16px; margin-top: 24px;'):

                def on_start_click():
                    get_state()['start_state'] = 1
                    get_state()['stop_state'] = 0
                    print('Start pressionado — start_state =', get_state()['start_state'])
                    apply_visual_state()

                def on_mode_click():
                    apply_mode_dialog_styles()
                    mode_dialog.open()

                def on_stop_click():
                    get_state()['stop_state'] = 1
                    get_state()['start_state'] = 0
                    print('Stop pressionado — stop_state =', get_state()['stop_state'])
                    apply_visual_state()

                start_button = ui.button('Start', on_click=on_start_click, color=None).style(
                    'background-color: #28a745 !important; color: white !important; '
                    'border: 2px solid black !important; border-radius: 10px; '
                    'font-weight: bold; font-size: 18px; width: 140px; height: 70px;'
                )

                serial_mode_button = ui.button('Mode', on_click=on_mode_click, color=None).style(
                    'background-color: #337ab7 !important; color: white !important; '
                    'border: 2px solid black !important; border-radius: 10px; '
                    'font-weight: bold; font-size: 18px; width: 140px; height: 70px;'
                )

                stop_button = ui.button('Stop', on_click=on_stop_click, color=None).style(
                    'background-color: #d9534f !important; color: white !important; '
                    'border: 2px solid black !important; border-radius: 10px; '
                    'font-weight: bold; font-size: 18px; width: 140px; height: 70px;'
                )

            with ui.row().classes('items-start') as exit_row:
                ui.button(
                    'Desligar',
                    on_click=close_native_window,
                    color=None
                ).style(
                    'background-color: #d9534f !important; color: white !important; '
                    'border: 2px solid black !important; border-radius: 6px; '
                    'font-weight: bold; width: 120px; height: 40px;'
                )

                sair_button = ui.button(
                    'Sair',
                    on_click=shutdown_app,
                    color=None
                ).style(
                    'background-color: black !important; color: white !important; '
                    'border: 2px solid black !important; border-radius: 6px; '
                    'font-weight: bold; width: 120px; height: 40px; '
                    'box-shadow: 0 0 0 2px white, 0 0 0 4px black;'
                )
                sair_button.set_visibility(False)

        with ui.column().style('flex: 2; height: 100%;') as serial_container:
            with ui.card().style(
                'width: 100%; min-width: 340px; background-color: #f7f7f7; '
                'border: 1px solid #cfcfcf; border-radius: 8px; '
                'padding: 10px; box-shadow: none;'
            ):
                ui.label('Monitor Serial').style(
                    'font-size: 16px; font-weight: bold; '
                    'width: 100%; text-align: center;'
                )

                serial_monitor = ui.textarea(
                    value='\n'.join(get_state()['serialhistory'])
                ).props('readonly outlined').classes('w-full').style('min-height: 420px;')

                with ui.row().classes('w-full items-center no-wrap'):
                    serial_input = ui.input(
                        placeholder='Escrever comando para enviar pela serial...'
                    ).props('outlined').classes('grow').on('keydown.enter', lambda e: send_serial_command())

                    def send_serial_command():
                        command = serial_input.value.strip()
                        if not command:
                            return
                        get_state()['serialhistory'].append(f'PI4B:  {command}')
                        app.storage.general['serialhistory'] = get_state()['serialhistory']
                        serial_input.value = ''
                        printpi(command)
                        apply_visual_state()

                    ui.button('Enviar', on_click=send_serial_command).style('min-width: 90px;')

    with ui.column().style(
        'position: fixed; bottom: 16px; right: 16px; align-items: flex-end; z-index: 1000;'
    ):
        with ui.card().classes('settings-panel').style(
            'padding: 8px; padding-bottom: 64px; margin-bottom: -52px; '
            'background-color: #f0f0f0; border: 1px solid #cfcfcf; '
            'border-radius: 8px; min-width: 180px; z-index: 1;'
        ) as settings_panel:

            with ui.column().classes('w-full items-center gap-1') as qr_section:
                app_url = get_app_url()
                qr_data_uri = generate_qr_code_base64(app_url)
                if qr_data_uri:
                    ui.image(qr_data_uri).style(
                        'width: 140px; height: 140px; '
                        'border: 1px solid #cfcfcf; border-radius: 6px;'
                    )
                else:
                    ui.label('QR code indisponível').style('font-size: 11px; color: #888;')
                    ui.label('(instalar: pip install qrcode[pil])').style('font-size: 10px; color: #888;')
                ui.label(app_url).style('font-size: 11px; color: #555; word-break: break-all;')
                qr_separator = ui.separator().classes('w-full')

            with ui.column().classes('w-full gap-2') as settings_menu:
                com_button = ui.button('Porta COM', icon='usb').classes('w-full')
                rede_button = ui.button('Rede', icon='wifi').classes('w-full')
                config_button = ui.button('Configurações', icon='settings').classes('w-full')

            with ui.column().classes('w-full gap-2') as com_panel:
                ui.label('Portas COM disponíveis').style('font-weight: bold; font-size: 13px;')
                com_list_container = ui.column().classes('w-full gap-1')

            with ui.column().classes('w-full gap-2') as rede_panel:
                ui.label('Estado da rede').style('font-weight: bold; font-size: 13px;')
                rede_status_container = ui.column().classes('w-full gap-1')
                ui.label('Redes disponíveis').style('font-weight: bold; font-size: 13px; margin-top: 4px;')
                rede_list_container = ui.column().classes('w-full gap-1')

        async def refresh_com_panel():
            com_list_container.clear()
            with com_list_container:
                ui.label('A procurar...').style('font-size: 12px; color: #888;')
            lines = await run.io_bound(list_com_ports)
            com_list_container.clear()
            with com_list_container:
                for line in lines:
                    ui.label(line).style('font-size: 12px;')

        async def refresh_rede_panel():
            rede_status_container.clear()
            rede_list_container.clear()
            with rede_status_container:
                ui.label('A verificar...').style('font-size: 12px; color: #888;')

            status_lines = await run.io_bound(get_network_status)
            rede_status_container.clear()
            with rede_status_container:
                for line in status_lines:
                    ui.label(line).style('font-size: 12px;')

            with rede_list_container:
                ui.label('A procurar redes...').style('font-size: 12px; color: #888;')

            network_lines = await run.io_bound(get_available_networks)
            rede_list_container.clear()
            with rede_list_container:
                for line in network_lines:
                    ui.label(line).style('font-size: 12px;')

        last_applied_view = {'value': None}

        def show_com_panel():
            get_state()['settings_view'] = 'com'
            apply_settings_view()

        def show_rede_panel():
            get_state()['settings_view'] = 'rede'
            apply_settings_view()

        com_button.on_click(show_com_panel)
        rede_button.on_click(show_rede_panel)

        def toggle_settings():
            current = get_state()['settings_view']
            if current == 'closed':
                get_state()['settings_view'] = 'menu'
            elif current in ('com', 'rede'):
                get_state()['settings_view'] = 'menu'
            else:
                get_state()['settings_view'] = 'closed'
            apply_settings_view()

        def apply_settings_view():
            view = get_state()['settings_view']
            settings_panel.set_visibility(view != 'closed')
            qr_section.set_visibility(view == 'menu')
            qr_separator.set_visibility(view == 'menu')
            settings_menu.set_visibility(view == 'menu')
            com_panel.set_visibility(view == 'com')
            rede_panel.set_visibility(view == 'rede')

            if view != last_applied_view['value']:
                last_applied_view['value'] = view
                if view == 'com':
                    asyncio.create_task(refresh_com_panel())
                elif view == 'rede':
                    asyncio.create_task(refresh_rede_panel())

        ui.button('Definições', icon='tune', on_click=toggle_settings).style(
            'background-color: #6c757d; color: white; '
            'border-radius: 6px; font-weight: bold; '
            'width: 150px; height: 40px; position: relative; z-index: 2;'
        )

    def apply_visual_state():
        s = get_state()
        if s['devmode'] == 1:
            mode_button.text = 'Modo DEV'
            mode_button.style(
                'background-color: #ffd84d; color: black; '
                'border: 2px solid #d9534f; border-radius: 14px; '
                'font-weight: bold; width: 120px; height: 60px;'
            )
            serial_container.set_visibility(True)
        else:
            mode_button.text = 'Modo CLIENTE'
            mode_button.style(
                'background-color: #337ab7; color: white; '
                'border: 2px solid #8ec5ff; border-radius: 14px; '
                'font-weight: bold; width: 120px; height: 60px;'
            )
            serial_container.set_visibility(False)

        sair_button.set_visibility(s['devmode'] == 1)
        serial_monitor.value = '\n'.join(get_state()['serialhistory'])

        apply_settings_view()
        apply_mode_dialog_styles()
        apply_start_stop_styles()

    ui.timer(0.5, apply_visual_state)
    apply_visual_state()


if __name__ == '__main__':
    reset_state()
    ui.run(
        native=True,
        reload=False,
        host='0.0.0.0',
        port=8081,
        title='PI_4B Control Panel',
        storage_secret='pi4b-secret-key',
    )
