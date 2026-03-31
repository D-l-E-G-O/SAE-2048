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
    // On récupère l'index du slot attribué à ce thread
    int slot_index = *(int *)arg;

    while (1)
    {
        // On ne verrouille QUE le mutex de ce slot
        pthread_mutex_lock(&slot_mutexes[slot_index]);

        while (shm_slots[slot_index].status != SLOT_TO_MOVE && !stop_requested)
        {
            pthread_mutex_unlock(&slot_mutexes[slot_index]);
            sem_wait(&sem_moves[slot_index]);
            pthread_mutex_lock(&slot_mutexes[slot_index]);
        }

        if (stop_requested)
        {
            pthread_mutex_unlock(&slot_mutexes[slot_index]);
            break;
        }

        // --- SHM CRITICAL SECTION ---
        bool const moved = move_grid(&shm_slots[slot_index].state, shm_slots[slot_index].cmd);

        if (moved)
        {
            spawn_tile(&shm_slots[slot_index].state);
        }

        // On passe le relais au Thread Goal de CE slot
        shm_slots[slot_index].status = SLOT_TO_GOAL;
        sem_post(&sem_goals[slot_index]);

        pthread_mutex_unlock(&slot_mutexes[slot_index]);
    }

    return NULL;
}