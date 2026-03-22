#define _XOPEN_SOURCE 700
#include "../include/game_threads.h"
#include "../include/game_logic.h"
#include "../include/array_list.h"

// =================================================================
// GLOBALES
// =================================================================

int shm_id = -1;
SharedGameSlot *shm_slot = NULL;

pthread_mutex_t shm_mutex;
pthread_cond_t cond_move;
pthread_cond_t cond_goal;
pthread_cond_t cond_free;

pthread_t main_thread_id;
volatile sig_atomic_t stop_requested = 0;
array_list players;

// =================================================================
// FONCTIONS UTILITAIRES (Helpers)
// =================================================================

/**
 * Fonction pour trouver le chemin de l'exécutable 'display'
 * en se basant sur le chemin de l'exécutable actuel (argv[0])
 * @param buffer Le buffer du chemin.
 * @param size La taille du buffer.
 * @param argv0 Le chemin de l'exécutable actuel.
 */
static void get_display_path(char *buffer, size_t size, const char *argv0)
{
    // 1. On copie le chemin complet de argv[0] (ex: "./bin/game_2048")
    strncpy(buffer, argv0, size - 1);
    buffer[size - 1] = '\0'; // Sécurité

    // 2. On cherche le dernier slash '/'
    char *last_slash = strrchr(buffer, '/');

    if (last_slash != NULL)
    {
        // On coupe juste après le slash (ex: "./bin/")
        *(last_slash + 1) = '\0';
    }
    else
    {
        // Pas de slash ? On est dans le dossier courant (ex: "")
        buffer[0] = '\0';
    }

    // 4. On ajoute "display" (ex: "./bin/display")
    strncat(buffer, "display", size - strlen(buffer) - 1);
}

/**
 * Lance le processus d'affichage via fork/exec.
 * @param write_fd_ptr Pointeur pour récupérer le File Descriptor d'écriture vers l'affichage.
 * @param argv0 Le chemin de l'exécutable actuel.
 * @return PID du processus fils (affichage).
 */
static pid_t spawn_display_process(int *write_fd_ptr, const char *argv0)
{
    int pipe_fd[2];

    // 1. Création du Pipe Anonyme
    if (pipe(pipe_fd) == -1)
    {
        perror("[GAME] Erreur fatal: pipe creation");
        exit(EXIT_FAILURE);
    }

    // 2. Fork du processus
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("[GAME] Erreur fatal: fork");
        exit(EXIT_FAILURE);
    }

    // --- PROCESSUS FILS (Affichage) ---
    if (pid == 0)
    {
        close(pipe_fd[1]); // Ferme écriture

        // Conversion du FD en chaîne pour l'argument
        char fd_str[16];
        snprintf(fd_str, sizeof(fd_str), "%d", pipe_fd[0]);

        // Calcul du chemin
        char path_to_display[256];
        get_display_path(path_to_display, sizeof(path_to_display), argv0);

        // Remplacement de l'image du processus
        execl(path_to_display, "display", fd_str, NULL);

        // Si on arrive ici, execl a échoué
        fprintf(stderr, "[GAME] Erreur: Impossible de lancer %s\n", path_to_display);
        perror("[GAME] Erreur fatal: execl display");
        exit(EXIT_FAILURE);
    }

    // --- PROCESSUS PÈRE (Moteur) ---
    close(pipe_fd[0]);          // Ferme lecture
    *write_fd_ptr = pipe_fd[1]; // On sauve le FD d'écriture pour plus tard

    return pid;
}

/**
 * Initialise le Pipe Nommé pour l'entrée clavier.
 * @return Pointeur FILE* ouvert en lecture binaire.
 */
