#include <stdio.h>
#include <pthread.h>
#include "../include/game_threads.h"
#include "../include/game_logic.h"

/**
 * Routine du Thread "Move & Score"
 * Rôle :
 * 1. Attendre une commande de direction venant du Thread Principal.
 * 2. Verrouiller l'état du jeu.
 * 3. Appliquer le mouvement (via game_logic).
 * 4. Si le mouvement est valide, ajouter une tuile.
 * 5. Réveiller le Thread Goal pour vérification.
 */
void *thread_move_routine(void *arg)
{
    (void)arg; // Pour éviter le warning "unused parameter"

    while (!stop_requested)
    {
        while (!input_data.has_new_cmd && !stop_requested)
        {
            usleep(1000);
        }

        if (stop_requested)
            break;

        UserCommand const cmd = input_data.cmd;
        input_data.has_new_cmd = false;

        if (cmd == CMD_QUIT)
            break;

        if (cmd >= CMD_UP && cmd <= CMD_RIGHT)
        {
            bool const moved = move_grid(&active_session->current_state, cmd);

            if (moved)
            {
                spawn_tile(&active_session->current_state);
                printf("[Move Thread] Move applied (%d). Score: %d\n", cmd, current_state.score);

                // Déclencher le thread Goal
                grid_has_changed = true;
            }
            else
            {
                printf("[Move Thread] Invalid move!\n");
            }
        }
    }

    printf("[Move Thread] Thread terminated.\n");
    return NULL;
}