#!/usr/bin/env python3

import os
import signal
import tkinter as tk
from tkinter import ttk, messagebox  # Ajout de messagebox
import random
import time
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.animation as animation

class PPCApp(tk.Tk):
    def __init__(self, situation_queue, parameters_queue, shutdown):
        super().__init__()
        self.situation_queue = situation_queue
        self.title("PPC Simulation - The Circle of Life")
        self.geometry("900x700")
        self.shutdown = shutdown

        # --- STYLE DES BOUTONS ---
        self.style = ttk.Style()
        self.style.theme_use('clam')
        
        self.style.configure('Rounded.TButton',
                             font=('Segoe UI', 9),
                             padding=6,
                             relief="flat",
                             borderwidth=1)
        
        # quand pointé
        self.style.map('Rounded.TButton',
                       background=[('active', '#e1e1e1')],
                       relief=[('pressed', 'sunken')])
        
        self.parameters_queue = parameters_queue

        self.simulation_running = False
        self.is_drought = False

        # Données pour le test
        self.times = []
        self.predators = []
        self.preys = []
        self.grass = []

        # Interface
        self.create_widgets()
        self.initial_popup()

        # Matplotlib (pas dynamique) : cadran qui reste tout le temps 
        self.fig, self.ax1 = plt.subplots(1, 1, figsize=(8, 6))
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

        # Animation -> dynamique : graphique 
        self.ani = animation.FuncAnimation(self.fig, self.update_graph, interval=500)
        
        # fermer si cliqué sur la croix 
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def on_close(self): 
        """ Arrête le programme lancé. 
            Paramètres : 
                Aucun
            Return :
                Aucun
        """
        if hasattr(self, "ani"):
            self.ani.event_source.stop()

        self.shutdown.set() # Signaler à tous les processus de s'arrêter via l'événement partagé
        
        self.quit()
        self.destroy()  # Fermer la fenêtre Tkinter


    def create_widgets(self):
        """ Ajouter les widget (boutons, console ,...) sur la fenêtre tkinter.
        Paramètres : 
            Aucun
        Return :
            Aucun"""
        control_frame = ttk.LabelFrame(self, text="Contrôles")
        control_frame.pack(fill="x", padx=10, pady=5)

        ttk.Button(control_frame, text="Ajouter une proie", command=self.add_prey).pack(side="left", padx=5)
        ttk.Button(control_frame, text="Ajouter un prédateur", command=self.add_predator).pack(side="left", padx=5)
        ttk.Button(control_frame, text="Ajouter 5 brins d'herbe", command=self.add_grass).pack(side="left", padx=5)
        ttk.Button(control_frame, text="Recommencer", command=self.reset_simulation).pack(side="left", padx=5)

        # Événements -> pas sure de à quoi ça sert
        event_frame = ttk.LabelFrame(self, text="Événements")
        event_frame.pack(fill="x", padx=10, pady=5)
        self.event_log = tk.Text(event_frame, height=8)
        self.event_log.pack(fill="both", padx=5, pady=5)

        ## fenetre pr le mpl
        self.graph_frame = ttk.LabelFrame(self, text="Graphiques")
        self.graph_frame.pack(fill="both", expand=True, padx=10, pady=5)

    def initial_popup(self, wrong = False):
        """ Créer et afficher le pop-up de début de simulation.
            Aucun
        Return :
            Aucun"""
        
        self.popup = tk.Toplevel(self)
        self.popup.title("Choix des paramètres initiaux")
        ##self.popup.geometry("500x450")
        self.popup.transient(self)   # le popup appartient à la fenêtre principale
        self.popup.grab_set()        # empêche d'interagir avec la fenêtre principale
        self.popup.focus_force()     # force le focus sur le popup

        if wrong : 
            ttk.Label(self.popup, text="Veuillez mettre au moins 1 individu", foreground='red', font = 'bold').pack(pady=(15, 0))

        ttk.Label(self.popup, text="Nombre de proies").pack(pady=(10, 5))
        self.entry_preys = ttk.Entry(self.popup)
        self.entry_preys.pack(padx = 100, pady=5)
        self.entry_preys.insert(0, "")

        ttk.Label(self.popup, text="Nombre de prédateurs").pack(pady=5)
        self.entry_predators = ttk.Entry(self.popup)
        self.entry_predators.pack(pady=5)
        self.entry_predators.insert(0, "")

        ttk.Label(self.popup, text="Quantité d'herbe").pack(pady=5)
        self.entry_grass = ttk.Entry(self.popup)
        self.entry_grass.pack(pady=5)
        self.entry_grass.insert(0, "")

        ttk.Button(self.popup, text="Démarrer", command=self.start_with_values).pack(pady=15)

        self.popup.bind('<Return>', lambda e: self.start_with_values())


    def start_with_values(self):
        """ Récupérer les champs saisis par l'utilisateur.
        Paramètres : 
            Aucun
        Return :
            Aucun"""
        # On ne fixe le start_time qu'au moment du lancement réel
        try:
            n_preys = int(self.entry_preys.get())
            n_predators = int(self.entry_predators.get())
            grass = int(self.entry_grass.get())
        except ValueError:
            return

        # Vérification de sécurité
        if n_preys == 0 and n_predators == 0:
            self.popup.destroy()
            self.initial_popup(wrong=True)
            return

        self.times = [0]
        self.preys = [n_preys]
        self.predators = [n_predators]
        self.grass = [grass]
        
        # On envoie les paramètres
        self.parameters_queue.put(("INIT", n_preys, n_predators, grass))
        self.popup.destroy()
        
        # ON NE LANCE PAS ENCORE LE GRAPHique
        self.is_loading = True 
        self.log_event("Initialisation du jeu en cours...")
    
    # animation mpl

    def update_graph(self, frame):
        """ ACtualisation du graphe matplotlib
        Paramètres : 
            Aucun
        Return :
            Aucun"""
        if not hasattr(self, 'is_loading'): self.is_loading = False
        
        data_received = False
        
        try:
            while not self.situation_queue.empty():
                data = self.situation_queue.get_nowait()
                
                # Si on reçoit des données et qu'on attendait le chargement
                if self.is_loading:
                    self.is_loading = False
                    self.simulation_running = True
                    self.start_time = time.time()
                    self.log_event("Simulation démarrée")

                # 1. VERIFICATION PRIORITAIRE DU GAME OVER
                if data.get("event") == "GAME_OVER":
                    self.show_game_over(data)
                    break # On sort pour ne pas tracer des données vides

                # 2. TRAITEMENT DES DONNEES NORMALES
                t = data.get("time", 0) - self.start_time
                if t >= 0:
                    self.times.append(t)
                    self.grass.append(data["grass"])
                    self.preys.append(data["preys"])
                    self.predators.append(data["predators"])
                    self.is_drought = data.get("drought")
                    data_received = True
                
            if data_received and len(self.times) > 1:
                # On crée une liste d'index (0, 1, 2, 3...)
                indices = list(range(len(self.times)))
                
                # On trie ces index en fonction de la valeur dans self.times
                # C'est l'étape magique simplifiée : on dit à Python de trier les positions
                indices.sort(key=lambda i: self.times[i])

                # On reconstruit les listes dans le bon ordre grâce aux index triés
                self.times = [self.times[i] for i in indices]
                self.grass = [self.grass[i] for i in indices]
                self.preys = [self.preys[i] for i in indices]
                self.predators = [self.predators[i] for i in indices]

        except Exception as e:
            print(f"Erreur queue: {e}")


        # Mise à jour du Graphe
        self.ax1.clear()
        if self.is_loading:
            # ÉCRAN DE CHARGEMENT
            # Petit texte d'attente
            self.ax1.text(0.5, 0.6, " CREATION DU JEU EN COURS", 
                          transform=self.ax1.transAxes, color='#1e1e1e',
                          ha='center', va='center', fontsize=12, fontweight='bold')
            self.ax1.set_xticks([]); self.ax1.set_yticks([])
            # Animation de chargement (points qui défilent)
            dots = "." * (int(time.time() * 3) % 4)
            self.ax1.text(0.5, 0.4, f"Veuillez patienter{dots}", 
                          transform=self.ax1.transAxes, color='#1e1e1e',
                          ha='center', va='center', fontsize=10)
            
            # On cache les axes pendant le chargement
            self.ax1.set_xticks([])
            self.ax1.set_yticks([])
        elif self.simulation_running:    
            # AJOUT ALERTE SUR LE CANEVAS
            if self.is_drought:
                self.ax1.set_facecolor('#ffe6e6')
                self.ax1.text(0.5, 0.5, 'SÉCHERESSE', transform=self.ax1.transAxes, fontsize=30, color='red', alpha=0.5, ha='center', va='center', fontweight='bold', bbox=dict(facecolor='white', alpha=0.5))
            else:
                self.ax1.set_facecolor('white')

            self.ax1.plot(self.times, self.predators, label="Prédateurs", color="red", lw=2)
            self.ax1.plot(self.times, self.preys, label="Proies", color="blue", lw=2)
            self.ax1.plot(self.times, self.grass, label="Herbe", color="green", lw=1, linestyle="--")
                

            self.ax1.legend(loc='upper left')
            self.ax1.set_xlim(left=0) # Force l'axe X à démarrer à 0
            self.ax1.set_ylim(bottom = 0)
            self.ax1.set_ylabel("Nombre d'individus")
            self.ax1.set_xlabel("Temps écoulé (s)")
            self.ax1.set_title("Évolution des populations")
            self.ax1.grid(True, alpha=0.3)

            
            # Force le rafraîchissement
        self.canvas.draw_idle()

    def show_game_over(self, stats):   
        """ Afiche la messageBox de fin de simulation.
            Aucun
        Return :
            Aucun"""
        
        self.simulation_running = False

        msg = (f"Fin de la simulation (Extinction totale) !\n\n"
               f" Durée : {round(stats['duration'] - self.start_time,1)} secondes\n"
               f" Proies mortes : {stats['dead_preys']}\n"
               f" Prédateurs morts : {stats['dead_preds']}\n"
               f" Herbe mangée : {stats['grass_eaten']}\n\n"
               f"Voulez-vous recommencer une partie ?")
        rejouer = messagebox.askyesno("Bilan de la Simulation", msg)  
        if rejouer:
            self.reset_simulation()
        else:
            self.on_close() 
            
    ## Boutons

    def add_prey(self):
        self.parameters_queue.put(("ADD_PREY",))
        self.log_event("Nouvelle proie ajoutée")

    def add_predator(self):
        self.parameters_queue.put(("ADD_PREDATOR",))
        self.log_event("Nouveau prédateur ajouté")

    def add_grass(self):
        self.parameters_queue.put(("ADD_GRASS",))
        self.log_event("Herbe ajoutée")

    def reset_simulation(self):
        self.parameters_queue.put(("RESET",))
        
        try:
            while not self.situation_queue.empty():
                self.situation_queue.get_nowait()
        except:
            pass # La queue est vide

        self.times, self.predators, self.preys, self.grass = [], [], [], []
        
        self.log_event("Simulation réinitialisée")
        self.simulation_running = False
        self.is_loading = False
        self.is_drought = False

        self.ax1.clear()
        self.canvas.draw_idle()

        if hasattr(self, "ani") and not self.ani.event_source:
            self.ani = animation.FuncAnimation(self.fig, self.update_graph, interval=500)
            # self.ani = animation.FuncAnimation(self.fig, self.update_graph, interval=500)
            
        self.initial_popup()

    def log_event(self, msg):
        self.event_log.insert("end", msg + "\n")
        self.event_log.see("end")

# main

if __name__ == "__main__":
    # Note : Nécessite situation_queue et parameters_queue pour tester seul
    pass