static FILE *setup_input_pipe()
{
    // Création du FIFO si inexistant
    if (mkfifo(NAMED_PIPE_PATH, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("[GAME] Erreur mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    printf("[GAME] En attente du contrôleur (Input) sur %s...\n", NAMED_PIPE_PATH);
    FILE *fp = fopen(NAMED_PIPE_PATH, "rb");

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

// Handler permettant de terminer le jeu
void game_stop(int const sig)
{
    if (sig == SIG_END_GAME || sig == SIG_CLEAN_EXIT || sig == SIGINT)
    {
        printf("[GAME] Signal d'arrêt reçu (CTRL+C).\n");
        stop_requested = 1;
    }
}

// =================================================================
// MAIN
// =================================================================

int main(int argc, char *argv[])
{
    (void)argc;
    array_list_init(&players, sizeof(ClientSession));

    // 1. Mise en place des handlers
    struct sigaction sa;
    sa.sa_handler = game_stop;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_CLEAN_EXIT, &sa, NULL);
    sigaction(SIG_END_GAME, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // 2. On sauvegarde l'identité du thread main pour que Goal puisse le viser
    main_thread_id = pthread_self();

    printf("[GAME] --- Initialisation du Moteur 2048 ---\n");

    // 3. Creation du segment de mémoire partagé
    shm_id = shmget(IPC_PRIVATE, sizeof(SharedGameSlot), IPC_CREAT | 0666);
    if (shm_id < 0)
    {
        perror("[GAME] shmget error");
        exit(EXIT_FAILURE);
    }

    shm_slot = (SharedGameSlot *)shmat(shm_id, NULL, 0);
    shm_slot->status = SLOT_FREE;

    // 4. Initialisation des Mutex et Cond Vars
    pthread_mutex_init(&shm_mutex, NULL);
    pthread_cond_init(&cond_move, NULL);
    pthread_cond_init(&cond_goal, NULL);
    pthread_cond_init(&cond_free, NULL);

    printf("[GAME] Moteur en marche.\n");

    pthread_t move_thread_id, goal_thread_id;
    pthread_create(&move_thread_id, NULL, thread_move_routine, NULL);
    pthread_create(&goal_thread_id, NULL, thread_goal_routine, NULL);

    FILE *input_stream = setup_input_pipe();
    InputPacket packet;

    // =============================================================
    // BOUCLE PRINCIPALE
    // =============================================================

    while (!stop_requested && fread(&packet, sizeof(InputPacket), 1, input_stream) > 0)
    {

        // Gestion du Handshake
        if (packet.cmd == CMD_HANDSHAKE)
        {
            ClientSession new_session;
            new_session.input_pid = packet.sender_pid;
            new_session.display_pid = spawn_display_process(&new_session.display_fd, argv[0]);
            init_game(&new_session.state);
            write(new_session.display_fd, &new_session.state, sizeof(GameState));
            array_list_push_back(&players, &new_session);

            kill(new_session.input_pid, SIGUSR2); // Le Acknowledge initial
            continue;
        }

        if (packet.cmd == CMD_QUIT)
        {
            continue;
        }

        // Trouver le joueur en question dans le tas
        ClientSession *active_session = NULL;
        for (size_t i = 0; i < players.size; i++)
        {
            ClientSession *s = array_list_get_pointer_mut(&players, i);
            if (s->input_pid == packet.sender_pid)
            {
                active_session = s;
                break;
            }
        }

        if (active_session != NULL)
        {

            // 5. Vérouiller le SHM et attendre qu'il soit dispo
            pthread_mutex_lock(&shm_mutex);

            while (shm_slot->status != SLOT_FREE && !stop_requested)
            {
                pthread_cond_wait(&cond_free, &shm_mutex);
            }

            if (!stop_requested)
            {
                // Tas -> transfère SHM
                shm_slot->state = active_session->state;
                shm_slot->cmd = packet.cmd;
                shm_slot->display_fd = active_session->display_fd;
                shm_slot->input_pid = active_session->input_pid;
                shm_slot->heap_session = active_session; // Liaison au tas

                // Pass control to Move thread
                shm_slot->status = SLOT_TO_MOVE;
                pthread_cond_signal(&cond_move);
            }
            pthread_mutex_unlock(&shm_mutex);
        }
    }

    // --- CLEANUP ---
    printf("[GAME] Nettoyage et fermeture...\n");

    // 1. Forcer l'arrêt des threads
    // Même si on sort de la boucle à cause d'une déconnexion (fread = 0)
    stop_requested = 1;

    // 2. Réveiller tous les threads endormis pour qu'ils voient stop_requested == 1
    pthread_mutex_lock(&shm_mutex);
    pthread_cond_broadcast(&cond_move);
    pthread_cond_broadcast(&cond_goal);
    pthread_cond_broadcast(&cond_free);
    pthread_mutex_unlock(&shm_mutex);

    // 3. Attendre leur mort (qui sera maintenant immédiate)
    pthread_join(move_thread_id, NULL);
    pthread_join(goal_thread_id, NULL);

    // 4. Nettoyage du système
    fclose(input_stream);
    unlink(NAMED_PIPE_PATH);

    // Libération Mutex et SHM
    pthread_mutex_destroy(&shm_mutex);
    pthread_cond_destroy(&cond_move);
    pthread_cond_destroy(&cond_goal);
    pthread_cond_destroy(&cond_free);
    shmdt(shm_slot);
    shmctl(shm_id, IPC_RMID, NULL);

    printf("[GAME] Arrêt complet.\n");
    return EXIT_SUCCESS;
}