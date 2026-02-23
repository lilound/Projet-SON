import serial
import time

# Remplace 'COM3' par ton port (ex: '/dev/ttyACM0' sur Linux/Mac)
# Le baudrate doit être IDENTIQUE à celui de l'Arduino (9600)
arduino = serial.Serial(port='COM3', baudrate=9600, timeout=.1)

def write_read(x):
    arduino.write(bytes(x, 'utf-8'))
    time.sleep(0.05)
    data = arduino.readline()
    return data

while True:
    num = input("Tape un message pour l'Arduino : ")
    value = write_read(num)
    print("Réponse de l'Arduino :", value.decode('utf-8'))
