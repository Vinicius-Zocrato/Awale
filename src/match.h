// #ifndef MATCH_H
// #define MATCH_H

// #include <stdbool.h>
// #include <string.h>
// #include <stdio.h>
// #include <stdlib.h>

// #include "board.h"
// #include "player.h"

// #define ABANDONED -1
// #define ONGOING 2
// #define PLAYER1_WIN 0
// #define PLAYER2_WIN 1

// typedef struct
// {
//     int id;
//     bool finished;
//     int playersID[2]; // player in position 0 is player 1, position 1 is player 2
//     Player players[2];
//     // We are storing players by value during the match, changes to player conditions wont be reflected during the match

//     int winner; //-1 if abandoned, 0 if player1 wins, 1 if player2 wins, 2 if ongoing
//     int scores[2];
//     int *moveSequences;
//     int moveCount;
//     int moveCapacity;
//     Board *board;
// } Match;

// void matchInit(Match *m, int id, int playerId1, int playerId2, int sens);
// void matchEnd(Match *m);
// void matchPrint(const Match *m);
// Player matchFindPlayerInCsv(int playerId1, int playerId2);
// void matchStoreInCSV(const Match *m);
// void matchDestroy(Match *m);
// bool matchIsGameOver(Match *m);
// void matchMove(Match *m, int pit);

// #endif
