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

void testPlayerStoreInCSV() {
    printf("=== Test playerStoreInCSV ===\n");
    
    // Test 1 : Joueur avec des matchs
    Player p1;
    playerInit(&p1, 1, "Alice");
    playerAddMatch(&p1, 5);
    playerAddMatch(&p1, 12);
    playerAddMatch(&p1, 8);
    
    printf("Stockage du joueur 1...\n");
    playerStoreInCSV(&p1);
    playerDestroy(&p1);
    
    // Test 2 : Joueur sans matchs
    Player p2;
    playerInit(&p2, 2, "Bob");
    
    printf("Stockage du joueur 2 (sans matchs)...\n");
    playerStoreInCSV(&p2);
    playerDestroy(&p2);
}

void testPlayerLoadFromCsv() {
    Player p1 = playerLoadPlayerFromCsv(1);
    Player p2 = playerLoadPlayerFromCsv(2);
    Player p3 = playerLoadPlayerFromCsv(3);
    playerDestroy(&p1);
    playerDestroy(&p2);
    playerDestroy(&p3);
}

void testPlayerUpdateCsv(){
    Player p1 = playerLoadPlayerFromCsv(1);

    playerAddMatch(&p1, 6);
    playerAddMatch(&p1, 3);
    playerAddMatch(&p1, 2);

    playerUpdateInCSV(&p1);

    playerDestroy(&p1);
}

int main(){

    // testPlayer();
    // testBoard();
    // testMatch();
    //testPlayerStoreInCSV();
    //testPlayerLoadFromCsv();
    testPlayerUpdateCsv();
    /*Match m;
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
    
    }*/

    return 0;
}

