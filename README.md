# SAE 2048

**SAE 2048** est un projet de 2e année de **BUT Informatique à l’IUT d’Illkirch** développé en C dont le but est de reproduire le célèbre jeu **2048**.
Il met en oeuvre une **séparation stricte des responsabilités** via plusieurs **processus** (Entrée, Jeu, Affichage) et **threads**, communiquant entre eux par des **pipes** (nommés et anonymes) et des **signaux**.

## Captures d'écran

### Interface de jeu (Console)
![Interface de jeu](captures/interface.png "Interface de jeu")

### Architecture Système
![Architecture Système](captures/architecture.png "Architecture Système")

## Technologies utilisées

* **Langage :** C (Standards POSIX)
* **Concepts :** Processus, Threads (pthreads), Pipes, Signaux
* **IDE :** Visual Studio Code

## Équipe

* Nombre de développeurs : **3**
* Durée du projet : 3 semaines

## Installation et exécution

1. **Cloner le dépôt :**
```bash
git clone https://github.com/D-l-E-G-O/SAE-2048.git
cd SAE-2048
```


2. **Compilation :**
```bash
make
```


3. **Préparation de l'environnement :**
Le jeu utilise un *pipe nommé* pour la communication. Le script ou le programme se charge généralement de le créer, mais assurez-vous que le dossier `/tmp` est accessible.
4. **Lancer le jeu :**
Il est nécessaire de lancer le moteur de jeu dans un terminal, et le contrôleur (input) dans un autre (ou via un script global).
```bash
# Terminal 1 (Moteur de jeu et Affichage)
./bin/game_2048

# Terminal 2 (Contrôleur clavier)
./bin/input
```



> [!IMPORTANT]
> Ce projet repose sur des **mécanismes IPC (Inter-Process Communication)** spécifiques à Linux/Unix. Il ne fonctionnera pas nativement sous Windows sans un environnement type WSL (Windows Subsystem for Linux).

## Fonctionnalités principales

- **Architecture Multi-processus :** Séparation distincte entre la gestion des entrées (Main), la logique du jeu (2048) et le rendu visuel (Affichage).
 
- **Gestion Multi-threads :** Le coeur du jeu utilise 3 threads dédiés pour gérer les entrées, le calcul des mouvements (Move & Score) et la vérification des conditions de victoire (Goal).

- **Communication IPC Avancée :**
 
- **Pipe Nommé** pour la transmission des commandes joueur vers le moteur.
 
- **Pipe Anonyme** pour l'envoi de la grille vers le processus d'affichage.
 
- **Synchronisation par Signaux :** Coordination temps réel entre les processus et les threads pour un affichage fluide.

- **Algorithme de jeu complet :** Gestion des fusions, apparition aléatoire de tuiles (2 ou 4) et détection de Game Over/Victoire.
