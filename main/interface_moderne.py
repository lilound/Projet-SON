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
        self.style.configure('Rounded.TButton', font=('Segoe UI', 18, 'bold'), padding=(20, 45), background='#2c3e50', foreground='white',borderwidth=0,focuscolor='none')
        
        # Effet au survol
        self.style.map('Rounded.TButton', background=[('active', '#34495e'), ('pressed', '#1a252f')])

        self.create_widgets()

        # Initialisation Matplotlib
        self.fig, (self.ax_gauche, self.ax_droite) = plt.subplots(1, 2, figsize=(10, 5))
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)
        
        self.style_graph([self.ax_gauche, self.ax_droite], ["OREILLE GAUCHE", "OREILLE DROITE"], self.fig)
        self.protocol("WM_DELETE_WINDOW", self.on_close)



    def create_widgets(self):
        # Cadre de contrôle
        control_frame = tk.Frame(self, bg='#f5f6f7')
        control_frame.pack(fill="x", padx=20)

        self.mode = tk.Label(control_frame, text="MODE CORRECTION", font=("Segoe UI", 45), bg='#f5f6f7')
        self.mode.pack(pady=(40,35))

        btn_config = {"font": ('Segoe UI', 18, 'bold'), "bg": '#2c3e50', "fg": 'white', "activebackground": '#34495e', "activeforeground": 'white', "bd": 0, "cursor": "hand2", "pady": 20}

        self.btn_diag = tk.Button(control_frame, text="Lancer le diagnostic", command=self.diagnostic, **btn_config)
        self.btn_diag.pack(side="left", padx=15, expand=True, fill="both")

        self.btn_correction = tk.Button(control_frame, text="Démarrer la correction", command=self.correction, **btn_config)
        self.btn_correction.pack(side="left", padx=15, expand=True, fill="both")

        # Bouton simulation (caché)
        self.btn_run_sim = tk.Button(control_frame, text="Lancer la simulation", command=self.simulation, **btn_config)

        self.btn_mode = tk.Button(control_frame, text="Passer en mode simulation", command=self.change_mode, **btn_config)
        self.btn_mode.pack(side="left", padx=15, expand=True, fill="both")

        # Affichage en mode correction
        self.correction_frame = tk.Frame(self, bg='#f5f6f7')
        self.correction_frame.pack(fill="both", expand=True)

        self.graph_frame = tk.Frame(self.correction_frame, bg='white', highlightbackground="#dddddd", highlightthickness=1)
        self.graph_frame.pack(fill="both", expand=True, padx=30, pady=10)
        tk.Label(self.graph_frame, text="AUDIOGRAMMES", font=("Segoe UI", 30, "bold"), bg="white", fg="#2c3e50").pack(side="top", pady=(150, 150))

        # Affichage en mode simulation
        self.simulation_frame = tk.Frame(self, bg='#f5f6f7') # On ne fait pas de .pack() ici pour qu'il soit caché au début
        
        # Paramètres à choisir à gauche
        self.sim_params_frame = tk.LabelFrame(self.simulation_frame, text=" PARAMÈTRES DE SIMULATION ", font=("Segoe UI", 30, "bold"), bg='#f5f6f7', fg="#2c3e50")
        self.sim_params_frame.pack(side="left", fill="both", padx=30, pady=(275,320), expand=False)

        # --- Sélecteur d'ÂGE ---
        tk.Label(self.sim_params_frame, text="Âge de l'audition :", font=("Segoe UI", 20), bg='#f5f6f7').pack(pady=(165, 0))
        self.age_var = tk.IntVar(value=20)
        self.age_scale = tk.Scale(self.sim_params_frame, from_=30, to=80, resolution=10, orient="horizontal", variable=self.age_var, bg='#f5f6f7', bd=0, length=460, width=30, sliderlength=75, sliderrelief='ridge', font=("Segoe UI", 16))
        self.age_scale.pack(padx=10)

        # --- Sélecteur de DÉGRADATION ---
        tk.Label(self.sim_params_frame, text="Niveau d'exposition sonore :", font=("Segoe UI", 20), bg='#f5f6f7').pack(pady=(60, 10))
        self.expo_var = tk.StringVar(value="Sain")
        self.option_add('*TCombobox*Listbox.font', ("Segoe UI", 16)) # pour changer la police du menu déroulant
        self.expo_menu = ttk.Combobox(self.sim_params_frame, textvariable=self.expo_var, values=["Sain", "Exposition modérée (Travail bruyant)", "Exposition forte (Concerts/Casque)", "Dégradation sévère"], state="readonly", font=("Segoe UI", 16), width=40)
        self.expo_menu.pack(padx=20, pady=10)

        # Audiogramme correspondant à la simulation à droite
        self.sim_graph_frame = tk.Frame(self.simulation_frame, bg='white', highlightbackground="#dddddd", highlightthickness=1)
        self.sim_graph_frame.pack(side="right", fill="both", expand=True, padx=30, pady=20)

        tk.Label(self.sim_graph_frame, text="PROFIL AUDITIF SIMULÉ", font=("Segoe UI", 30, "bold"), bg="white", fg="#2c3e50").pack(pady=(100,40))

        self.fig_sim, self.ax_sim = plt.subplots(figsize=(6, 4))
        self.canvas_sim = FigureCanvasTkAgg(self.fig_sim, master=self.sim_graph_frame)
        self.canvas_sim.get_tk_widget().pack(fill="both", expand=True)
        self.style_graph([self.ax_sim], [""], self.fig_sim)

        

    def diagnostic(self):

        # on réinitialise l'affichage des graphes
        self.style_graph([self.ax_gauche, self.ax_droite], ["OREILLE GAUCHE", "OREILLE DROITE"], self.fig)
        self.canvas.draw()

        # Affichage du popup
        self.popup_test()

        # Données fictives
        donnees = ['125.00,10.00,0.00', '250.00,0.00,0.00', '500.00,0.00,0.00', '1000.00,0.00,0.00', '2000.00,70.00,0.00', '4000.00,0.00,30.00', '8000.00,0.00,0.00']
        
        if donnees:

            # on ferme le pop up
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


            # Tracé
            self.ax_gauche.plot(freqs, gauche, color='#3498db', marker='x', markersize=8, linewidth=2)
            self.ax_droite.plot(freqs, droite, color='#e74c3c', marker='o', markersize=8, linewidth=2)
            
            self.fig.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.15, wspace=0.3)
            self.canvas.draw()
        else:
            messagebox.showwarning("Erreur", "Aucune donnée reçue.")



    def correction(self): 
        pass



    def simulation(self, ):
        # on nettoie le graphe
        self.style_graph([self.ax_sim], [""], self.fig_sim)



    def change_mode(self):
        # Si la zone correction est visible, on passe en simulation
        if self.correction_frame.winfo_viewable():
            # Cacher l'interface de diagnostic
            self.correction_frame.pack_forget()
            self.btn_diag.pack_forget()
            self.btn_correction.pack_forget()
            
            # Afficher l'interface de simulation
            self.simulation_frame.pack(fill="both", expand=True)
            
            # On place le bouton 'Lancer simulation' à gauche du bouton de mode
            self.btn_run_sim.pack(side="left", padx=10, expand=True, fill="both", before=self.btn_mode)      

            self.mode.config(text="MODE SIMULATION")
            self.btn_mode.config(text="Passer en mode correction")
        else:
            # Cacher l'interface de simulation
            self.simulation_frame.pack_forget()
            self.btn_run_sim.pack_forget()
            
            # Réafficher l'interface de diagnostic
            self.correction_frame.pack(fill="both", expand=True)
            
            # On réaffiche les boutons
            self.btn_correction.pack(side="left", padx=10, expand=True, fill="both", before=self.btn_mode)
            self.btn_diag.pack(side="left", padx=10, expand=True, fill="both", before=self.btn_correction) 
            
            self.mode.config(text="MODE CORRECTION")
            self.btn_mode.config(text="Passer en mode simulation")   



    def popup_test(self):
        self.popup = tk.Toplevel(self)
        self.popup.title("Information")
        self.popup.configure(bg="white")

        # Centrage dynamique
        w, h = 800, 300
        x = self.winfo_x() + (self.winfo_width() // 2) - (w // 2)
        y = self.winfo_y() + (self.winfo_height() // 2) - (h // 2)
        self.popup.geometry(f"{w}x{h}+{x}+{y}")
        
        self.popup.transient(self) # le popup appartient à la fenêtre principale
        self.popup.grab_set() # empêche d'interagir avec la fenêtre principale

        # Conteneur pour centrer les deux éléments
        info_frame = tk.Frame(self.popup, bg="white")
        info_frame.pack(expand=True)

    
        tk.Label(info_frame, text="ⓘ", font=("Segoe UI", 65), fg="#2196F3", bg="white").grid(row=0, column=0, padx=10, sticky="ns")
        tk.Label(info_frame, text="Test auditif en cours", font=("Segoe UI", 25, "bold"), bg="white").grid(row=0, column=1, pady=(15,0), sticky="ns")
        
        # On force la mise à jour visuelle pour que le popup apparaisse bien
        self.update()



    def style_graph(self, axes_cibles, titles, fig_cible):
            """ Configuration des graphiques """

            for ax, title in zip(axes_cibles, titles):
                ax.clear()
                ax.set_title(title, fontweight='bold')
                ax.set_xlabel("Fréquence (Hz)", color='#7f8c8d')
                ax.set_ylabel("Niveau (dB HL)", color='#7f8c8d')
                
                ax.set_xscale('log')
                ax.invert_yaxis()
                
                ax.grid(True, which="both", linestyle='--', alpha=0.3)


            if len(axes_cibles) > 1: # Mode Correction
                fig_cible.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.15, wspace=0.3)
            else: # Mode Simulation
                fig_cible.subplots_adjust(left=0.15, right=0.85, top=0.95, bottom=0.25)



    def on_close(self): 
        self.quit()
        self.destroy()



if __name__ == "__main__":
    app = Window()
    app.mainloop()