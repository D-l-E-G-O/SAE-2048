#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "../include/utils.h"
#include "../include/common.h"
#include  "../include/input_process.h"


/**
 * Initialise le tube nommé (création et ouverture).
 */
int init_fifo(const char *fifo_name) {
    // Tentative de création du tube avec les permissions 0666 (rw-rw-rw-)
    if (mkfifo(fifo_name, 0666) == -1) {
        // Si l'erreur est "Le fichier existe déjà", ce n'est pas grave, on continue.
        if (errno != EEXIST) {
            perror("error while creating the fifo");
            exit(EXIT_FAILURE); // On quitte si c'est une vraie erreur critique
        }
    }

    // Ouverture du tube en écriture seule (Write Only).
    // Cette ligne est bloquante tant qu'un autre processus n'ouvre pas le tube en lecture.
    return open(fifo_name, O_WRONLY);
}

/**
 * Lit l'entrée clavier et retourne la commande correspondante.
 * Gère les séquences d'échappement pour les flèches.
 */
UserCommand read_input_command() {
    // Lecture d'un caractère sur l'entrée standard
    int c = getchar(); // 1er appel : On récupère le code 27 (ESC)

    // Détection des séquences d'échappement (Flèches directionnelles)
    // Les flèches envoient 3 octets : ESC (27) + '[' (91) + Lettre (A, B, C ou D)
    if (c == 27) {
        // On sait que c'est une séquence spéciale.
        // Le caractère suivant dans la file d'attente est forcément '['
        
        getchar(); // 2ème appel : On se debarrasse du '['
        switch(getchar()) { // 3ème appel : On récupère la lettre finale (A, B, C ou D)
            case 'A': return CMD_UP;    // Flèche Haut
            case 'B': return CMD_DOWN;  // Flèche Bas
            case 'C': return CMD_RIGHT; // Flèche Droite
            case 'D': return CMD_LEFT;  // Flèche Gauche
        }
    } else if (c == 'q') {
        // Si l'utilisateur appuie sur 'q', on prépare la commande de sortie
        return CMD_QUIT;
    }

    return CMD_NONE;
}

int main(void)
{
    // Définition du nom du tube nommé (FIFO)
    char *my_fifo = "fifo";

    // Initialisation du FIFO (Création + Ouverture)
    int fd = init_fifo(my_fifo);

    // Préparation du paquet d'initialisation (Handshake)
    // Cela permet au serveur de savoir quel processus (PID) vient de se connecter.
    InputPacket pkt;
    pkt.cmd = CMD_HANDSHAKE;
    pkt.sender_pid = getpid(); // Récupère l'ID du processus actuel

    // Envoi du paquet de handshake dans le tube
    write(fd, &pkt, sizeof(InputPacket));

    // Passage du terminal en mode "raw" (brut).
    // Cela permet de lire les touches (comme les flèches) sans attendre que l'utilisateur appuie sur "Entrée".
    // On sauvegarde la configuration d'origine pour la restaurer à la fin.
    struct termios orig_termios = set_raw_mode();

    int running = 1;
    while (running) {
        // Récupération de la commande via notre fonction dédiée
        int command = read_input_command();

        // Réinitialisation des champs du paquet pour éviter d'envoyer d'anciennes données
        pkt.sender_pid = 0;
        pkt.cmd = command;

        if (command == CMD_QUIT) {
            running = 0; // On brisera la boucle while après l'envoi
        }

        // Si une commande valide a été détectée (Flèche ou Quit), on l'envoie dans le tube
        if (pkt.cmd != CMD_NONE) {
            write(fd, &pkt, sizeof(InputPacket));
        }
    }

    // Fermeture du descripteur de fichier du tube
    close(fd);

    // Restauration du terminal dans son état d'origine (mode canonique).
    // C'est crucial, sinon le terminal restera inutilisable (pas d'écho des touches, pas de retour à la ligne automatique) après la fin du programme.
    restore_mode(orig_termios);
    
    return 0;
}