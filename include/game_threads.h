#pragma once

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include "common.h"
#include "array_list.h"

/* Statut actuel de l'emplacement de mémoire partagé */
typedef enum
{
    SLOT_FREE,
    SLOT_IDLE,
    SLOT_TO_MOVE,
    SLOT_TO_GOAL
} SlotStatus;

/* Structure du segment de mémoire partagé */
typedef struct SharedGameSlot
{
    GameState state;
    UserCommand cmd;
    int display_fd;
    pid_t input_pid;
    SlotStatus status;
} SharedGameSlot;

// --- Variables globales dynamiques pour N slots ---
extern int num_slots;
extern int shm_id;
extern SharedGameSlot *shm_slots;

// --- Synchronisation multi-thread (Tableaux dynamiques) ---
extern pthread_mutex_t heap_mutex;
extern pthread_mutex_t *slot_mutexes; // Un verrou par slot
extern pthread_cond_t *cond_moves;    // Condition Move par slot
extern pthread_cond_t *cond_goals;    // ondition Goal par slot
extern pthread_cond_t *cond_frees;    // Condition Free/Idle par slot

extern pthread_t main_thread_id;
extern volatile sig_atomic_t stop_requested;

extern array_list players;

typedef struct ClientSession
{
    pid_t input_pid;
    pid_t display_pid;
    int display_fd;
    GameState state;
    int slot_index;
} ClientSession;

void *thread_move_routine(void *arg);
void *thread_goal_routine(void *arg);
