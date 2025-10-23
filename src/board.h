#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int pits[12];
    int sens;
    int whoseTurn;
} Board;


void boardStartGame(Board *b, int direction);
bool boardIsMoveLegal(const Board *b, int pit, int playerMatchId);
int boardMove(Board *b, int pit);
void boardPrint(const Board *b);
bool boardIsGameOver(const Board *b);
void boardDestroy(Board *b);

#endif