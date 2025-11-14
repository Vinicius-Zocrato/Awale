#include "board.h"
#include "match.h"

bool boardIsMoveLegal(const Board *b, int pit, int playerMatchId)
{
    if (b == NULL)
    {
        fprintf(stderr, "Board pointer is NULL in ISMOVELEGAL.\n");
        return false;
    }

    if (pit <= 0 || pit > 12)
        return false;

    int idx = pit - 1; /* zero-based index for array access */

    if (b->whoseTurn != playerMatchId)
    {
        printf("It's Player %d's turn, and you're Player %d !\n", b->whoseTurn + 1, playerMatchId + 1);
        return false;
    }

    if (b->pits[idx] == 0)
    {
        printf("No stones in the pit\n");
        return false;
    }

    /* keep these checks with 1-based pit values */
    if (b->whoseTurn == 0 && pit > 6)
    {
        printf("Player 1 must play in pits 1 to 6\n");
        return false;
    }

    if (b->whoseTurn == 1 && pit < 7)
    {
        printf("Player 2 must play in pits 7 to 12\n");
        return false;
    }

    return true;
}

int boardMove(Board *b, int pit)
{
    if (b == NULL)
    {
        fprintf(stderr, "Board pointer is NULL in MOVE.\n");
        return -1;
    }

    pit = pit - 1; // Convert to 0-based index

    int stones = b->pits[pit];
    b->pits[pit] = 0;
    int index = pit;

    while (stones > 0)
    {
        index = (index + b->sens + 12) % 12;
        if (index == pit)
            index = (index + b->sens + 12) % 12; // Skip starting pit
        b->pits[index]++;
        stones--;
    }

    int points = 0;
    if ((b->whoseTurn == 0 && index >= 6) || (b->whoseTurn == 1 && index < 6))
    {
        while (b->pits[index] == 2 || b->pits[index] == 3)
        {
            points += b->pits[index];
            b->pits[index] = 0;
            index = (index - b->sens + 12) % 12;
        }
    }

    b->whoseTurn = (b->whoseTurn == 0) ? 1 : 0;
    return points;
}

void boardPrint(const Board *b)
{
    if (b == NULL)
    {
        fprintf(stderr, "Board pointer is NULL in PRINT.\n");
        return;
    }

    printf("Board State:\n");
    /* top row: pits 7..12 => indices 6..11 left-to-right */
    printf("  ");
    for (int i = 6; i <= 11; i++)
        printf("%2d ", b->pits[i]);
    printf("\n");

    /* separator */
    printf("-------------------------\n");

    /* bottom row: pits 1..6 => indices 0..5 left-to-right */
    printf("  ");
    for (int i = 0; i <= 5; i++)
        printf("%2d ", b->pits[i]);
    printf("\n");

    printf("Next turn: Player %d\n", b->whoseTurn + 1);
    printf("Board sens: %d\n\n", b->sens);
}

void boardStartGame(Board *b, int direction)
{
    if (b == NULL)
    {
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
    if (b == NULL)
    {
        fprintf(stderr, "Board pointer is NULL in DESTROY.\n");
        return;
    }
}
