# ==========================================
#   Makefile - Projet Système 2048
# ==========================================

# --- Variables de compilation ---
CC       = gcc
CFLAGS   = -Wall -Wextra -g -Iinclude
# [CORRECTION] Ajout de -lncurses pour l'interface et -lm pour les maths si besoin
LDFLAGS  = -pthread -lncurses -lm

# --- Dossiers ---
SRC_DIR  = src
OBJ_DIR  = obj
BIN_DIR  = bin
CORE_DIR = $(BIN_DIR)/core
INC_DIR  = include

# --- Exécutables à produire ---
TARGET_GAME    = $(CORE_DIR)/game_2048
TARGET_INPUT   = $(CORE_DIR)/input
TARGET_DISPLAY = $(CORE_DIR)/display

# Le Launcher sera à la racine de bin/
TARGET_LAUNCHER= $(BIN_DIR)/launcher

# Liste de tous les exécutables pour la règle 'all'
# [CORRECTION] Ajout du launcher dans la liste globale
TARGETS = $(TARGET_GAME) $(TARGET_INPUT) $(TARGET_DISPLAY) $(TARGET_LAUNCHER)

# ==========================================
#   Règles Principales
# ==========================================

# 1. Règle par défaut
all: directories $(TARGETS)

# 2. Création des répertoires
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(CORE_DIR)

# [AJOUT] Règle pour lancer le jeu facilement
run: all
	@echo "Lancement du jeu via le launcher..."
	./$(TARGET_LAUNCHER)


# ==========================================
#   Règles de Linkage (Création des exécutables)
# ==========================================

# A. Processus Principal (Moteur de jeu + Threads)
GAME_OBJS = $(OBJ_DIR)/game_main.o \
            $(OBJ_DIR)/thread_move.o \
            $(OBJ_DIR)/thread_goal.o \
            $(OBJ_DIR)/game_logic.o \
            $(OBJ_DIR)/utils.o

$(TARGET_GAME): $(GAME_OBJS)
	@echo "Linking Game Engine..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# B. Processus Input (Gestion clavier)
$(TARGET_INPUT): $(OBJ_DIR)/input_process.o $(OBJ_DIR)/utils.o
	@echo "Linking Input Controller..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# C. Processus Affichage (Rendu graphique)
$(TARGET_DISPLAY): $(OBJ_DIR)/display_process.o $(OBJ_DIR)/utils.o
	@echo "Linking Display System..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# D. Launcher (Le processus père qui lance les autres)
$(TARGET_LAUNCHER): $(OBJ_DIR)/launcher.o $(OBJ_DIR)/utils.o
	@echo "Linking Launcher..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ==========================================
#   Règles de Compilation (Source -> Objet)
# ==========================================

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# ==========================================
#   Nettoyage
# ==========================================

clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	# Nettoyage des fichiers FIFO/Pipes nommés potentiels
	rm -f /tmp/fifo_2048_*

# Pour éviter les conflits
.PHONY: all clean directories run valgrind