#pragma once

#include "../include/common.h"

/**
 * Initialise le tube nommé.
 * Crée le FIFO s'il n'existe pas et l'ouvre en écriture.
 * @param fifo_name Le chemin du tube
 * @return Le descripteur de fichier (fd) du tube ouvert
 */
int init_fifo(const char *fifo_name);

/**
 * Lit une commande depuis l'entrée standard (clavier).
 * Gère les séquences d'échappement (flèches directionnelles).
 * @return La commande détectée (CMD_UP, CMD_QUIT, etc.)
 */
UserCommand read_input_command();