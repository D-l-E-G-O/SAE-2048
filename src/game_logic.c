#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../include/game_logic.h"

// --- FONCTIONS PRIVÉES (Helpers) ---

// Retourne un entier aléatoire : soit 2 (90%), soit 4 (10%)
// Règle d'apparition standard du 2048
static int get_random_tile_value() {
    return (rand() % 10 == 0) ? 4 : 2;
}

// --- IMPLÉMENTATION DE L'INTERFACE ---

void init_game(GameState *state) {
    // Initialisation de la graine aléatoire
    srand(time(NULL));

    // Nettoyage de la mémoire (tout à 0)
    memset(state, 0, sizeof(GameState));

    // Apparition de deux tuiles au début
    spawn_tile(state);
    spawn_tile(state);
}

void spawn_tile(GameState *state) {
    // On cherche toutes les cases vides
    int empty_cells[GRID_SIZE * GRID_SIZE][2]; // Stocke les coordonnées {y, x}
    int count = 0;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (state->cells[y][x] == 0) {
                empty_cells[count][0] = y;
                empty_cells[count][1] = x;
                count++;
            }
        }
    }

    // S'il n'y a plus de place, on ne fait rien
    if (count == 0) return;

    // On choisit une case vide au hasard
    int index = rand() % count;
    int y = empty_cells[index][0];
    int x = empty_cells[index][1];

    // On y place une nouvelle tuile (2 ou 4)
    state->cells[y][x] = get_random_tile_value();
}

/*
 * ALGORITHME DE DÉPLACEMENT ET FUSION
 * Pour simplifier, on traite chaque ligne ou colonne comme un tableau 1D.
 * L'algo est :
 * 1. "Tasser" les non-zéros vers le début (supprimer les trous).
 * 2. Fusionner les adjacents identiques.
 * 3. "Tasser" à nouveau après fusion.
 */
bool move_grid(GameState *state, UserCommand dir) {
    bool moved = false;
    int score_gain = 0;

    // Tableaux temporaires pour les calculs
    int line[GRID_SIZE];
    int merged_line[GRID_SIZE];

    // On itère 4 fois (pour chaque ligne ou colonne)
    for (int i = 0; i < GRID_SIZE; i++) {
        
        // Extraction de la ligne/colonne concernée
        for (int j = 0; j < GRID_SIZE; j++) {
            switch (dir) {
                case CMD_LEFT:  line[j] = state->cells[i][j]; break; // Ligne i
                case CMD_RIGHT: line[j] = state->cells[i][GRID_SIZE - 1 - j]; break; // Ligne i inversée
                case CMD_UP:    line[j] = state->cells[j][i]; break; // Colonne i
                case CMD_DOWN:  line[j] = state->cells[GRID_SIZE - 1 - j][i]; break; // Colonne i inversée
                default: return false;
            }
        }

        // Compression (suppression des zéros)
        int pos = 0;
        int temp[GRID_SIZE] = {0};
        for (int j = 0; j < GRID_SIZE; j++) {
            if (line[j] != 0) {
                temp[pos++] = line[j];
            }
        }

        // Fusion
        for (int j = 0; j < GRID_SIZE - 1; j++) {
            // Si deux tuiles adjacentes sont identiques et non nulles
            if (temp[j] != 0 && temp[j] == temp[j+1]) {
                temp[j] *= 2;           // Fusion (valeur double)
                score_gain += temp[j];  // Mise à jour du score
                temp[j+1] = 0;          // La deuxième tuile disparaît
                j++; // On saute la case suivante pour éviter double fusion (ex: 2-2-2 -> 4-2, pas 4-0)
            }
        }

        // Seconde Compression (après fusions)
        pos = 0;
        memset(merged_line, 0, sizeof(merged_line));
        for (int j = 0; j < GRID_SIZE; j++) {
            if (temp[j] != 0) {
                merged_line[pos++] = temp[j];
            }
        }

        // Réécriture dans la grille et détection de changement
        for (int j = 0; j < GRID_SIZE; j++) {
            int val = merged_line[j];
            int current_val;

            // On récupère la valeur actuelle dans la grille pour comparer
            switch (dir) {
                case CMD_LEFT:  current_val = state->cells[i][j]; break;
                case CMD_RIGHT: current_val = state->cells[i][GRID_SIZE - 1 - j]; break;
                case CMD_UP:    current_val = state->cells[j][i]; break;
                case CMD_DOWN:  current_val = state->cells[GRID_SIZE - 1 - j][i]; break;
                default: current_val = 0;
            }

            if (current_val != val) {
                moved = true; // Quelque chose a changé
                // Écriture de la nouvelle valeur
                switch (dir) {
                    case CMD_LEFT:  state->cells[i][j] = val; break;
                    case CMD_RIGHT: state->cells[i][GRID_SIZE - 1 - j] = val; break;
                    case CMD_UP:    state->cells[j][i] = val; break;
                    case CMD_DOWN:  state->cells[GRID_SIZE - 1 - j][i] = val; break;
                    default: break;
                }
            }
        }
    }

    if (moved) {
        state->score += score_gain;
    }

    return moved; // Retourne true seulement si un coup valide a eu lieu
}

bool check_win(GameState *state) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (state->cells[y][x] == TARGET_VAL) { // Valeur 2048 atteinte
                state->victory = true;
                state->game_over = true;
                return true;
            }
        }
    }
    return false;
}

bool check_lose(GameState *state) {
    // S'il reste des cases vides, on n'a pas perdu
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (state->cells[y][x] == 0) return false;
        }
    }

    // Si aucune case vide, on regarde si une fusion est possible (voisins identiques)
    // Vérification horizontale et verticale
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            int val = state->cells[y][x];
            
            // Regarde à droite
            if (x < GRID_SIZE - 1 && state->cells[y][x+1] == val) return false;
            // Regarde en bas
            if (y < GRID_SIZE - 1 && state->cells[y+1][x] == val) return false;
        }
    }

    // Si on arrive ici : grille pleine + aucune fusion possible = Défaite
    state->game_over = true;
    state->victory = false;
    return true;
}