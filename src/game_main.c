#include "../include/game_threads.h"

void *thread_main_routine(void *arg){
    // Caster l'argument passer en paramètre pour récupérer le pid du processus Affichage
    pid_t pid_process_display = *(pid_t *) arg;

    // Instanciation des threads move et goal
    pthread_t thread_move;
    pthread_t thread_goal;
    // Création des threads move et goal
    pthread_create(&thread_move, NULL, thread_move_routine, NULL);
    pthread_create(&thread_goal, NULL, thread_goal_routine, NULL);

    // Initialisation du mutex de syncronisation du InputSharedData entre les threads move et main
    pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
    input_data.mutex = data_mutex;

    while(1){
        int fd = 0;
        int check_read = 0;

        // Ouverture du pipe nommé
        if(fd = open(NAMED_PIPE_PATH, O_CREAT, O_RDONLY) == -1){
            perror("open");
            // En cas d'échec de open, on passe simplement à la prochaine ittération jusqu'à ce que le pipe s'ouvre correctement
            continue;
        }

        InputPacket cmd_received;

        // Vérification de la réussite de la lecture 
        if(check_read = read(fd, &cmd_received, sizeof(InputSharedData)) == -1){
            perror("read");
            continue;
        }

        // Si on ne lit aucune InputSharedData
        if(check_read == 0){
            printf("Aucune donnée lut dans le pipe\n");
            continue;
        }

        pthread_mutex_lock(&input_data.mutex);

        // Remplissage de la structure InputSharedData pour qu'elle puisse être exploité par le thread Move
        input_data.cmd = cmd_received.cmd;
        input_data.has_new_cmd = true;

        // Réveiller le thread move
        pthread_cond_signal(&input_data.cond);

        pthread_mutex_unlock(&input_data.mutex);

        // Unique condition d'arret pour le thread main
        if(cmd_received.cmd == CMD_QUIT){
            // Envoie du signal d'arret au processus Main
            kill(cmd_received.sender_pid, SIG_CLEAN_EXIT);
            // Envoie du signal d'arret au processus Affichage
            kill(pid_process_display, SIG_CLEAN_EXIT);
            // Fin du thread main
            return EXIT_SUCCESS;
        }
    }    

}

void process_2048(pid_t pid_process_display){
    pthread_t thread_main;
    // Création du thread main
    pthread_create(&thread_main, NULL, thread_main_routine, &pid_process_display);

    // Attendre la fin du thread main
    pthread_join(thread_main, NULL);

    // Une fois le thread main terminé, tous les autres processus et thread devraient aussi l'être.
    // Donc c'est qu'il ne reste plus qu'à arreter le processus 2048.
    printf("fin du jeu !!\n");
    return EXIT_SUCCESS;
 
}

void process_display(){
    // A faire ! 
}

int main(int argc, char *argv[]) {
    // création du processus enfant "Affichage"
    pid_t display_process = fork();
    
    if(display_process < 0){ // gestion de l'erreur de fork
        perror("Fork");
        return EXIT_FAILURE;
    }
    if(display_process == 0){ // dans le père (Processus 2048)
        process_2048(display_process);
    }
    else{ // dans le fils (Processus Affichage)
        process_display();
    }
    
}