CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./src -I. -I./server
LDFLAGS =

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
SERVER_DIR = server
CLIENT_DIR = client

# === Server / Client (full) ===
SERVER_SRC = $(SERVER_DIR)/server.c $(SERVER_DIR)/client_storage.c server_match.c board.c
CLIENT_SRC = $(CLIENT_DIR)/client.c $(SERVER_DIR)/client_storage.c server_match.c board.c
SERVER_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SERVER_SRC))
CLIENT_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(CLIENT_SRC))

.PHONY: all server client clean fclean re

# Default: build server + client
all: $(BIN_DIR) server client

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

server: $(SERVER_OBJS)
	@echo "🔧 Linking server..."
	$(CC) $(SERVER_OBJS) -o $(BIN_DIR)/server $(LDFLAGS)

client: $(CLIENT_OBJS)
	@echo "🔧 Linking client..."
	$(CC) $(CLIENT_OBJS) -o $(BIN_DIR)/client $(LDFLAGS)

# Generic .c -> obj/.o rule
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "🧩 Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure obj subfolders exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/$(SRC_DIR) $(OBJ_DIR)/$(SERVER_DIR) $(OBJ_DIR)/$(CLIENT_DIR)

clean:
	@echo "🧹 Cleaning object files..."
	rm -rf $(OBJ_DIR)

fclean: clean
	@echo "🧽 Removing executables..."
	rm -f $(BIN_DIR)/server $(BIN_DIR)/client

re: fclean all