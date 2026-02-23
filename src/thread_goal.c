#define _XOPEN_SOURCE 700
#include "../include/game_threads.h"
#include "../include/utils.h"

// --- Fonctions utilitaires ---

// Vérifie si la tuile 2048 est présente
int check_victory(int cells[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (cells[i][j] == TARGET_VAL)
                return 1;
        }
    }
    return 0;
}

// Vérifie si le joueur a perdu (Grille pleine ET aucun mouvement possible)
int check_defeat(int cells[GRID_SIZE][GRID_SIZE])
{
    // 1. Si une case est vide, on n'a pas perdu
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (cells[i][j] == 0)
                return 0;
        }
    }

    // 2. Si des fusions sont possibles, on n'a pas perdu
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            // Test droite (sauf dernière colonne)
            if (j < GRID_SIZE - 1 && cells[i][j] == cells[i][j + 1])
                return 0;
            // Test bas (sauf dernière ligne)
            if (i < GRID_SIZE - 1 && cells[i][j] == cells[i + 1][j])
                return 0;
        }
    }

    // Sinon, c'est perdu
    return 1;
}

// --- Routine du Thread Goal ---

void *thread_goal_routine(void *arg)
{
    // Récupération de l'argument
    int const display_fd = *((int *)arg);

    while (!stop_requested)
    {

        // ACTIVE WAITING: Loop until the grid is modified
        while (!grid_has_changed && !stop_requested)
        {
            usleep(1000);
        }

        if (stop_requested)
            break;

        grid_has_changed = false;

        if (check_victory(current_state.cells))
        {
            current_state.victory = true;
            current_state.game_over = true;
        }
        else if (check_defeat(current_state.cells))
        {
            current_state.victory = false;
            current_state.game_over = true;
        }

        if (write(display_fd, &current_state, sizeof(GameState)) == -1)
        {
            perror("[GOAL] error Pipe write");
        }

        if (current_state.game_over)
        {
            pthread_kill(main_thread_id, SIG_END_GAME);
            break;
        }
    }

    pthread_exit(NULL);
}