#include "board.h"

bool boardIsMoveLegal(const Board *b, int pit, int playerMatchId)
{
    if (b == NULL) {
        fprintf(stderr, "Board pointer is NULL in ISMOVELEGAL.\n");
        return false;
    }

    if (b->whoseTurn != playerMatchId) {
        printf("It's Player %d's turn, and you're Player %d !\n", b->whoseTurn+1, playerMatchId+1);
        
        return false;
    }
    if (pit <= 0 || pit > 12) return false;

    if (b->pits[pit] == 0) {
        printf("No stones in the pit\n");
        return false;
    }

    if (b->whoseTurn == 0 && pit>6){
        printf("Player 1 must play in pits 1 to 6\n");
        return false;
    }

    if (b->whoseTurn == 1 && pit<7){
        printf("Player 2 must play in pits 7 to 12\n");
        return false;
    }

    return true;
}

int boardMove(Board *b, int pit)
{
    if (b == NULL) {
        fprintf(stderr, "Board pointer is NULL in MOVE.\n");
        return -1;
    }

    pit = pit-1; // Convert to 0-based index

    int stones = b->pits[pit];
    b->pits[pit] = 0;
    int index = pit;

    while (stones > 0) {
        index = (index + b->sens + 12) % 12;
        if (index == pit)
            index = (index + b->sens + 12) % 12; // Skip starting pit
        b->pits[index]++;
        stones--;
    }

    int points = 0;
    while (b->pits[index] == 2 || b->pits[index] == 3) {
        points += b->pits[index];
        b->pits[index] = 0;
        index = (index - b->sens + 12) % 12;
    }

    b->whoseTurn = (b->whoseTurn == 0) ? 1 : 0;
    return points;
}

void boardPrint(const Board *b)
{
    if (b == NULL) {
        fprintf(stderr, "Board pointer is NULL in PRINT.\n");
        return;
    }

    if(b->whoseTurn == 0){
        printf("Board State:\n");
        printf("| ");
        for (int i = 0; i < 6; i++)
            printf("%d | ", b->pits[11-i]);
        printf("\n-------------------------\n| ");
        for (int i = 0; i < 6; i++)
            printf("%d | ", b->pits[i]);
        printf("\n");
        printf("Next turn: Player %d\n", b->whoseTurn+1);
        printf("Board sens: %d\n\n", b->sens);
    }

    else{
        printf("Board State:\n");
        printf("| ");
        for (int i = 0; i < 6; i++)
            printf("%d | ", b->pits[6-i-1]);
        printf("\n-------------------------\n| ");
        for (int i = 0; i < 6; i++)
            printf("%d | ", b->pits[i+6]);
        printf("\n");
        printf("Next turn: Player %d\n", b->whoseTurn+1);
        printf("Board sens: %d\n\n", b->sens);
    }

}

bool boardIsGameOver(const Board *b)
{
    //TODO: complete

    if (b == NULL) {
        fprintf(stderr, "Board pointer is NULL in ISGAMEOVER.\n");
        return false;
    }


    return false;
}

void boardStartGame(Board *b, int direction)
{
    if (b == NULL) {
        fprintf(stderr, "Board pointer is NULL in STARTGAME.\n");
        return;
    }

    b->sens = direction;
    b->whoseTurn = 0;
    for (int i = 0; i < 12; i++)
        b->pits[i] = 4;
}

void boardDestroy(Board *b)
{
    // No dynamic memory to free currently
    if (b == NULL) {
        fprintf(stderr, "Board pointer is NULL in DESTROY.\n");
        return;
    }
}