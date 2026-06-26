# AirWAY Workspace Setup

Este ficheiro descreve como preparar um PC/Raspberry/VS Code novo para correr este repositorio.

## 1. Extensoes VS Code

Instalar as extensoes recomendadas pelo workspace:

- `ms-python.python`
- `ms-python.vscode-pylance`
- `ms-python.debugpy`
- `platformio.platformio-ide`
- `openai.chatgpt` opcional, para apoio no VS Code

O ficheiro `.vscode/extensions.json` ja contem estas recomendacoes.

## 2. Abrir o projeto

Abrir sempre a raiz do repositorio no VS Code:

```bash
cd /home/airway/Documents/Projeto_Airway/Delta
code .
```

No Windows, abrir a pasta raiz equivalente do clone do repositorio.

## 3. Identificar dispositivo e sistema operativo

Antes de instalar dependencias, identificar onde o projeto esta a correr. Isto muda comandos, paths, permissao de portas serie e configuracao do Code Runner.

Linux/Raspberry:

```bash
uname -a
uname -m
cat /etc/os-release
hostnamectl
python3 --version
```

Detetar se parece Raspberry Pi:

```bash
cat /proc/device-tree/model 2>/dev/null || cat /sys/firmware/devicetree/base/model 2>/dev/null || echo "Modelo nao encontrado"
```

Ver CPU/arquitetura:

```bash
lscpu
```

Windows PowerShell:

```powershell
systeminfo
$PSVersionTable
py --version
wmic os get Caption,Version,OSArchitecture
wmic computersystem get Manufacturer,Model
```

macOS:

```bash
sw_vers
uname -m
python3 --version
```

Interpretacao rapida:

- Raspberry Pi OS / Debian / Ubuntu em ARM: usar comandos Linux, `.venv/bin/python`, grupo `dialout`, portas `/dev/ttyUSB*` ou `/dev/ttyACM*`.
- Linux PC x86_64: usar comandos Linux, `.venv/bin/python`, pode tambem precisar de `dialout` para serial USB.
- Windows: usar `.venv\Scripts\python.exe`, portas `COM3`, `COM4`, etc., sem grupo `dialout`.
- WSL: cuidado, USB/serial pode nao aparecer automaticamente dentro do WSL; para ESP32 e PlatformIO pode ser melhor usar VS Code nativo no Windows ou configurar passagem USB.
- macOS: usar `.venv/bin/python`, portas `/dev/cu.*` ou `/dev/tty.*`.

Comandos para portas serie por sistema:

Linux/Raspberry:

```bash
ls /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/*
.venv/bin/python -m serial.tools.list_ports -v
```

Windows PowerShell:

```powershell
py -m serial.tools.list_ports -v
```

macOS:

```bash
ls /dev/cu.* /dev/tty.*
.venv/bin/python -m serial.tools.list_ports -v
```

## 4. Criar o ambiente Python `.venv`

O projeto deve usar um ambiente Python privado chamado `.venv`.

Linux/Raspberry:

```bash
cd /home/airway/Documents/Projeto_Airway/Delta
python3 -m venv .venv
.venv/bin/python -m pip install --upgrade pip
.venv/bin/python -m pip install -r Python_CODE/requirements.txt
```

Windows PowerShell:

```powershell
cd caminho\para\Delta
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r Python_CODE\requirements.txt
```

Bibliotecas Python usadas:

- `flask`
- `flask-socketio`
- `pyserial`
- `qrcode[pil]`
- `numpy`
- `matplotlib`
- `nicegui`

## 5. Selecionar interpretador no VS Code

No VS Code:

1. `Ctrl+Shift+P`
2. `Python: Select Interpreter`
3. Escolher o interpretador dentro de `.venv`

Linux/Raspberry:

```text
.venv/bin/python
```

Windows:

```text
.venv\Scripts\python.exe
```

## 6. Configuracoes VS Code importantes

O ficheiro `.vscode/settings.json` configura:

- `python.analysis.extraPaths` para o Pylance encontrar os modulos locais.
- `python.terminal.activateEnvironment` para ativar o `.venv` no terminal.
- `code-runner.executorMap` para o Code Runner usar `.venv/bin/python` em Linux.
- `LANG=C.UTF-8` e `LC_ALL=C.UTF-8` para evitar avisos de locale no Raspberry.

Nota: em Windows, se for usado Code Runner, o executor pode precisar de ser adaptado para:

```json
"code-runner.executorMap": {
  "python": "cd $workspaceRoot && .venv\\Scripts\\python.exe -u $fullFileName"
}
```

## 7. Correr a interface Raspberry Pi

Comando recomendado em Linux/Raspberry:

```bash
cd /home/airway/Documents/Projeto_Airway/Delta
.venv/bin/python "Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI.py"
```

