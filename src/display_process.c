#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/common.h"
#include "../include/utils.h"

// --- COULEURS ANSI ---
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_GRID    "\033[38;5;240m"  // Gris foncé pour les cases
#define COLOR_TITLE   "\033[36m"        // Cyan
#define COLOR_SCORE   "\033[33m"        // Jaune
#define COLOR_WIN     "\033[32m"        // Vert
#define COLOR_LOSE    "\033[31m"        // Rouge

// Fonction interne pour choisir la couleur d'un nombre
const char* get_color(int value) {
    switch(value) {
        case 0:    return "\033[38;5;236m"; // Gris très sombre (vide)
        case 2:    return "\033[37m";       // Blanc
        case 4:    return "\033[33m";       // Jaune pâle
        case 8:    return "\033[38;5;208m"; // Orange
        case 16:   return "\033[38;5;202m"; // Orange foncé
        case 32:   return "\033[31m";       // Rouge
        case 64:   return "\033[35m";       // Magenta
        case 128:  return "\033[36m";       // Cyan
        default:   return "\033[32m";       // Vert (Niveaux élevés)
    }
}

void draw_interface(const GameState *state) {
    clear_screen(); // Fonction définie dans utils.c

    // En-tête
    printf(COLOR_BOLD "=== PROJET 2048 ===" COLOR_RESET "\n");
    printf("Score: " COLOR_SCORE "%d" COLOR_RESET "\n\n", state->score);

    // Dessin de la grille ligne par ligne
    printf(COLOR_GRID "┌──────┬──────┬──────┬──────┐\n" COLOR_RESET);
    
    for (int y = 0; y < GRID_SIZE; y++) {
        printf(COLOR_GRID "│" COLOR_RESET); // Bord gauche
        
        for (int x = 0; x < GRID_SIZE; x++) {
            int val = state->cells[y][x];
            
            if (val == 0) {
                // Case vide : on affiche un point ou rien
                printf("      ");
            } else {
                // Case remplie : on centre le nombre et on met la couleur
                const char* c = get_color(val);
                // %4d assure que le nombre prend 4 espaces. 
                printf("%s %4d " COLOR_RESET, c, val);
            }
            printf(COLOR_GRID "│" COLOR_RESET); // Séparateur vertical
        }
        printf("\n"); // Fin de la ligne de chiffres
        
        // Séparateur horizontal
        if (y < GRID_SIZE-1) {
            printf(COLOR_GRID "├──────┼──────┼──────┼──────┤\n" COLOR_RESET);
        }
    }
    printf(COLOR_GRID "└──────┴──────┴──────┴──────┘\n" COLOR_RESET);

    // Messages de fin
    if (state->game_over) {
        printf("\n");
        if (state->victory) {
            printf(COLOR_WIN COLOR_BOLD "VICTOIRY ! You reached %d !\n" COLOR_RESET, TARGET_VAL);
        } else {
            printf(COLOR_LOSE COLOR_BOLD "GAME OVER... No more moves.\n" COLOR_RESET);
        }
        printf("Press Q in the Input Window to leave.\n");
    }

    // Forcer l'affichage immédiat
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    // Le file descriptor du pipe anonyme est passé en argument (argv[1])
    // par le processus père via execl.
    if (argc < 2) {
        fprintf(stderr, "Erreur: Ce programme doit être lancé par le moteur de jeu.\n");
        return EXIT_FAILURE;
    }

    int pipe_fd = atoi(argv[1]);
    GameState buffer;
    ssize_t bytes_read;

    // --- BOUCLE PRINCIPALE ---
    // read() est bloquant : le programme s'endort ici tant qu'il n'y a rien à lire.
    // Il se réveille dès que le thread Goal écrit dans le pipe.
    while ((bytes_read = read(pipe_fd, &buffer, sizeof(GameState))) > 0) {
        
        // Sécurité : Vérifier qu'on a bien lu une structure entière
        if (bytes_read != sizeof(GameState)) {
            fprintf(stderr, "Erreur lecture pipe: paquet incomplet\n");
            continue;
        }

        draw_interface(&buffer);

        // Si le jeu est fini, on peut choisir de sortir de la boucle ou d'attendre
        // que le père tue le processus. Ici, on continue d'afficher (au cas où).
    }

    // Si on sort de la boucle, c'est que le pipe a été fermé (le père est mort)
    printf("Connexion au moteur perdue. Fermeture de l'affichage.\n");
    close(pipe_fd);
    return EXIT_SUCCESS;
}