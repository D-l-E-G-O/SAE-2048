#define _XOPEN_SOURCE 700
#include "../include/game_threads.h"
#include "../include/game_logic.h"

// =================================================================
// GLOBALES (Dynamiques pour N parties)
// =================================================================

int num_slots = 0;
int shm_id = -1;
SharedGameSlot *shm_slots = NULL;

pthread_mutex_t heap_mutex;
pthread_mutex_t *slot_mutexes = NULL;
sem_t *sem_moves = NULL;
sem_t *sem_goals = NULL;
sem_t *sem_frees = NULL;

pthread_t main_thread_id;
volatile sig_atomic_t stop_requested = 0;
array_list players;

// =================================================================
// FONCTIONS UTILITAIRES (Helpers)
// =================================================================

static void get_display_path(char *buffer, size_t size, const char *argv0)
{
    strncpy(buffer, argv0, size - 1);
    buffer[size - 1] = '\0';
    char *last_slash = strrchr(buffer, '/');
    if (last_slash != NULL)
    {
        *(last_slash + 1) = '\0';
    }
    else
    {
        buffer[0] = '\0';
    }
    strncat(buffer, "display", size - strlen(buffer) - 1);
}

static pid_t spawn_display_process(int *write_fd_ptr, const char *argv0)
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("[GAME] Erreur fatal: pipe creation");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("[GAME] Erreur fatal: fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        // 1. On ferme le bout d'écriture du nouveau pipe (celui de ce joueur)
        close(pipe_fd[1]);

        // 2. On ferme tous les bouts d'écriture des anciens joueurs
        // que cet enfant a hérité accidentellement de son père.
        for (size_t i = 0; i < players.size; i++)
        {
            ClientSession const *s = array_list_get_pointer(&players, i);
            if (s->display_fd > 0)
            {
                close(s->display_fd); // On coupe le lien fantôme
            }
        }

        char fd_str[16];
        snprintf(fd_str, sizeof(fd_str), "%d", pipe_fd[0]);
        char path_to_display[256];
        get_display_path(path_to_display, sizeof(path_to_display), argv0);

        execl(path_to_display, "display", fd_str, NULL);

        perror("[GAME] Erreur fatal: execl display");
        exit(EXIT_FAILURE);
    }

    // Processus père (game_main)
    close(pipe_fd[0]);
    *write_fd_ptr = pipe_fd[1];
    return pid;
}

