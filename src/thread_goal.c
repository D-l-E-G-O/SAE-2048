#define _XOPEN_SOURCE 700
#include "../include/game_threads.h"
#include "../include/game_logic.h"

/**
 * Routine du Thread Goal
 */
void *thread_goal_routine(void *arg)
{
    int slot_index = *(int *)arg;

    while (1)
    {
        pthread_mutex_lock(&slot_mutexes[slot_index]);

        while (shm_slots[slot_index].status != SLOT_TO_GOAL && !stop_requested)
        {
            pthread_cond_wait(&cond_goals[slot_index], &slot_mutexes[slot_index]);
        }

        if (stop_requested)
        {
            pthread_mutex_unlock(&slot_mutexes[slot_index]);
            break;
        }

        // --- SECTION CRITIQUE ---
        if (check_win(&shm_slots[slot_index].state))
        {
            // La mise à jour est faite directement dans shm_slots[slot_index].state
        }
        else if (check_lose(&shm_slots[slot_index].state))
        {
            // La mise à jour est faite directement dans shm_slots[slot_index].state
        }

        // Mise à jour du display pour le joueur en question
        if (write(shm_slots[slot_index].display_fd, &shm_slots[slot_index].state, sizeof(GameState)) == -1)
        {
            perror("[GOAL] Erreur d'écriture dans le Pipe");
        }

        // --- Transfère du SHM vers le Tas ---
        pthread_mutex_lock(&heap_mutex);
        for (size_t i = 0; i < players.size; i++)
        {
            ClientSession *s = array_list_get_pointer_mut(&players, i);
            if (s->input_pid == shm_slots[slot_index].input_pid)
            {
                s->state = shm_slots[slot_index].state;
                break;
            }
        }
        pthread_mutex_unlock(&heap_mutex);

        // On sauvegarde les infos nécessaires pour les signaux
        bool const is_game_over = shm_slots[slot_index].state.game_over;
        pid_t const player_pid = shm_slots[slot_index].input_pid;

        // Le slot redevient IDLE, prêt pour le prochain mouvement de CE joueur
        shm_slots[slot_index].status = SLOT_IDLE;
        pthread_cond_signal(&cond_frees[slot_index]);
        pthread_mutex_unlock(&slot_mutexes[slot_index]);

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

    return NULL;
}