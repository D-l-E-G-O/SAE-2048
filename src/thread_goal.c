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
        // On copie l'état du jeu modifié dans le Tas
        ClientSession *heap_session = (ClientSession *)shm_slot->heap_session;
        heap_session->state = shm_slot->state;

        // Le Acknowledge du joueur
        if (shm_slot->state.game_over)
        {
            kill(shm_slot->input_pid, SIG_CLEAN_EXIT);
        }
        else
        {
            kill(shm_slot->input_pid, SIGUSR2); // Envoyer le ACK
        }

        // Libérer le slot de SHM pour le joueur suivant
        shm_slot->status = SLOT_FREE;
        pthread_cond_signal(&cond_free);

        pthread_mutex_unlock(&shm_mutex);
    }

    printf("[Goal] Fin du Thread.\n");
    return NULL;
}