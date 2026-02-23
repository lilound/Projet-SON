#!/usr/bin/env python3
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from manage_data import get_data
from time import sleep

class Window(tk.Tk):

    def __init__(self):
        super().__init__()

        self.title("TeensHear - Appareil auditif et de prévention")
        self.state('zoomed')  # Pour Windows (met la fenêtre en plein écran)  

        # --- STYLE DES BOUTONS ---
        self.style = ttk.Style()
        self.style.theme_use('clam')
        self.style.configure('Rounded.TButton', font=('Segoe UI', 25), padding=(20, 20), anchor="center")
        self.create_widgets()

        # Matplotlib
        self.fig, (self.ax_gauche, self.ax_droite) = plt.subplots(1, 2, figsize=(10, 2))
        self.fig.subplots_adjust(left=0.1, right=0.95, top=0.9, bottom=0.2, wspace=0.3)
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)
        
        # Configuration esthétique rapide
        self.style_graph()
        
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    

    def style_graph(self):
        """ Reconfigure les deux graphiques en mode Audiogramme standard """
        for ax, title in zip([self.ax_gauche, self.ax_droite], ["Oreille Gauche", "Oreille Droite"]):
            ax.set_title(title, fontweight='bold')
            ax.set_xlabel("Fréquence (Hz)")
            ax.set_ylabel("Niveau (dB HL)")
            
            # 1. Échelle Logarithmique
            ax.set_xscale('log')
            
            
            # 3. Axe Y descendant (0 en haut, 100 en bas)
            ax.invert_yaxis()
            
            ax.grid(True, which="both", linestyle='--', alpha=0.5)



    def create_widgets(self):
        control_frame = ttk.LabelFrame(self)
        control_frame.pack(fill="x", padx=10, pady=20, ipady=30)

        self.btn_diag = ttk.Button(control_frame, text="Lancer le diagnostic", command=self.diagnostic, style='Rounded.TButton')
        self.btn_diag.pack(side="left", padx=10, pady=20, expand=True, fill="both")

        self.btn_correction = ttk.Button(control_frame, text="Démarrer la correction", command=self.correction, style='Rounded.TButton')
        self.btn_correction.pack(side="left", padx=10, pady=20, expand=True, fill="both")

        # Bouton spécifique au mode Simulation (caché au début)
        self.btn_run_sim = ttk.Button(control_frame, text="Lancer la simulation", command=self.simulation, style='Rounded.TButton')

        self.btn_mode = ttk.Button(control_frame, text="Passer en mode simulation", command=self.change_mode, style='Rounded.TButton')
        self.btn_mode.pack(side="left", padx=10, pady=20, expand=True, fill="both")

        self.correction_frame = ttk.Frame(self)
        self.correction_frame.pack(fill="both", expand=True) # Affiché par défaut

        self.graph_frame = ttk.LabelFrame(self.correction_frame)
        self.graph_frame.pack(fill="both", expand=True, padx=10, pady=5)

        tk.Label(self.graph_frame, text="AUDIOGRAMMES", font=("Segoe UI", 22)).pack(side="top", pady=(20, 0))


        self.simulation_frame = ttk.Frame(self) # On ne fait pas de .pack() ici pour qu'il soit caché au début

    

    def diagnostic(self):

        # on vide l'affichage des graphes
        self.ax_gauche.clear()
        self.ax_droite.clear()
        self.style_graph()
        self.canvas.draw()

        # Affichage du popup
        self.popup_test()

        # On récupère les données du teensy
        donnees = ['125.00,10.00,0.00', '250.00,0.00,0.00', '500.00,0.00,0.00', '1000.00,0.00,0.00', '2000.00,70.00,0.00', '4000.00,0.00,30.00', '8000.00,0.00,0.00']
        #get_data()
        
        if donnees:
            self.popup.destroy()

            freqs = []
            gauche = []
            droite = []

            for ligne in donnees:
                try:
                    # On sépare par la virgule (format '125.00,10.00,0.00')
                    f, g, d = map(float, ligne.split(','))
                    freqs.append(f)
                    gauche.append(g)
                    droite.append(d)
                except ValueError:
                    continue 

            # Affichage des chiffres des fréquences sur l'abscisses
            self.ax_gauche.set_xticks(freqs)
            self.ax_droite.set_xticks(freqs)
            self.ax_gauche.get_xaxis().set_major_formatter(plt.ScalarFormatter())
            self.ax_droite.get_xaxis().set_major_formatter(plt.ScalarFormatter())

            # Tracé Oreille Gauche
            self.ax_gauche.plot(freqs, gauche, 'b-x', label="Gauche")

            # Tracé Oreille Droite
            self.ax_droite.plot(freqs, droite, 'r-x', label="Droite")

            # Rafraîchissement du canvas
            self.fig.subplots_adjust(left=0.1, right=0.95, top=0.9, bottom=0.2, wspace=0.3)
            self.canvas.draw()

        else:
            messagebox.showwarning("Erreur", "Aucune donnée n'a pu être récupérée.")



    def popup_test(self):
        self.popup = tk.Toplevel(self)
        self.popup.title("Information")
        self.popup.geometry("800x300")
        self.popup.configure(bg="white")
        self.popup.transient(self)   # le popup appartient à la fenêtre principale
        self.popup.grab_set()        # empêche d'interagir avec la fenêtre principale
        self.popup.focus_force()     # force le focus sur le popup  

        x = self.winfo_x() + (self.winfo_width() // 2) - (800 // 2)
        y = self.winfo_y() + (self.winfo_height() // 2) - (300 // 2)
        
        self.popup.geometry(f"{800}x{300}+{x}+{y}")


        # Conteneur pour centrer les deux éléments
        info_frame = tk.Frame(self.popup, bg="white")
        info_frame.pack(expand=True)

        # Le symbole en bleu
        tk.Label(info_frame, text="ⓘ", font=("Segoe UI", 65, "bold"), fg="#2196F3", bg="white").grid(row=0, column=0, padx=10)

        # Le texte en noir
        tk.Label(info_frame, text="Test auditif en cours", font=("Segoe UI", 25, "bold"), bg="white").grid(row=0, column=1, pady=(14,0))
        
        # On force la mise à jour visuelle pour que le popup apparaisse bien
        self.update()



    def correction(self):
        pass


    def simulation(self):
        pass


    def change_mode(self):
        # Si la zone correction est visible, on passe en simulation
        if self.correction_frame.winfo_viewable():
            # 1. Cacher l'interface de diagnostic
            self.correction_frame.pack_forget()
            self.btn_diag.pack_forget()
            self.btn_correction.pack_forget()
            
            # 2. Afficher l'interface de simulation
            self.simulation_frame.pack(fill="both", expand=True)
            # On place le bouton 'Lancer simulation' à gauche du bouton de mode
            self.btn_run_sim.pack(side="left", padx=10, pady=20, expand=True, fill="both", before=self.btn_mode)
            
            self.btn_mode.config(text="Passer en mode correction")
        else:
            # 1. Cacher l'interface de simulation
            self.simulation_frame.pack_forget()
            self.btn_run_sim.pack_forget()
            
            # 2. Réafficher l'interface de diagnostic
            self.correction_frame.pack(fill="both", expand=True)
            
            # On réaffiche les boutons
            self.btn_correction.pack(side="left", padx=10, pady=20, expand=True, fill="both", before=self.btn_mode)
            self.btn_diag.pack(side="left", padx=10, pady=20, expand=True, fill="both", before=self.btn_correction)
            
            self.btn_mode.config(text="Passer en mode simulation")   




    def on_close(self): 
        self.quit()
        self.destroy()



if __name__ == "__main__":
    app = Window()
    app.mainloop()