#include "../include/game_threads.h"
#include "../include/utils.h"

// --- Fonctions utilitaires ---

// Vérifie si la tuile 2048 est présente
int check_victory(int cells[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (cells[i][j] == TARGET_VAL) return 1;
        }
    }
    return 0;
}

// Vérifie si le joueur a perdu (Grille pleine ET aucun mouvement possible)
int check_defeat(int cells[GRID_SIZE][GRID_SIZE]) {
    // 1. Si une case est vide, on n'a pas perdu
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (cells[i][j] == 0) return 0;
        }
    }

    // 2. Si des fusions sont possibles, on n'a pas perdu
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            // Test droite (sauf dernière colonne)
            if (j < GRID_SIZE - 1 && cells[i][j] == cells[i][j + 1]) return 0;
            // Test bas (sauf dernière ligne)
            if (i < GRID_SIZE - 1 && cells[i][j] == cells[i + 1][j]) return 0;
        }
    }

    // Sinon, c'est perdu
    return 1;
}

// --- Routine du Thread Goal ---

void *thread_goal_routine(void *arg) {
    // Récupération de l'argument
    int display_fd = *((int *)arg);

    while (true) {
        // 1. Verrouiller le Mutex
        pthread_mutex_lock(&state_mutex);

        // 2. Attendre le signal du Thread Move
        pthread_cond_wait(&cond_goal, &state_mutex);

        // --- SECTION CRITIQUE ---

        // Réinitialisation des flags par défaut
        // (On part du principe qu'on joue, sauf si victoire ou défaite détectée)
        // Note : Si on veut arrêter le jeu après victoire, on met game_over = true aussi.
        
        if (check_victory(current_state.cells)) {
            current_state.victory = true;
            current_state.game_over = true; // On arrête le jeu si gagné
        } 
        else if (check_defeat(current_state.cells)) {
            current_state.victory = false;
            current_state.game_over = true; // On arrête le jeu si perdu
        } 

        // 3. IPC Display : Écrire tout le GameState dans le Pipe Anonyme
        if (write(display_fd, &current_state, sizeof(GameState)) == -1) {
            perror("[GOAL] Erreur write pipe");
        }

        // 4. Déverrouiller le Mutex
        pthread_mutex_unlock(&state_mutex);

        if (current_state.game_over) {
            pthread_kill(main_thread_id, SIG_CLEAN_EXIT);
            break;
        }
    }

    pthread_exit(NULL);
}