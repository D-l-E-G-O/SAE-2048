#pragma once

#include <string.h>
#include "common.h"

// Initialise la grille : tout à 0, puis spawn 2 tuiles aléatoires.
void init_game(GameState *state);

// Tente de déplacer la grille dans une direction donnée.
// Gère le glissement et la fusion des tuiles.
// Retourne true si au moins une tuile a bougé (coup valide).
bool move_grid(GameState *state, UserCommand dir);

// Ajoute une tuile (2 ou 4) aléatoirement sur une case vide.
// A appeler après chaque déplacement valide.
void spawn_tile(GameState *state);

// Vérifie si la valeur 2048 est présente dans la grille.
// Retourne true si victoire.
bool check_win(GameState *state);

// Vérifie si plus aucun coup n'est possible (grille pleine + pas de fusions possibles).
// Retourne true si défaite.
bool check_lose(GameState *state);