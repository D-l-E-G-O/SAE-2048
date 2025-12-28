#pragma once

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "common.h"

// --- VARIABLES PARTAGÉES (Déclarations "extern") ---
// Le mot-clé "extern" dit : "C'est défini dans un autre .c, mais je l'utilise ici"

extern GameState current_state;      // L'état du jeu
extern pthread_mutex_t state_mutex;  // Le mutex pour protéger la grille
extern pthread_cond_t cond_move;     // Condition pour réveiller le move
extern pthread_cond_t cond_goal;     // Condition pour réveiller le goal

// Structure pour la communication Thread Main -> Thread Move
typedef struct InputSharedData {
    UserCommand cmd;
    bool has_new_cmd;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} InputSharedData;

// Structure pour passer les arguments au Thread Goal
typedef struct {
    int pipe_fd;        // Descripteur d'écriture du pipe anonyme
    pid_t display_pid;  // PID du processus d'affichage
} GoalThreadArgs;

extern InputSharedData input_data;   // Données venant du clavier

// --- PROTOTYPES DES FONCTIONS DE THREADS ---
void *thread_move_routine(void *arg);
void *thread_goal_routine(void *arg);
void *thread_main_routine(void *arg);

void process_2048(pid_t pid_process_display);
void process_display();
int check_victory(int cells[GRID_SIZE][GRID_SIZE]);
int check_defeat(int cells[GRID_SIZE][GRID_SIZE]);
