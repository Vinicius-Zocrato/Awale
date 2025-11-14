#ifndef SERVER_MATCH_H
#define SERVER_MATCH_H

#define MAX_CLIENTS 100
#include <stdbool.h>
#include "board.h"

#define ABANDONED -1
#define ONGOING 2
#define PLAYER1_WIN 0
#define PLAYER2_WIN 1

typedef struct Client Client;

typedef struct
{
   int tab[12];
   Client *player1;
   Client *player2;
   Client *observers[MAX_CLIENTS];
   int joueur;
   int pointsP1;
   int pointsP2;
   int private;
   int id;
   int winner; //-1 if abandoned, 0 if player1 wins, 1 if player2 wins, 2 if ongoing
   int scores[2];
   int *moveSequences;
   int moveCount;
   int moveCapacity;
   Board *board;

} ServerMatch;

void matchInit(ServerMatch *m, int id, int playerId1, int playerId2, int sens);
// void matchStoreInCSV(const ServerMatch *m);
void matchDestroy(ServerMatch *m);
bool matchIsGameOver(ServerMatch *m);
void matchMove(ServerMatch *m, int pit);

#endif
