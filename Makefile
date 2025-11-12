# === Variables ===
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./src
LDFLAGS = 

# Dossiers
SRC_DIR = src
BIN_DIR = bin
SERVER_DIR = server
CLIENT_DIR = client

# Fichiers source
SERVER_SRC = $(SERVER_DIR)/server.c $(SRC_DIR)/board.c $(SRC_DIR)/match.c $(SRC_DIR)/player.c
CLIENT_SRC = $(CLIENT_DIR)/client.c $(SRC_DIR)/board.c $(SRC_DIR)/match.c $(SRC_DIR)/player.c

# Fichiers objets
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Exécutables
SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/client

# === Règles ===
all: $(BIN_DIR) $(SERVER_BIN) $(CLIENT_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilation générique
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(SRC_DIR)/*.o $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o

fclean: clean
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

re: fclean all

.PHONY: all clean fclean re
