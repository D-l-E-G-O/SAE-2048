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

extern int shm_id;
extern SharedGameSlot *shm_slot;

// --- Synchronisation multi-thread ---
extern pthread_mutex_t heap_mutex;
extern pthread_mutex_t shm_mutex;
extern pthread_cond_t cond_move;
extern pthread_cond_t cond_goal;
extern pthread_cond_t cond_free;

extern pthread_t main_thread_id;
extern volatile sig_atomic_t stop_requested;

extern array_list players;

typedef struct ClientSession
{
    pid_t input_pid;
    pid_t display_pid;
    int display_fd;
    GameState state;
} ClientSession;

void *thread_move_routine(void *arg);
void *thread_goal_routine(void *arg);