static FILE *setup_input_pipe()
{
    if (mkfifo(NAMED_PIPE_PATH, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("[GAME] Erreur mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    printf("[GAME] En attente du contrôleur (Input) sur %s...\n", NAMED_PIPE_PATH);
    FILE *fp = fopen(NAMED_PIPE_PATH, "r+b");
    if (fp == NULL)
    {
        perror("[GAME] Erreur ouverture pipe nommé");
        exit(EXIT_FAILURE);
    }
    printf("[GAME] Contrôleur connecté.\n");
    return fp;
}

// =================================================================
// HANDLERS
// =================================================================

void game_stop(int const sig)
{
    if (sig == SIG_END_GAME || sig == SIG_CLEAN_EXIT || sig == SIGINT)
    {
        stop_requested = 1;
    }
}

// =================================================================
// MAIN
// =================================================================

int main(int argc, char *argv[])
{
    // L'argument N (nombre de joueurs max) est requis
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <N_players>\n", argv[0]);
        return EXIT_FAILURE;
    }
    num_slots = atoi(argv[1]);
    if (num_slots <= 0)
        num_slots = 1; // Au moins 1 joueur

    array_list_init(&players, sizeof(ClientSession));

    // 1. Mise en place des handlers
    struct sigaction sa;
    sa.sa_handler = game_stop;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_CLEAN_EXIT, &sa, NULL);
    sigaction(SIG_END_GAME, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    main_thread_id = pthread_self();

    printf("[GAME] --- Initialisation du Moteur 2048 (Max %d parties) ---\n", num_slots);

    // 2. Création du segment de mémoire partagé (Dimensionné pour N parties)
    shm_id = shmget(IPC_PRIVATE, num_slots * sizeof(SharedGameSlot), IPC_CREAT | 0666);
    if (shm_id < 0)
    {
        perror("[GAME] shmget error");
        exit(EXIT_FAILURE);
    }

    shm_slots = (SharedGameSlot *)shmat(shm_id, NULL, 0);

    // 3. Initialisation dynamique des Mutex et Threads
    pthread_mutex_init(&heap_mutex, NULL);
    slot_mutexes = malloc(num_slots * sizeof(pthread_mutex_t));
    sem_moves = malloc(num_slots * sizeof(sem_t));
    sem_goals = malloc(num_slots * sizeof(sem_t));
    sem_frees = malloc(num_slots * sizeof(sem_t));

    pthread_t *move_threads = malloc(num_slots * sizeof(pthread_t));
    pthread_t *goal_threads = malloc(num_slots * sizeof(pthread_t));
    int *thread_args = malloc(num_slots * sizeof(int));

    for (int i = 0; i < num_slots; i++)
    {
        shm_slots[i].status = SLOT_FREE;
        pthread_mutex_init(&slot_mutexes[i], NULL);
        sem_init(&sem_moves[i], 0, 0);
        sem_init(&sem_goals[i], 0, 0);
        sem_init(&sem_frees[i], 0, 0);

        thread_args[i] = i; // On passe l'index au thread
        pthread_create(&move_threads[i], NULL, thread_move_routine, &thread_args[i]);
        pthread_create(&goal_threads[i], NULL, thread_goal_routine, &thread_args[i]);
    }

    printf("[GAME] Moteur en marche avec %d paires de threads ouvriers.\n", num_slots);

    FILE *input_stream = setup_input_pipe();
    InputPacket packet;

    // =============================================================
    // BOUCLE PRINCIPALE
    // =============================================================

    while (!stop_requested && fread(&packet, sizeof(InputPacket), 1, input_stream) > 0)
    {
        // Gestion du Handshake : Affectation d'un Slot
        if (packet.cmd == CMD_HANDSHAKE)
        {
            int assigned_slot = -1;

            // Chercher un slot libre
            for (int i = 0; i < num_slots; i++)
            {
                pthread_mutex_lock(&slot_mutexes[i]);
                if (shm_slots[i].status == SLOT_FREE)
                {
                    shm_slots[i].status = SLOT_IDLE; // Réservé pour ce joueur
                    assigned_slot = i;
                    pthread_mutex_unlock(&slot_mutexes[i]);
                    break;
                }
                pthread_mutex_unlock(&slot_mutexes[i]);
            }

            if (assigned_slot == -1)
            {
                printf("[GAME] Serveur plein, connexion refusée pour PID %d\n", packet.sender_pid);
                kill(packet.sender_pid, SIG_CLEAN_EXIT);
                continue;
            }

            ClientSession new_session;
            new_session.input_pid = packet.sender_pid;
            new_session.display_pid = spawn_display_process(&new_session.display_fd, argv[0]);
            new_session.slot_index = assigned_slot; // ASSIGNATION DU SLOT
            init_game(&new_session.state);
            write(new_session.display_fd, &new_session.state, sizeof(GameState));

            pthread_mutex_lock(&heap_mutex);
            array_list_push_back(&players, &new_session);
            pthread_mutex_unlock(&heap_mutex);

            kill(new_session.input_pid, SIGUSR2); // Acknowledge initial
            continue;
        }

        // Gestion de la Déconnexion
        if (packet.cmd == CMD_QUIT)
        {
            pthread_mutex_lock(&heap_mutex);
            for (size_t i = 0; i < players.size; i++)
            {
                ClientSession const *s = array_list_get_pointer(&players, i);
                if (s->input_pid == packet.sender_pid)
                {
                    // Libérer le slot dans la SHM
                    pthread_mutex_lock(&slot_mutexes[s->slot_index]);
                    shm_slots[s->slot_index].status = SLOT_FREE;
                    pthread_mutex_unlock(&slot_mutexes[s->slot_index]);

                    close(s->display_fd);
                    waitpid(s->display_pid, NULL, 0);
                    array_list_erase(&players, i);
                    printf("[GAME] Joueur %d déconnecté.\n", packet.sender_pid);
                    break;
                }
            }
            pthread_mutex_unlock(&heap_mutex);
            continue;
        }

        // --- Action en cours de partie ---
        // Trouver le joueur en question dans le tas
        ClientSession *active_session = NULL;
        pthread_mutex_lock(&heap_mutex);
        for (size_t i = 0; i < players.size; i++)
        {
            ClientSession *s = array_list_get_pointer_mut(&players, i);
            if (s->input_pid == packet.sender_pid)
            {
                active_session = s;
                break;
            }
        }

        GameState const temp_state = active_session ? active_session->state : (GameState){0};
        int const temp_fd = active_session ? active_session->display_fd : -1;
        int const s_idx = active_session ? active_session->slot_index : -1;
        pthread_mutex_unlock(&heap_mutex);

        if (active_session != NULL && s_idx != -1)
        {
            // On verrouille UNIQUEMENT le mutex de CE joueur
            pthread_mutex_lock(&slot_mutexes[s_idx]);

            while (shm_slots[s_idx].status != SLOT_IDLE && !stop_requested)
            {
                pthread_mutex_unlock(&slot_mutexes[s_idx]);
                sem_wait(&sem_frees[s_idx]);
                pthread_mutex_lock(&slot_mutexes[s_idx]);
            }

            if (!stop_requested)
            {
                // Tas -> transfère SHM
                shm_slots[s_idx].state = temp_state;
                shm_slots[s_idx].cmd = packet.cmd;
                shm_slots[s_idx].display_fd = temp_fd;
                shm_slots[s_idx].input_pid = packet.sender_pid;

                // Passer le contrôle au thread Move
                shm_slots[s_idx].status = SLOT_TO_MOVE;
                sem_post(&sem_moves[s_idx]);
            }
            pthread_mutex_unlock(&slot_mutexes[s_idx]);
        }
    }

    // --- CLEANUP ---
    stop_requested = 1;

    // 1. Réveiller tous les threads de tous les slots
    for (int i = 0; i < num_slots; i++)
    {
        sem_post(&sem_moves[i]);
        sem_post(&sem_goals[i]);
        sem_post(&sem_frees[i]);

        pthread_join(move_threads[i], NULL);
        pthread_join(goal_threads[i], NULL);

        pthread_mutex_destroy(&slot_mutexes[i]);
        sem_destroy(&sem_moves[i]);
        sem_destroy(&sem_goals[i]);
        sem_destroy(&sem_frees[i]);
    }

    // 2. Nettoyage du système IPC
    fclose(input_stream);
    unlink(NAMED_PIPE_PATH);
    shmdt(shm_slots);
    shmctl(shm_id, IPC_RMID, NULL);

    // 3. Prévenir tous les clients que le serveur ferme
    pthread_mutex_lock(&heap_mutex);
    for (size_t i = 0; i < players.size; i++)
    {
        ClientSession const *s = array_list_get_pointer(&players, i);
        kill(s->input_pid, SIG_CLEAN_EXIT);
        close(s->display_fd);
    }
    pthread_mutex_unlock(&heap_mutex);

    // 4. Nettoyage mémoire
    array_list_deinit(&players);
    free(slot_mutexes);
    free(sem_moves);
    free(sem_goals);
    free(sem_frees);
    free(move_threads);
    free(goal_threads);
    free(thread_args);

    return EXIT_SUCCESS;
}