Forcar porta serie, se necessario:

```bash
PI4B_SERIAL_PORT=/dev/ttyUSB0 .venv/bin/python "Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI.py"
```

Desativar kiosk temporariamente:

```bash
PI4B_KIOSK=0 .venv/bin/python "Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI.py"
```

Nao abrir browser automaticamente:

```bash
PI4B_OPEN_BROWSER=0 .venv/bin/python "Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI.py"
```

## 8. Porta serie ESP32 no Raspberry

Listar portas serie:

```bash
ls /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/*
```

Listar com pyserial:

```bash
.venv/bin/python -m serial.tools.list_ports -v
```

Ver eventos USB ao ligar/desligar ESP32:

```bash
dmesg -w
```

Se aparecer erro de permissao em `/dev/ttyUSB0` ou `/dev/ttyACM0`:

```bash
sudo usermod -a -G dialout $USER
```

Depois terminar sessao ou reiniciar o Raspberry.

Se a porta nao aparecer:

- verificar cabo USB de dados
- testar outra porta USB
- confirmar que o ESP32 esta alimentado
- fechar o Serial Monitor do PlatformIO, porque so um programa pode usar a porta de cada vez

## 9. PlatformIO / ESP32

O ficheiro `platformio.ini` na raiz aponta para o projeto ESP32 dentro da pasta `PlatformIO/`.

Dependencias ESP32 definidas:

- `adafruit/Adafruit NeoPixel`
- `madhephaestus/ESP32Servo`
- `waspinator/AccelStepper`

No VS Code, com PlatformIO instalado:

1. Abrir a raiz do repositorio.
2. Esperar o PlatformIO carregar o ambiente `freenove_esp32_s3_wroom`.
3. Usar Build/Upload/Monitor pelo painel do PlatformIO.

## 10. Atalho do ambiente de trabalho no Raspberry

Existe o script:

```text
run_airway_gui.sh
```

Ele:

- entra na pasta do projeto
- usa `.venv/bin/python`
- configura locale UTF-8
- configura `MPLCONFIGDIR`
- arranca `RB_PI4B_Main_PI.py`

Para criar/recriar o atalho no Desktop:

```bash
cat > /home/airway/Desktop/AirWAY_GUI.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=AirWAY_GUI
Comment=Iniciar interface AirWAY PI4B
Exec=/home/airway/Documents/Projeto_Airway/Delta/run_airway_gui.sh
Path=/home/airway/Documents/Projeto_Airway/Delta
Terminal=false
Categories=Utility;
StartupNotify=false
EOF

chmod +x /home/airway/Desktop/AirWAY_GUI.desktop
gio set /home/airway/Desktop/AirWAY_GUI.desktop metadata::trusted true
```

Se o sistema mostrar aviso, escolher `Trust and Launch` / `Confiar e executar`.

## 11. Ficheiros importantes

- `Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI.py`
  - servidor Flask/SocketIO e controlo principal da interface Raspberry.

- `Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI_template_PC.html`
  - interface kiosk/desktop.

- `Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI_template_MB.html`
  - interface mobile/tablet.

- `Python_CODE/Interface de controlo ver_PI4B/calculo_movimentos.py`
  - calculo dos pontos e imagens dos graficos.

- `Python_CODE/Interface de controlo ver_PI4B/movements_config.json`
  - configuracoes persistentes dos movimentos.

- `Python_CODE/requirements.txt`
  - bibliotecas Python a instalar no `.venv`.

- `.vscode/settings.json`
  - paths, Pylance, Code Runner e locale.

- `pyrightconfig.json`
  - configuracao do Pylance/Pyright para encontrar `.venv` e modulos locais.

- `platformio.ini`
  - entrada PlatformIO do projeto ESP32.

## 12. Diagnostico rapido

Confirmar que o Python certo esta a ser usado:

```bash
.venv/bin/python -c "import sys; print(sys.executable)"
```

Confirmar bibliotecas principais:

```bash
.venv/bin/python - <<'PY'
import flask
import flask_socketio
import serial
import qrcode
import numpy
import matplotlib
print('OK')
PY
```

Testar rota de calculo sem abrir browser:

```bash
.venv/bin/python - <<'PY'
import sys
sys.path.insert(0, 'Python_CODE/Interface de controlo ver_PI4B')
import RB_PI4B_Main_PI as appmod
client = appmod.app.test_client()
resp = client.post('/movements/calcular', json={})
print(resp.status_code, resp.get_json().get('ok'))
PY
```

Compilar ficheiros Python:

```bash
.venv/bin/python -m py_compile \
  "Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI.py" \
  "Python_CODE/Interface de controlo ver_PI4B/calculo_movimentos.py"
```
