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

// =================================================================
// FONCTIONS UTILITAIRES (Helpers)
// =================================================================

/**
 * Lance le processus d'affichage via fork/exec.
 * @param write_fd_ptr Pointeur pour récupérer le File Descriptor d'écriture vers l'affichage.
 * @return PID du processus fils (affichage).
 */
static pid_t spawn_display_process(int *write_fd_ptr) {
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

        // Remplacement de l'image du processus
        execl("./bin/display", "display", fd_str, NULL);
        
        // Si on arrive ici, execl a échoué
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
// MAIN
// =================================================================

int main(void) {
    printf("[GAME] --- Initialisation du Moteur 2048 ---\n");

    // 1. Lancement du sous-système d'affichage
    int display_pipe_fd;
    pid_t pid_display = spawn_display_process(&display_pipe_fd);

    // 2. Initialisation logique du jeu
    init_game(&current_state);
    
    // Envoi de l'état initial (évite l'écran noir au démarrage)
    if (write(display_pipe_fd, &current_state, sizeof(GameState)) == -1) {
        perror("[GAME] Erreur envoi initial");
    }

    // 3. Démarrage des Threads "Ouvriers"
    pthread_t t_move, t_goal;
    
    // Note: On passe display_pipe_fd pour que le Goal puisse rafraichir l'écran
    if (pthread_create(&t_move, NULL, thread_move_routine, NULL) != 0) {
        perror("[GAME] Erreur create thread move"); exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&t_goal, NULL, thread_goal_routine, &display_pipe_fd) != 0) {
        perror("[GAME] Erreur create thread goal"); exit(EXIT_FAILURE);
    }

    // 4. Connexion au contrôleur (Bloquant jusqu'à lancement de ./bin/input)
    FILE *input_stream = setup_input_pipe();

    // =============================================================
    // BOUCLE PRINCIPALE
    // =============================================================
    InputPacket packet;

    while (fread(&packet, sizeof(InputPacket), 1, input_stream) > 0) {
        
        // Gestion du Handshake
        if (packet.cmd == CMD_HANDSHAKE) {
            continue; 
        }

        // Gestion de l'arrêt
        if (packet.cmd == CMD_QUIT) {
            printf("[GAME] Signal d'arrêt reçu.\n");
            
            // On tue l'affichage proprement avec SIGTERM
            kill(pid_display, SIGTERM); 
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
    printf("[GAME] Arrêt du système.\n");
    
    fclose(input_stream);
    close(display_pipe_fd); // Cela provoquera EOF côté Display

    // Supprimer le fichier pipe nommé du disque
    unlink(NAMED_PIPE_PATH); 

    return EXIT_SUCCESS;
}