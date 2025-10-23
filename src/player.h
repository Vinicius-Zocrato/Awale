#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    int matchCount;
    char username[50];
    int* matchIds;
} Player;


void playerPrint(const Player *p);
void playerInit(Player *p, int id, const char *username);
void playerAddMatch(Player *p, int matchId);
Player playerFindPlayerInCsv(int playerId1);
void playerStoreInCSV(const Player *p);
void playerDestroy(Player *p);

#endif