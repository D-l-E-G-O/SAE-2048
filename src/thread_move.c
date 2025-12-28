#include <stdio.h>
#include <pthread.h>
#include "../include/game_threads.h"
#include "../include/game_logic.h"

/**
 * Routine du Thread "Move & Score"
 * Rôle : 
 * 1. Attendre une commande de direction venant du Thread Principal.
 * 2. Verrouiller l'état du jeu.
 * 3. Appliquer le mouvement (via game_logic).
 * 4. Si le mouvement est valide, ajouter une tuile.
 * 5. Réveiller le Thread Goal pour vérification.
 */
void *thread_move_routine(void *arg) {
    (void)arg; // Pour éviter le warning "unused parameter"

    while (1) {
        // ---------------------------------------------------------
        // ÉTAPE 1 : ATTENTE DU SIGNAL (Consommateur de Input)
        // ---------------------------------------------------------
        
        // On verrouille le mutex des données d'entrée
        pthread_mutex_lock(&input_data.mutex);

        // Boucle d'attente (protège contre les réveils spontanés "spurious wakeups")
        while (!input_data.has_new_cmd) {
            pthread_cond_wait(&input_data.cond, &input_data.mutex);
        }

        // On a reçu un signal ! On récupère la commande.
        UserCommand cmd = input_data.cmd;
        input_data.has_new_cmd = false; // On acquitte la commande (reset)

        // On libère le mutex d'input tout de suite pour ne pas bloquer le Main
        pthread_mutex_unlock(&input_data.mutex);

        // ---------------------------------------------------------
        // ÉTAPE 2 : TRAITEMENT DE LA COMMANDE
        // ---------------------------------------------------------

        if (cmd == CMD_QUIT) {
            // Si on demande de quitter, on arrête ce thread proprement
            break; 
        }

        // On vérifie si c'est bien une commande de mouvement
        if (cmd >= CMD_UP && cmd <= CMD_RIGHT) {
            
            // -----------------------------------------------------
            // ÉTAPE 3 : SECTION CRITIQUE (Modification du Jeu)
            // -----------------------------------------------------
            pthread_mutex_lock(&state_mutex);

            // On appelle la logique pure (qui retourne true si la grille a bougé)
            bool moved = move_grid(&current_state, cmd);

            if (moved) {
                // Règle du jeu : Une tuile apparait seulement après un coup valide
                spawn_tile(&current_state);

                // Debug optionnel pour voir ce qui se passe dans la console du moteur
                printf("[Move Thread] Mouvement appliqué (%d). Score: %d\n", cmd, current_state.score);

                // -------------------------------------------------
                // ÉTAPE 4 : PASSER LE RELAIS AU THREAD GOAL
                // -------------------------------------------------
                // On signale à l'arbitre (Goal) qu'il faut vérifier l'état
                pthread_cond_signal(&cond_goal);
            } else {
                // Si le mouvement est impossible (bloqué dans cette direction),
                // on ne fait rien, on ne spawn pas de tuile, on n'appelle pas Goal.
                printf("[Move Thread] Mouvement impossible !\n");
            }

            pthread_mutex_unlock(&state_mutex);
        }
    }

    printf("[Move Thread] Fin du thread.\n");
    return NULL;
}