#!/usr/bin/env python3

import tkinter as tk
from tkinter import ttk, messagebox  # Ajout de messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

class Window(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("TeensHear - Appareil auditif et de prévention")
        self.geometry("900x700")
        
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
        
        # Interface
        self.create_widgets()

        # Matplotlib (pas dynamique) : cadran qui reste tout le temps 
        self.fig, self.ax1 = plt.subplots(1, 1, figsize=(8, 6))
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

        
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

        ttk.Button(control_frame, text="Lancer le diagnostic", command=self.diagnostic).pack(side="left", padx=5)
        ttk.Button(control_frame, text="Démarrer la simulation", command=self.simulation).pack(side="left", padx=5)
        

        ## fenetre pr le mpl
        self.graph_frame = ttk.LabelFrame(self, text="Graphiques")
        self.graph_frame.pack(fill="both", expand=True, padx=10, pady=5)
    
    def diagnostic(self):
        pass
    
    def simulation(self):
        pass


    
                  
# main

if __name__ == "__main__":
    pass
