import serial
import time

def get_data(port='COM3', baudrate=9600):
    """
    Se connecte, attend START_DATA, capture tout jusqu'à END_DATA 
    et renvoie la liste complète.
    """
    try:
        arduino = serial.Serial(port=port, baudrate=baudrate, timeout=.1)
        time.sleep(2) 
        print("Connexion OK. En attente du signal...")
    except Exception as e:
        print(f"Erreur port : {e}")
        return []

    recording = False
    data_captured = []

    while True:
        line = arduino.readline().decode('utf-8').strip()
        
        if line == "START_DATA":
            recording = True
            data_captured = []
            print("Capture en cours...")
            
        elif line == "END_DATA":
            print("Capture terminée.")
            arduino.close() # On libère le port pour les futurs usages
            return data_captured # On sort de la fonction avec les données
            
        elif recording and line:
            data_captured.append(line)
