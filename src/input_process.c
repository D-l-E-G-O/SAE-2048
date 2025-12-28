#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // read, write, close, sleep
#include <fcntl.h>      // open, O_WRONLY
#include <sys/stat.h>   // mkfifo
#include <errno.h>
#include <termios.h>
#include <signal.h>

#include "../include/utils.h"
#include "../include/common.h"

// Globale pour signaler l'arrêt demandé
volatile sig_atomic_t stop_requested = 0;

// =================================================================
// FONCTIONS UTILITAIRES
// =================================================================

/**
 * Tente d'ouvrir le Pipe Nommé pour communiquer avec le moteur.
 * Bloque jusqu'à ce que le moteur ouvre le pipe en lecture.
 * @return Le descripteur de fichier du pipe.
 */
static int connect_to_game_engine() {
    printf("[INPUT] Tentative de connexion au moteur sur '%s'...\n", NAMED_PIPE_PATH);

    // On essaie de créer le pipe au cas où (sécurité)
    if (mkfifo(NAMED_PIPE_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            perror("[INPUT] Warning mkfifo");
            // On continue quand même, le pipe existe peut-être déjà
        }
    }

    // Ouverture bloquante (attente du serveur)
    int fd = open(NAMED_PIPE_PATH, O_WRONLY);
    if (fd == -1) {
        perror("[INPUT] Erreur fatale ouverture pipe");
        exit(EXIT_FAILURE);
    }

    printf("[INPUT] Connecté au moteur de jeu !\n");
    return fd;
}

/**
 * Lit une touche brute depuis le terminal.
 * Gère les séquences d'échappement ANSI pour les flèches.
 * @return UserCommand récupérée et interprétée.
 */
static UserCommand get_user_command() {
    char c;
    // Lecture d'un seul octet
    if (read(STDIN_FILENO, &c, 1) <= 0) return CMD_NONE;

    if (c == 'q') return CMD_QUIT;

    // Détection Séquence Échappement (Flèches : ESC + [ + Lettre)
    if (c == 27) { 
        char seq[2];
        // On tente de lire les 2 caractères suivants
        if (read(STDIN_FILENO, &seq, 2) == 2) {
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return CMD_UP;
                    case 'B': return CMD_DOWN;
                    case 'C': return CMD_RIGHT;
                    case 'D': return CMD_LEFT;
                }
            }
        }
    }

    return CMD_NONE;
}

// =================================================================
// HANDLERS
// =================================================================

// Handler permettant de terminer le jeu
void stop_game(int sig) {
    printf("[INPUT] Signal d'arrêt reçu.\n");
    // On lève le drapeau pour dire à la boucle principale de s'arrêter 
    if (sig == SIG_CLEAN_EXIT) { // CTRL+C ou terminaison en cours de partie
        stop_requested = 1;
    }
    else if (sig == SIG_END_GAME) { // Fin du jeu
        stop_requested = 2;
    }
}

// =================================================================
// MAIN
// =================================================================

int main(void) {
    // 1. Mise en place des handlers
    struct sigaction sa;
    sa.sa_handler = stop_game;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_CLEAN_EXIT, &sa, NULL);
    sigaction(SIG_END_GAME, &sa, NULL);

    // 2. Connexion au Pipe (Communication)
    int pipe_fd = connect_to_game_engine();

    // 3. Handshake (Présentation)
    InputPacket packet;
    packet.cmd = CMD_HANDSHAKE;
    packet.sender_pid = getpid();
    
    if (write(pipe_fd, &packet, sizeof(InputPacket)) == -1) {
        perror("[INPUT] Erreur handshake");
        close(pipe_fd);
        return EXIT_FAILURE;
    }

    // 4. Configuration du Terminal (Mode Raw)
    // Indispensable pour capter les touches sans "Entrée"
    printf("[INPUT] Contrôleur prêt. Utilisez les flèches. 'q' pour quitter.\n");
    struct termios orig_termios = set_raw_mode();

    // 5. Boucle d'événements
    while (true) {
        UserCommand cmd = get_user_command();

        if (stop_requested) {
            cmd = CMD_QUIT;
        }

        if (cmd != CMD_NONE) {
            packet.cmd = cmd;
            packet.sender_pid = 0; // Plus besoin d'envoyer le PID

            // Envoi au moteur (Si le mode de fermeture n'est pas à 1 -> Jeu déjà fini)
            if (stop_requested != 2) {
                if (write(pipe_fd, &packet, sizeof(InputPacket)) == -1) {
                    // Si write échoue (ex: Broken Pipe), le jeu a crashé ou fermé
                    break;
                } 
            }

            if (cmd == CMD_QUIT) {
                break;
            }
        }
    }

    // 6. Nettoyage
    close(pipe_fd);
    restore_mode(orig_termios); // Restaurer le terminal
    printf("[INPUT] Déconnexion.\n");

    return EXIT_SUCCESS;
}