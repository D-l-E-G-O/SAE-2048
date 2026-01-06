#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h> // Pour PATH_MAX (taille max d'un chemin)

int main(void) {
    char exe_path[PATH_MAX];
    char dir_path[PATH_MAX];

    // 1. Récupérer le chemin absolu de l'exécutable actuel (le launcher lui-même)
    // /proc/self/exe est un lien système qui pointe vers le binaire en cours d'exécution
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

    if (len == -1) {
        perror("[LAUNCHER] Erreur lecture chemin executable");
        return EXIT_FAILURE;
    }
    exe_path[len] = '\0'; // On met le \0 final

    // 2. Extraire le dossier contenant l'exécutable
    // Exemple : si exe_path = "/home/diego/SAE-2048/bin/launcher"
    // On veut obtenir : "/home/diego/SAE-2048/bin"
    strcpy(dir_path, exe_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; // On coupe juste après le dernier slash
    }

    // 3. Construire les commandes avec des chemins absolus
    // On sait que les exécutables cibles sont dans le sous-dossier "core"
    // par rapport au launcher.
    char cmd_game[PATH_MAX + 64];
    char cmd_input[PATH_MAX + 64];

    // Construction dynamique : "CHEMIN_BIN/core/game_2048 &"
    snprintf(cmd_game, sizeof(cmd_game), "%s/core/game_2048 &", dir_path);
    
    // Construction dynamique : "CHEMIN_BIN/core/input"
    snprintf(cmd_input, sizeof(cmd_input), "%s/core/input", dir_path);

    // --- Lancement des processus ---

    printf("[LAUNCHER] Démarrage du moteur 2048 en arrière-plan...\n");
    
    // 1. Lance le jeu en background (Note le '&' à la fin)
    // Le système rend la main tout de suite.
    int ret_game = system(cmd_game);
    
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
    int ret_input = system(cmd_input);

    if (ret_input == -1) {
        perror("Erreur lancement input");
        return EXIT_FAILURE;
    }

    printf("[LAUNCHER] Fin de session.\n");
    return EXIT_SUCCESS;
}