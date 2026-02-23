#!/usr/bin/env python3
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

class Window(tk.Tk):

    def __init__(self):
        super().__init__()

        self.title("TeensHear - Appareil auditif et de prévention")
        self.state('zoomed')  

        # --- STYLE MODERNE ET ARRONDI ---
        self.configure(bg='#f5f6f7') # Fond gris très clair
        self.style = ttk.Style()
        self.style.theme_use('clam')
        
        # Configuration des boutons "Capsule"
        # padding=(horizontal, vertical) -> 45 en vertical pour une grande hauteur
        self.style.configure('Rounded.TButton', 
                             font=('Segoe UI', 18, 'bold'), 
                             padding=(20, 45), 
                             background='#2c3e50', 
                             foreground='white',
                             borderwidth=0,
                             focuscolor='none')
        
        # Effet au survol
        self.style.map('Rounded.TButton', 
                       background=[('active', '#34495e'), ('pressed', '#1a252f')])

        self.create_widgets()

        # Initialisation Matplotlib
        self.fig, (self.ax_gauche, self.ax_droite) = plt.subplots(1, 2, figsize=(10, 4))
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)
        
        self.style_graph()
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def style_graph(self):
        """ Configuration épurée des graphiques """
        for ax, title in zip([self.ax_gauche, self.ax_droite], ["OREILLE GAUCHE", "OREILLE DROITE"]):
            ax.clear()
            ax.set_title(title, fontweight='bold', fontsize=12, pad=15, color='#2c3e50')
            ax.set_xlabel("Fréquence (Hz)", color='#7f8c8d')
            ax.set_ylabel("Niveau (dB HL)", color='#7f8c8d')
            
            ax.set_xscale('log')
            ax.invert_yaxis()
            ax.set_ylim(100, -10)
            
            ax.grid(True, which="both", linestyle='--', alpha=0.3)
            freq_ticks = [125, 250, 500, 1000, 2000, 4000, 8000]
            ax.set_xticks(freq_ticks)
            ax.get_xaxis().set_major_formatter(plt.ScalarFormatter())
            
        self.fig.subplots_adjust(left=0.1, right=0.95, top=0.85, bottom=0.2, wspace=0.3)

    def create_widgets(self):
        # Cadre de contrôle
        control_frame = tk.Frame(self, bg='#f5f6f7')
        control_frame.pack(fill="x", padx=20, pady=30)

        # Style commun pour les boutons (Simule l'arrondi via le relief)
        btn_config = {
            "font": ('Segoe UI', 18, 'bold'),
            "bg": '#2c3e50',
            "fg": 'white',
            "activebackground": '#34495e',
            "activeforeground": 'white',
            "bd": 0,
            "cursor": "hand2",
            "pady": 20 # Hauteur interne
        }

        # Création des boutons tk.Button (plus flexibles pour le look plat/arrondi)
        self.btn_diag = tk.Button(control_frame, text="Lancer le diagnostic", 
                                  command=self.diagnostic, **btn_config)
        self.btn_diag.pack(side="left", padx=15, expand=True, fill="both")

        self.btn_correction = tk.Button(control_frame, text="Démarrer la correction", 
                                        command=self.correction, **btn_config)
        self.btn_correction.pack(side="left", padx=15, expand=True, fill="both")

        # Bouton simulation (caché)
        self.btn_run_sim = tk.Button(control_frame, text="Lancer la simulation", 
                                     command=self.simulation, **btn_config)

        self.btn_mode = tk.Button(control_frame, text="Passer en mode simulation", 
                                  command=self.change_mode, **btn_config)
        self.btn_mode.pack(side="left", padx=15, expand=True, fill="both")

        # --- RESTE DU CODE (Frames) ---
        self.correction_frame = tk.Frame(self, bg='#f5f6f7')
        self.correction_frame.pack(fill="both", expand=True)

        self.graph_frame = tk.Frame(self.correction_frame, bg='white', 
                                    highlightbackground="#dddddd", highlightthickness=1)
        self.graph_frame.pack(fill="both", expand=True, padx=30, pady=10)

        self.simulation_frame = tk.Frame(self, bg='#f5f6f7')

    def diagnostic(self):
        # Clear visuel
        self.style_graph()
        self.canvas.draw()
        self.update() 

        # Affichage Popup
        self.popup_test()

        # Données fictives (Simulation)
        donnees = ['125.00,10.00,0.00', '250.00,0.00,0.00', '500.00,0.00,0.00', '1000.00,0.00,0.00', '2000.00,70.00,0.00', '4000.00,0.00,30.00', '8000.00,0.00,0.00']
        
        if donnees:
            freqs, gauche, droite = [], [], []
            for ligne in donnees:
                f, g, d = map(float, ligne.split(','))
                freqs.append(f); gauche.append(g); droite.append(d)

            # Tracé
            self.ax_gauche.plot(freqs, gauche, color='#3498db', marker='x', markersize=8, linewidth=2)
            self.ax_droite.plot(freqs, droite, color='#e74c3c', marker='o', markersize=8, linewidth=2)
            
            self.canvas.draw()
            self.popup.destroy() 
        else:
            self.popup.destroy()
            messagebox.showwarning("Erreur", "Aucune donnée reçue.")

    def popup_test(self):
        self.popup = tk.Toplevel(self)
        self.popup.title("Information")
        self.popup.configure(bg="white")
        self.popup.withdraw() 

        # Centrage dynamique
        self.update_idletasks()
        w, h = 800, 300
        x = self.winfo_x() + (self.winfo_width() // 2) - (w // 2)
        y = self.winfo_y() + (self.winfo_height() // 2) - (h // 2)
        self.popup.geometry(f"{w}x{h}+{x}+{y}")
        
        self.popup.deiconify()
        self.popup.transient(self)
        self.popup.grab_set()

        info_frame = tk.Frame(self.popup, bg="white")
        info_frame.pack(expand=True)

        # Icône ⓘ et Texte alignés
        tk.Label(info_frame, text="ⓘ", font=("Segoe UI", 65), fg="#2196F3", bg="white").grid(row=0, column=0, padx=10, sticky="ns")
        tk.Label(info_frame, text="Test auditif en cours", font=("Segoe UI", 25, "bold"), bg="white").grid(row=0, column=1, pady=(15,0), sticky="ns")
        
        self.update()

    def change_mode(self):
        if self.correction_frame.winfo_viewable():
            self.correction_frame.pack_forget()
            self.btn_diag.pack_forget()
            self.btn_correction.pack_forget()
            self.simulation_frame.pack(fill="both", expand=True)
            self.btn_run_sim.pack(side="left", padx=15, expand=True, fill="both", before=self.btn_mode)
            self.btn_mode.config(text="Passer en mode diagnostic")
        else:
            self.simulation_frame.pack_forget()
            self.btn_run_sim.pack_forget()
            self.correction_frame.pack(fill="both", expand=True)
            self.btn_correction.pack(side="left", padx=15, expand=True, fill="both", before=self.btn_mode)
            self.btn_diag.pack(side="left", padx=15, expand=True, fill="both", before=self.btn_correction)
            self.btn_mode.config(text="Passer en mode simulation")

    def correction(self): pass
    def simulation(self): pass
    def on_close(self): self.destroy()

if __name__ == "__main__":
    app = Window()
    app.mainloop()