#define _XOPEN_SOURCE 700
#include "../include/game_threads.h"
#include "../include/game_logic.h"

// =================================================================
// GLOBALES ET SYNCHRONISATION
// =================================================================

GameState current_state;

pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond_move   = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cond_goal   = PTHREAD_COND_INITIALIZER;

InputSharedData input_data = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond  = PTHREAD_COND_INITIALIZER,
    .has_new_cmd = false,
    .cmd = CMD_NONE
};

pthread_t main_thread_id;

// Globale pour signaler l'arrêt demandé
volatile sig_atomic_t stop_requested = 0;

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
static void get_display_path(char *buffer, size_t size, const char *argv0) {
    // 1. On copie le chemin complet de argv[0] (ex: "./bin/game_2048")
    strncpy(buffer, argv0, size - 1);
    buffer[size - 1] = '\0'; // Sécurité

    // 2. On cherche le dernier slash '/'
    char *last_slash = strrchr(buffer, '/');
    
    if (last_slash != NULL) {
        // On coupe juste après le slash (ex: "./bin/")
        *(last_slash + 1) = '\0'; 
    } else {
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
static pid_t spawn_display_process(int *write_fd_ptr, const char *argv0) {
    int pipe_fd[2];

    // 1. Création du Pipe Anonyme
    if (pipe(pipe_fd) == -1) {
        perror("[GAME] Erreur fatal: pipe creation");
        exit(EXIT_FAILURE);
    }

    // 2. Fork du processus
    pid_t pid = fork();
    if (pid < 0) {
        perror("[GAME] Erreur fatal: fork");
        exit(EXIT_FAILURE);
    }

    // --- PROCESSUS FILS (Affichage) ---
    if (pid == 0) {
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
    close(pipe_fd[0]); // Ferme lecture
    *write_fd_ptr = pipe_fd[1]; // On sauve le FD d'écriture pour plus tard
    
    return pid;
}

/**
 * Initialise le Pipe Nommé pour l'entrée clavier.
 * @return Pointeur FILE* ouvert en lecture binaire.
 */
static FILE* setup_input_pipe() {
    // Création du FIFO si inexistant
    if (mkfifo(NAMED_PIPE_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            perror("[GAME] Erreur mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    printf("[GAME] En attente du contrôleur (Input) sur %s...\n", NAMED_PIPE_PATH);
    FILE *fp = fopen(NAMED_PIPE_PATH, "rb");
    
    if (fp == NULL) {
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
void stop_game(int sig) {
    if (sig == SIG_END_GAME) { // Fin du jeu standard (pas spontanée)
        sleep(1); // Pour laisser le temps à l'affichage
    }
    printf("[GAME] Signal d'arrêt reçu.\n");
    // On lève le drapeau pour dire à la boucle principale de s'arrêter
    stop_requested = 1;
}

// =================================================================
// MAIN
// =================================================================

int main(int argc, char *argv[]) {
    (void)argc; // On ignore argc pour éviter le warning unused

    // 1. Mise en place des handlers
    struct sigaction sa;
    sa.sa_handler = stop_game;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_CLEAN_EXIT, &sa, NULL);
    sigaction(SIG_END_GAME, &sa, NULL);

    // 2. On sauvegarde l'identité du thread main pour que Goal puisse le viser
    main_thread_id = pthread_self();

    printf("[GAME] --- Initialisation du Moteur 2048 ---\n");

    // 3. Lancement du sous-système d'affichage
    int display_pipe_fd;
    pid_t pid_display = spawn_display_process(&display_pipe_fd, argv[0]);

    // 4. Initialisation logique du jeu
    init_game(&current_state);
    
    // Envoi de l'état initial (évite l'écran noir au démarrage)
    if (write(display_pipe_fd, &current_state, sizeof(GameState)) == -1) {
        perror("[GAME] Erreur envoi initial");
    }

    // 5. Démarrage des Threads "Ouvriers"
    pthread_t t_move, t_goal;
    
    if (pthread_create(&t_move, NULL, thread_move_routine, NULL) != 0) {
        perror("[GAME] Erreur create thread move"); exit(EXIT_FAILURE);
    }
    
    // On passe display_pipe_fd pour que le Goal puisse rafraichir l'écran
    if (pthread_create(&t_goal, NULL, thread_goal_routine, &display_pipe_fd) != 0) {
        perror("[GAME] Erreur create thread goal"); exit(EXIT_FAILURE);
    }

    // 6. Connexion au contrôleur (Bloquant jusqu'à lancement de ./bin/input)
    FILE *input_stream = setup_input_pipe();

    // =============================================================
    // BOUCLE PRINCIPALE
    // =============================================================
    InputPacket packet;

    pid_t input_process_pid;

    while (!stop_requested && fread(&packet, sizeof(InputPacket), 1, input_stream) > 0) {
        
        // Gestion du Handshake
        if (packet.cmd == CMD_HANDSHAKE) {
            input_process_pid = packet.sender_pid;
            continue; 
        }

        // Gestion de l'arrêt
        if (packet.cmd == CMD_QUIT) {
            printf("[GAME] Signal d'arrêt reçu.\n");
            break;
        }

        // Transmission de la commande au thread Move
        pthread_mutex_lock(&input_data.mutex);
        
        input_data.cmd = packet.cmd;
        input_data.has_new_cmd = true;
        
        pthread_cond_signal(&input_data.cond);
        pthread_mutex_unlock(&input_data.mutex);
    }

    // =============================================================
    // NETTOYAGE
    // =============================================================

    // On tue Processus Affichage (avec SIG_CLEAN_EXIT car le processus Affichage a un handler)
    kill(pid_display, SIG_CLEAN_EXIT); 

    // On attend la mort du Processus Affichage
    waitpid(pid_display, NULL, 0);

    // On tue le Processus Input (avec SIG_END_GAME car le processus Input a un handler)
    if (input_process_pid) {
        kill(input_process_pid, SIG_END_GAME);
    }

    printf("[GAME] Arrêt du système.\n");
    
    fclose(input_stream);
    close(display_pipe_fd); // Cela provoquera EOF côté Display

    // Supprimer le fichier pipe nommé du disque
    unlink(NAMED_PIPE_PATH); 

    return EXIT_SUCCESS;
}