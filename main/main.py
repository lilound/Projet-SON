import serial
import time
from interface import Window

arduino = serial.Serial(port='COM3', baudrate=9600, timeout=.1)

def read():
    data = arduino.readline()
    time.sleep(0.05)
    return data

int = Window()
int.mainloop()

while True:
    value = read()
    print("Réponse de l'Arduino :", value.decode('utf-8'))
