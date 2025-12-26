#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      // Pour STDIN_FILENO
#include <termios.h>     // Pour struct termios, tcgetattr, tcsetattr
#include "../include/utils.h"
#include "../include/common.h"


// =========================================================
// GESTION DE L'AFFICHAGE (Utilisé par Display)
// =========================================================

void clear_screen() {
    // Utilisation des codes ANSI pour nettoyer le terminal
    // \033[H  : Place le curseur en haut à gauche (Home)
    // \033[2J : Efface tout l'écran (J)
    // C'est beaucoup plus rapide et fluide que system("clear")
    printf("\033[H\033[2J");
    fflush(stdout); // On force l'écriture immédiate
}

// =========================================================
// GESTION DU TERMINAL (Utilisé par Input)
// =========================================================

/**
 * Configure le terminal en mode "RAW" (Brut).
 * 1. Désactive le buffer de ligne (ICANON) : La saisie est détectée dès l'appui touche.
 * 2. Désactive l'écho (ECHO) : Les caractères tapés ne s'affichent pas.
 */
struct termios set_raw_mode() {
    struct termios orig_termios;
    struct termios raw;

    // 1. On récupère la configuration actuelle
    // STDIN_FILENO est le descripteur du clavier (0)
    tcgetattr(STDIN_FILENO, &orig_termios);

    // 2. On copie la config pour la modifier
    raw = orig_termios;

    // 3. Modification des flags (opérations binaires)
    // ~ICANON : Inverse le bit canonique (on le passe à 0)
    // ~ECHO   : Inverse le bit écho (on le passe à 0)
    raw.c_lflag &= ~(ICANON | ECHO);

    // 4. Application immédiate (TCSANOW) de la nouvelle config
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    // On retourne l'ancienne config pour pouvoir restaurer plus tard
    return orig_termios;
}

void restore_mode(struct termios orig_termios) {
    // Restaure la configuration sauvegardée (avec écho et buffer)
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// =========================================================
// FONCTIONS DE DEBUG / AIDE
// =========================================================

void print_grid_ascii(const GameState *state) {
    printf("Score: %d\n", state->score);
    printf("---------------------\n");
    for (int y = 0; y < GRID_SIZE; y++) {
        printf("|");
        for (int x = 0; x < GRID_SIZE; x++) {
            int val = state->cells[y][x];
            if (val == 0) {
                printf("  .  "); // Point pour case vide
            } else {
                printf("%5d", val); // Alignement sur 5 espaces
            }
        }
        printf("|\n");
    }
    printf("---------------------\n");
}