"""
import serial
import time

serialcomm = serial.Serial('COM7', 115200)  # usa o baud rate que tens no Arduino
serialcomm.timeout = 1

while True:
    i = input("Introduza um comando: ").strip()
    if i == 'done':
        print('finished program')
        break

    if not i:
        continue

    # envia só o primeiro carácter como comando
    serialcomm.write(i[0].encode())  

    time.sleep(0.05)  # pequeno atraso

    # ler o que o Arduino escreveu (opcional)
    resp = serialcomm.readline().decode('ascii', errors='ignore')
    if resp:
        print("Arduino:", resp)

serialcomm.close()

"""
"""
import serial
import time

serialcomm = serial.Serial('COM7', 115200, timeout=1)

while True:
    data = serialcomm.readline().decode('ascii', errors='ignore')
    data = data.strip()            # tira \r, \n, espaços

    if data:                       # só entra se não for string vazia
        print("Esp32:", data)

    time.sleep(0.2)
    
"""
# Source - https://stackoverflow.com/a/68914329
# Posted by ItayMiz, modified by community. See post 'Timeline' for change history
# Retrieved 2026-06-07, License - CC BY-SA 4.0
"""
import time
import serial
counter = 0

ser = serial.Serial()
ser.port = 'COM7'
ser.baudrate = 115200
ser.setDTR(False)
ser.setRTS(False)

ser.open()
while True:
    counter = counter+1
    Data_bus = ser.readline().decode('ascii', errors='ignore')
    Data_bus = Data_bus.strip() 
    print(Data_bus)
    time.sleep(0.005)
    if(counter==1):
        print("insira um comando")
    
    command = input().strip()
    
    if command == 'done':
        print('finished program')
        break
    ser.write(command.encode('utf-8'))
"""
import serial
import threading
import time

def print_pi(*args):
    print("PI_4B: ", *args)

PORT = 'COM7'
BAUD = 115200

ser = serial.Serial()
ser.port = PORT
ser.baudrate = BAUD
ser.timeout = 0.1

ser.dtr = False
ser.rts = True

ser.open()
time.sleep(0.1)
ser.rts = False
time.sleep(0.1)

print_pi("Ligado. ESP32 deve ter feito reset agora.")

def reader():
    while True:
        data = ser.readline().decode('ascii', errors='ignore').strip()
        if data:
            print("\rEsp32:", data)
            print("> ", end="", flush=True)

def writer():
    while True:
        cmd = input("> ").strip()
        if cmd == "done":
            print_pi("finished program")
            ser.close()
            break
        if cmd:
            ser.write(cmd.encode('utf-8'))
            print_pi(cmd)

t_read = threading.Thread(target=reader, daemon=True)
t_read.start()

writer()