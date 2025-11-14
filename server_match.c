#include "src/board.h"
#include "server_match.h"
#include <stdlib.h>
#include <stdio.h>

void matchInit(ServerMatch *m, int id, int playerId1, int playerId2, int sens)
{
    if (m == NULL)
    {
        fprintf(stderr, "ServerMatch pointer is NULL in matchInit.\n");
        return;
    }

    m->id = id;
    m->winner = 2; // ONGOING
    m->scores[0] = 0;
    m->scores[1] = 0;
    m->moveSequences = (int *)malloc(20 * sizeof(int));
    m->moveCapacity = 20;
    m->moveCount = 0;

    m->board = (Board *)malloc(sizeof(Board));
    if (m->board == NULL)
    {
        fprintf(stderr, "Failed to allocate Board in matchInit.\n");
        return;
    }
    boardStartGame(m->board, sens);
    m->joueur = m->board->whoseTurn;
}

void matchMove(ServerMatch *m, int pit)
{
    int playingPlayer = m->board->whoseTurn;
    int points = boardMove(m->board, pit);
    m->scores[playingPlayer] += points;

    if (m->moveCapacity == m->moveCount)
    {
        m->moveCapacity *= 2;
        m->moveSequences = (int *)realloc(m->moveSequences, m->moveCapacity * sizeof(int));
    }

    m->moveSequences[m->moveCount] = pit;
    m->moveCount++;
}

void matchDestroy(ServerMatch *m)
{
    if (m == NULL)
        return;
    if (m->board != NULL)
        free(m->board);
    if (m->moveSequences != NULL)
        free(m->moveSequences);
}

bool matchIsGameOver(ServerMatch *m)
{
    // Your game-over logic here
    return m->winner != 2;
}