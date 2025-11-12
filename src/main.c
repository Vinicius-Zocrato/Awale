#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "player.h"
#include "board.h"
#include "match.h"

int nextPlayerID= 1;
int nextMatchID= 1;


void testBoard() {
    Board b;
    boardStartGame(&b, 1);
    boardPrint(&b);

    if (boardIsMoveLegal(&b, 2, 0)) {
        int points = boardMove(&b, 2);
        printf("Points: %d\n", points);
    }

    boardPrint(&b);
}

void testPlayer() {
    Player p;
    playerInit(&p, nextPlayerID, "Alice");
    playerPrint(&p);

}

void testMatch()
{
    Match m;
    Player p1;
    playerInit(&p1, nextPlayerID, "Eugenie");

    Player p2;
    playerInit(&p2, nextPlayerID, "Vinicius");

    matchInit(&m, nextMatchID, p1.id, p2.id, 1);
    matchPrint(&m);
}

int main(){

    // testPlayer();
    // testBoard();
    // testMatch();
    Match m;
    Player p1;
    playerInit(&p1, nextPlayerID, "Eugenie");

    Player p2;
    playerInit(&p2, nextPlayerID, "Vinicius");

    matchInit(&m, nextMatchID, p1.id, p2.id, 1);
    Board* board = &(m.board);

    int playingPlayerID = 0;
    while(m.winner == ONGOING)
    {

        boardPrint(board);
        int move;
        scanf("%d", &move);
        // TO DO: fix bug - entering a letter breaks the code
        if(boardIsMoveLegal(board, move, playingPlayerID)){
            boardMove(board, move);
            if(playingPlayerID==0){playingPlayerID=1;}
            else if(playingPlayerID==1) {playingPlayerID=0;}
        }
    
    }

    return 0;
}

