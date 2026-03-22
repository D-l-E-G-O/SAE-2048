#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <pthread.h>
#include "../include/game_threads.h"
#include "../include/game_logic.h"

/**
 * Routine du Thread Move & Score
 */
void *thread_move_routine(void *arg)
{
    (void)arg; // Pour éviter le warning "unused parameter"

    while (1)
    {
        pthread_mutex_lock(&shm_mutex);

        while (shm_slot->status != SLOT_TO_MOVE && !stop_requested)
        {
            pthread_cond_wait(&cond_move, &shm_mutex);
        }

        if (stop_requested)
        {
            pthread_mutex_unlock(&shm_mutex);
            break;
        }

        // --- SHM CRITICAL SECTION ---
        bool const moved = move_grid(&shm_slot->state, shm_slot->cmd);

        if (moved)
        {
            spawn_tile(&shm_slot->state);
            printf("[Move] Deplacement appliqué\n");
        }
        else
        {
            printf("[Move] Deplacement invalide !\n");
        }

        // Pass control to Goal Thread
        shm_slot->status = SLOT_TO_GOAL;
        pthread_cond_signal(&cond_goal);

        pthread_mutex_unlock(&shm_mutex);
    }

    // printf("[Move] Fin du Thread.\n");
    return NULL;
}