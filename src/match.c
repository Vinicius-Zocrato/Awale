#include "match.h"
#include "player.h"

//finir match -> verifier si tableau matchId des joueurs est assez grand

void matchInit(Match *m, int id, int playerId1, int playerId2, int sens)
{
    if (m == NULL) {
        fprintf(stderr, "Match pointer is NULL in mINIT.\n");
        return;
    }

    m->id= id;
    m->finished= false;

    m->players[0] = playerLoadPlayerFromCsv(playerId1);
    m->players[1] = playerLoadPlayerFromCsv(playerId2);

    m->winner= ONGOING;

    m->scores[0]= 0;
    m->scores[1]= 0;

    m->moveSequences= (int*)malloc(20*sizeof(int)); 
    Board board;
    boardStartGame(&board, 1);
    m->board = board;
    matchStart(m, sens);

}
void matchStart(Match *m, int sens)
{
    if (m == NULL) {
        fprintf(stderr, "Match pointer is NULL in mSTART.\n");
        return;
    }
    boardStartGame(&(m->board), sens);
}

void matchEnd(Match *m)
{
    if (m == NULL) {
        fprintf(stderr, "Match pointer is NULL in mEND.\n");
        return;
    }

}
void matchPrint(const Match *m)
{
    if (m == NULL) {
        fprintf(stderr, "Match pointer is NULL in mPRINT.\n");
        return;
    }
    if(m->moveSequences == NULL) {
        printf("No moves played yet.\n");
        return;
    }

    printf("Match ID: %d\n", m->id);
    printf("Winner: %d\n", m->winner);
    printf("Score P1: %d, Score P2: %d\n", m->scores[0], m->scores[1]);
    boardPrint(&(m->board));
}

Match matchFindMatchInCsv(int matchId)
{
    //TODO: implement
    Match m;
    m.id = matchId;
    return m;
}

void matchStoreInCSV(const Match *m)
{
    if (m == NULL) {
        fprintf(stderr, "Match pointer is NULL in mSTOREINC SV.\n");
        return;
    }
    //TODO: implement
}

void matchDestroy(Match *m)
{
    if (m == NULL) {
        fprintf(stderr, "Match pointer is NULL in mDESTROY.\n");
        return;
    }
    if (m->moveSequences != NULL) {
        free(m->moveSequences);
        m->moveSequences = NULL;
    }
}

