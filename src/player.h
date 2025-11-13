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
void playerStoreInCSV(const Player *p);
Player playerLoadPlayerFromCsv(int playerId1);
void playerUpdateInCSV(const Player *p);
void playerAddMatch(Player *p, int matchId);
void playerDestroy(Player *p);

#endif