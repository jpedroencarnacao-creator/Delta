"""
from nicegui import ui

ui.label('Hello NiceGUI')
ui.run(native=True, reload=False, host='0.0.0.0', port=8081)
"""
from nicegui import ui, app

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
    return s


def reset_state():
    """Limpa o estado guardado em disco — chamado uma vez a cada arranque do programa."""
    app.storage.general['devmode'] = 0
    app.storage.general['serialhistory'] = [
        'Zona reservada para mensagens enviadas e recebidas pela serial.',
        'A comunicação será ligada mais tarde.',
    ]


def printpi(args):
    print('PI4B:', args)

def close_native_window():
    if getattr(app, 'native', None) and app.native.main_window:
        app.native.main_window.destroy()

def shutdown_app():
    app.shutdown()


# ---------------------------------------------------------------------------
# Página — executada para CADA cliente que se liga
# Cada cliente cria os seus próprios widgets mas lê/escreve no mesmo estado.
# ---------------------------------------------------------------------------
@ui.page('/')
def main_page():
    ui.add_head_html('''<style> body, .nicegui-content {background-color: white;}</style>''')
    state = get_state()

    # --- diálogo de password ------------------------------------------------
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

    # --- layout principal ---------------------------------------------------
    with ui.row().classes('w-full no-wrap').style(
        'height: 100vh; padding: 12px; box-sizing: border-box; gap: 12px;'
    ):
        # coluna esquerda
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

        # coluna direita — monitor serial
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
                    serial_input = ui.input(placeholder='Escrever comando para enviar pela serial...').props('outlined').classes('grow').on('keydown.enter', lambda e: send_serial_command())

                    def send_serial_command():
                        command = serial_input.value.strip()
                        if not command:
                            return
                        get_state()['serialhistory'].append(f'PI4B:  {command}')
                        # forçar persistência da lista (mutação in-place não notifica)
                        app.storage.general['serialhistory'] = get_state()['serialhistory']
                        serial_input.value = ''
                        printpi(command)
                        apply_visual_state()

                    ui.button('Enviar', on_click=send_serial_command).style('min-width: 90px;')

    # --- função que aplica o estado visual a ESTE cliente -------------------
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

    # --- timer POR CLIENTE: faz polling ao estado global e atualiza o ecrã -
    # Intervalo de 0.5 s → reação rápida sem sobrecarregar
    ui.timer(0.5, apply_visual_state)

    # estado inicial correto ao carregar a página
    apply_visual_state()

"""
# ---------------------------------------------------------------------------
# Reset do estado a cada arranque do programa (não persiste entre execuções)
# ---------------------------------------------------------------------------
reset_state()
"""
# ---------------------------------------------------------------------------
# Arranque — native=True abre a janela; o browser acede em localhost:8081
# ---------------------------------------------------------------------------
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