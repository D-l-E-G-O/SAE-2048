#pragma once

#include <stdbool.h>
#include <sys/types.h> // Pour pid_t

// --- CONSTANTES DU JEU ---
#define GRID_SIZE 4
#define TARGET_VAL 2048

// --- CONFIGURATION IPC ---
// Chemin du pipe nommé créé par le processus Input
#define NAMED_PIPE_PATH "/tmp/fifo_2048_input"

// --- STRUCTURES D'ECHANGE ---

// Commandes envoyées par le Processus INPUT vers 2048 (via Pipe Nommé)
typedef enum UserCommand {
    CMD_NONE = 0,
    CMD_UP,
    CMD_DOWN,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_QUIT,
    CMD_HANDSHAKE   // Utilisé pour envoyer le PID au démarrage
} UserCommand;

// Structure wrapper pour le Pipe Nommé
typedef struct InputPacket {
    UserCommand cmd;    // Information à transmettre
    pid_t sender_pid;   // PID de l'envoyeur
} InputPacket;

// Etat du jeu envoyé par le processus 2048 vers le processus Affichage (via Pipe Anonyme)
// __attribute__((packed)) assure que la struct a la même taille binaire partout
typedef struct __attribute__((packed)) GameState {
    int cells[GRID_SIZE][GRID_SIZE];    // La grille de valeurs
    int score;                          // Score
    bool game_over;                     // True si la partie est finie, False si la partie est en cours
    bool victory;                       // True si gagné (2048 atteint)
} GameState;
