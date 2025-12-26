# ==========================================
#   Makefile - Projet Système 2048
# ==========================================

# --- Variables de compilation ---
CC       = gcc
CFLAGS   = -Wall -Wextra -g -Iinclude  # -g pour le debug (gdb/valgrind)
LDFLAGS  = -pthread                    # Nécessaire pour les threads POSIX

# --- Dossiers ---
SRC_DIR  = src
OBJ_DIR  = obj
BIN_DIR  = bin
INC_DIR  = include

# --- Exécutables à produire ---
TARGET_GAME    = $(BIN_DIR)/game_2048
TARGET_INPUT   = $(BIN_DIR)/input
TARGET_DISPLAY = $(BIN_DIR)/display

# Liste de tous les exécutables pour la règle 'all'
TARGETS = $(TARGET_GAME) $(TARGET_INPUT) $(TARGET_DISPLAY)

# ==========================================
#   Règles Principales
# ==========================================

# 1. Règle par défaut : construit tout
all: directories $(TARGETS)

# 2. Création des répertoires de build s'ils n'existent pas
directories:
  @mkdir -p $(OBJ_DIR)
  @mkdir -p $(BIN_DIR)

# ==========================================
#   Règles de Linkage (Création des exécutables)
# ==========================================

# A. Processus Principal (Moteur de jeu + Threads)
# Dépendances : game_process, game_logic, utils
$(TARGET_GAME): $(OBJ_DIR)/game_process.o $(OBJ_DIR)/game_logic.o $(OBJ_DIR)/utils.o
	@echo "Linking Game Engine..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# B. Processus Input (Gestion clavier)
# Dépendances : input_process, utils
$(TARGET_INPUT): $(OBJ_DIR)/input_process.o $(OBJ_DIR)/utils.o
	@echo "Linking Input Controller..."
	$(CC) $(CFLAGS) -o $@ $^

# C. Processus Affichage (Rendu graphique)
# Dépendances : display_process, utils
$(TARGET_DISPLAY): $(OBJ_DIR)/display_process.o $(OBJ_DIR)/utils.o
	@echo "Linking Display System..."
	$(CC) $(CFLAGS) -o $@ $^

# ==========================================
#   Règles de Compilation (Source -> Objet)
# ==========================================

# Règle générique : tout .c dans src/ devient un .o dans obj/
# On ajoute une dépendance sur les headers (*.h) pour recompiler si on change une structure
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# ==========================================
#   Nettoyage
# ==========================================

# Supprime les fichiers compilés
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -f /tmp/fifo_2048_input  # Nettoyage optionnel du pipe nommé

# Pour éviter les conflits avec des fichiers qui s'appelleraient 'clean' ou 'all'
.PHONY: all clean directories
