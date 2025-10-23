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


int main() {

    testPlayer();

    return 0;
}

