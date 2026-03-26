# ==========================================
#   Makefile - Projet Système 2048
# ==========================================

# --- Variables de compilation ---
CC       = gcc
CFLAGS   = -Wall -Wextra -g -Iinclude
LDFLAGS  = -pthread

# --- Dossiers ---
SRC_DIR  = src
OBJ_DIR  = obj
BIN_DIR  = bin
CORE_DIR = $(BIN_DIR)/core
INC_DIR  = include

# --- Exécutables à produire ---
TARGET_GAME    = $(BIN_DIR)/game_2048
TARGET_INPUT   = $(BIN_DIR)/input
TARGET_DISPLAY = $(CORE_DIR)/display

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

# ==========================================
#   Règles de Linkage (Création des exécutables)
# ==========================================

# A. Processus Principal (Moteur de jeu + Threads)
GAME_OBJS = $(OBJ_DIR)/game_main.o \
            $(OBJ_DIR)/thread_move.o \
            $(OBJ_DIR)/thread_goal.o \
            $(OBJ_DIR)/game_logic.o \
            $(OBJ_DIR)/utils.o \
            $(OBJ_DIR)/array_list.o

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