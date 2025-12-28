#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("[LAUNCHER] Démarrage du moteur 2048 en arrière-plan...\n");
    
    // 1. Lance le jeu en background (Note le '&' à la fin)
    // Le système rend la main tout de suite.
    int ret_game = system("./bin/core/game_2048 &");
    
    if (ret_game == -1) {
        perror("Erreur lancement game_2048");
        return EXIT_FAILURE;
    }

    // 2. Petite pause de sécurité (500ms)
    // Pour être sûr que le moteur a eu le temps de créer le FIFO (/tmp/fifo...)
    usleep(500000);

    printf("[LAUNCHER] Démarrage du contrôleur (Input)...\n");

    // 3. Lance l'Input en premier plan (Foreground)
    // Le launcher va "attendre" ici tant que l'input tourne.
    int ret_input = system("./bin/core/input");

    if (ret_input == -1) {
        perror("Erreur lancement input");
        return EXIT_FAILURE;
    }

    printf("[LAUNCHER] Fin de session.\n");
    return EXIT_SUCCESS;
}