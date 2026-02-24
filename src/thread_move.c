#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <pthread.h>
#include "../include/game_threads.h"
#include "../include/game_logic.h"

volatile sig_atomic_t active_move = 0;

void start_move(int sig)
{
    active_move = 1;
}

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

    struct sigaction sa;
    sa.sa_handler = start_move;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    while (!stop_requested)
    {
        pause();
        GameState state = active_session->state;

        if (stop_requested)
            break;

        UserCommand const cmd = input_data.cmd;
        input_data.has_new_cmd = false;

        if (cmd == CMD_QUIT)
            break;

        if (cmd >= CMD_UP && cmd <= CMD_RIGHT)
        {
            bool const moved = move_grid(&active_session->state, cmd);

            if (moved)
            {
                spawn_tile(&active_session->state);
                printf("[Move] Deplacement appliqué (%d). Score: %d\n", cmd, state.score);

                // Déclencher le thread Goal
                grid_has_changed = true;
                pthread_kill(goal_thread_id, SIGUSR2);
            }
            else
            {
                printf("[Move] Deplacement invalide !\n");
                engine_busy = 0;
            }
        }
        active_move = 0;
    }

    printf("[Move] Fin du Thread.\n");
    return NULL;
}