TARGET = awale

CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = main.c board.c player.c match.c
OBJ = $(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
