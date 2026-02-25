#!/usr/bin/env python3


def get_data_from_teensy(arduino):
    """
    Se connecte, attend START_DATA, capture tout jusqu'à END_DATA 
    et renvoie la liste complète.
    """
    
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
            return data_captured # On sort de la fonction avec les données
            
        elif line == "ABORT_DIAG":
            return "STOPPED"
            
        elif recording and line:
            data_captured.append(line)

        


def send_data_to_teensy(arduino, data):
    print(data)
    arduino.write(bytes(data, 'utf-8'))
