#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "src/player.h"
#include "src/board.h"
#include "server_match.h"

int nextPlayerID = 1;
int nextMatchID = 1;

void testBoard()
{
    Board b;
    boardStartGame(&b, 1);
    boardPrint(&b);

    if (boardIsMoveLegal(&b, 2, 0))
    {
        int points = boardMove(&b, 2);
        printf("Points: %d\n", points);
    }

    boardPrint(&b);
}

void testMatch()
{
    ServerMatch m;

    matchInit(&m, nextMatchID, nextPlayerID, nextPlayerID + 1, 1);
    boardPrint(m.board);
}

int main()
{

    // testPlayer();
    // testBoard();
    // testMatch();
    ServerMatch m;

    matchInit(&m, nextMatchID, nextPlayerID, nextPlayerID + 1, 1);
    Board *board = m.board;

    int playingPlayerID = 0;
    while (m.winner == ONGOING)
    {

        boardPrint(board);
        int move;
        scanf("%d", &move);
        // TO DO: fix bug - entering a letter breaks the code
        if (boardIsMoveLegal(board, move, playingPlayerID))
        {
            boardMove(board, move);
            if (playingPlayerID == 0)
            {
                playingPlayerID = 1;
            }
            else if (playingPlayerID == 1)
            {
                playingPlayerID = 0;
            }
        }
    }

    return 0;
}
