#define _XOPEN_SOURCE 700
#include "../include/game_threads.h"
#include "../include/utils.h"
#include "../include/game_logic.h"

/**
 * Routine du Thread Goal
 */
void *thread_goal_routine(void *arg)
{
    (void)arg;

    while (1)
    {
        pthread_mutex_lock(&shm_mutex);

        while (shm_slot->status != SLOT_TO_GOAL && !stop_requested)
        {
            pthread_cond_wait(&cond_goal, &shm_mutex);
        }

        if (stop_requested)
        {
            pthread_mutex_unlock(&shm_mutex);
            break;
        }

        // --- SECTION CRITIQUE ---
        if (check_win(&shm_slot->state))
        {
            // La mise à jour est faite directement dans shm_slot->state
        }
        else if (check_lose(&shm_slot->state))
        {
            // La mise à jour est faite directement dans shm_slot->state
        }

        // Mise à jour du display pour le joueur en question
        if (write(shm_slot->display_fd, &shm_slot->state, sizeof(GameState)) == -1)
        {
            perror("[GOAL] Erreur d'écriture dans le Pipe");
        }

        // --- Transfère du SHM vers le Tas ---
        pthread_mutex_lock(&heap_mutex);
        for (size_t i = 0; i < players.size; i++)
        {
            ClientSession *s = array_list_get_pointer_mut(&players, i);
            if (s->input_pid == shm_slot->input_pid)
            {
                s->state = shm_slot->state;
                break;
            }
        }
        pthread_mutex_unlock(&heap_mutex);

        // On sauvegarde les infos nécessaires pour les signaux
        bool const is_game_over = shm_slot->state.game_over;
        pid_t const player_pid = shm_slot->input_pid;

        // On libère la SHM immédiatement (Le slot est prêt pour le joueur suivant)
        shm_slot->status = SLOT_FREE;
        pthread_cond_signal(&cond_free);
        pthread_mutex_unlock(&shm_mutex);

        // --- Envoi des signaux (En dehors de la section critique) ---
        if (is_game_over)
        {
            kill(player_pid, SIG_CLEAN_EXIT);
        }
        else
        {
            kill(player_pid, SIGUSR2);
        }
    }

    // printf("[Goal] Fin du Thread.\n");
    return NULL;
}