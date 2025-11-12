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

    m->players[0] = playerFindPlayerInCsv(playerId1);
    m->players[1] = playerFindPlayerInCsv(playerId2);

    m->winner= ONGOING;

    m->scores[0]= 0;
    m->scores[1]= 0;

    m->moveSequences= (int*)malloc(20*sizeof(int)); 
    m->moveCapacity = 20;
    m->moveCount = 0;

    m->board = malloc(sizeof(Board));
    if (m->board == NULL) {
        fprintf(stderr, "Allocation failed for board.\n");
        return;
    }
    boardStartGame(m->board, sens);
}

void matchMove(Match *m, int pit)
{
    int points = boardMove(m->board, pit);
    
    m->scores[m->board->whoseTurn] += points;
    if(m->moveCapacity == m->moveCount)
    {
        realloc(m->moveSequences, sizeof(int)*(m->moveCapacity+10));
        m->moveCapacity += 10;
    }
    m->moveSequences[m->moveCount] = pit;
    m->moveCount++;

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
    boardPrint(m->board);
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
    if (m->board != NULL) {
        free(m->board);
        m->board = NULL;
    }
}

bool matchIsGameOver(Match *m)
{
    if (m == NULL || m->board == NULL) {
        fprintf(stderr, "Board or Match pointer is NULL in matchISGAMEOVER.\n");
        return false;
    }

    int sumP1 = 0, sumP2 = 0;
    for (int i = 0; i < 6; i++) {
        sumP1 += m->board->pits[i];
        sumP2 += m->board->pits[i+6];
    }

    // --- Fin par indétermination ---
    if (sumP1 + sumP2 <= 3) {
        printf("Fin de partie : indétermination (trop peu de graines).\n");
        printf("Player 1 garde %d graines, Player 2 garde %d graines.\n", sumP1, sumP2);
        m->scores[0]+=sumP1;
        m->scores[1] += sumP2;
        return true;
    }

    // --- Fin par famine ---
    if (m->board->whoseTurn == 0 && sumP1 == 0) {
        // Vérifier si le joueur 2 peut nourrir (en jouant une case qui dépose dans camp 1)
        bool peutNourrir = false;
        for (int i = 6; i < 12; i++) {
            if (m->board->pits[i] > 12 - i) {
                peutNourrir = true;
                break;
            }
        }
        if (!peutNourrir) {
            printf("Fin de partie : famine. Joueur 2 ramasse tout le reste !\n");
            // Joueur 2 ramasse les graines restantes
            int reste = sumP1 + sumP2;
            for (int i = 0; i < 12; i++) m->board->pits[i] = 0;
            m->scores[1] += reste;
            return true;
        }
    }

    if (m->board->whoseTurn == 1 && sumP2 == 0) {
        bool peutNourrir = false;
        for (int i = 0; i < 6; i++) {
            if (m->board->pits[i] > 6 - i) {
                peutNourrir = true;
                break;
            }
        }
        if (!peutNourrir) {
            printf("Fin de partie : famine. Joueur 1 ramasse tout le reste !\n");
            int reste = sumP1 + sumP2;
            for (int i = 0; i < 12; i++) m->board->pits[i] = 0;
            m->scores[0] += reste;
            return true;
        }
    }

    // Sinon, la partie continue
    return false;
}