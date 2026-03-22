#pragma once

#include <termios.h>
#include "common.h"

// Efface le terminal proprement
// Utilisé par le processus Affichage avant de redessiner
void clear_screen();

// Configure le terminal en mode "raw" (non-canonique)
// Permet de capturer les touches sans appuyer sur Entrée.
// Utilisé par le processus Input
// Retourne la configuration d'origine pour pouvoir la restaurer à la fin.
struct termios set_raw_mode();

// Restaure le terminal dans son état d'origine (avec écho et buffer ligne)
void restore_mode(struct termios orig_termios);

// Affiche la grille avec des caractères ascii
void print_grid_ascii(const GameState *state);