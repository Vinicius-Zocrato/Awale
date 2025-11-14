#include "match_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MATCHES_CSV_PATH "data/matches.csv"

void matchStoreInCSV(const ServerMatch *match) {
    if (match == NULL) {
        fprintf(stderr, "Match pointer is NULL\n");
        return;
    }

    FILE* file = fopen(MATCHES_CSV_PATH, "a");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir matches.csv\n");
        return;
    }
    
    // Vérifier si fichier vide → écrire l'en-tête
    long pos = ftell(file);
    if (pos == 0) {
        fprintf(file, "id,player1Name,player2Name,winner,score1,score2,boardState,moveSequences,whoseTurn,sens\n");
    }

    // Convertir l'état du plateau en chaîne: "4;4;4;4;4;4;4;4;4;4;4;4"
    char boardStateStr[200] = "";
    int offset = 0;
    for (int i = 0; i < 12; i++) {
        if (i > 0) {
            offset += sprintf(boardStateStr + offset, ";");
        }
        offset += sprintf(boardStateStr + offset, "%d", match->board->pits[i]);
    }

    // Convertir la séquence de coups en chaîne: "3;7;2;9"
    char moveSeqStr[500] = "";
    offset = 0;
    for (int i = 0; i < match->moveCount; i++) {
        if (i > 0) {
            offset += sprintf(moveSeqStr + offset, ";");
        }
        offset += sprintf(moveSeqStr + offset, "%d", match->moveSequences[i]);
    }

    // Écrire la ligne
    fprintf(file, "%d,\"%s\",\"%s\",%d,%d,%d,\"%s\",\"%s\",%d,%d\n",
        match->id,
        match->player1 ? match->player1->name : "",
        match->player2 ? match->player2->name : "",
        match->winner,
        match->scores[0],
        match->scores[1],
        boardStateStr,
        moveSeqStr,
        match->board->whoseTurn,
        match->board->sens);

    fclose(file);
    printf("Match %d sauvegardé dans le CSV\n", match->id);
}

ServerMatch matchLoadFromCSV(int matchId) {
    ServerMatch match;
    match.id = -1;  // Par défaut = non trouvé
    match.board = NULL;
    match.moveSequences = NULL;
    match.player1 = NULL;
    match.player2 = NULL;

    FILE* file = fopen(MATCHES_CSV_PATH, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir matches.csv\n");
        return match;
    }

    char line[3000];
    bool found = false;

    // Ignorer l'en-tête
    fgets(line, sizeof(line), file);

    // Chercher le match
    while (fgets(line, sizeof(line), file) != NULL) {
        int id;
        sscanf(line, "%d,", &id);

        if (id == matchId) {
            found = true;
            
            char player1Name[BUF_SIZE];
            char player2Name[BUF_SIZE];
            int winner;
            int score1, score2;
            char boardStateStr[200];
            char moveSeqStr[500];
            int whoseTurn;
            int sens;

            // Parser la ligne complète
            sscanf(line, "%d,\"%[^\"]\",\"%[^\"]\",%d,%d,%d,\"%[^\"]\",\"%[^\"]\",%d,%d",
                   &id, player1Name, player2Name, &winner, &score1, &score2,
                   boardStateStr, moveSeqStr, &whoseTurn, &sens);

            match.id = id;
            match.winner = winner;
            match.scores[0] = score1;
            match.scores[1] = score2;

            // Allouer et reconstruire le plateau
            match.board = (Board*)malloc(sizeof(Board));
            match.board->whoseTurn = whoseTurn;
            match.board->sens = sens;

            // Parser l'état du plateau: "4;4;4;4;4;4;4;4;4;4;4;4"
            int index = 0;
            int offset = 0;
            int len = (int)strlen(boardStateStr);
            
            for (int i = 0; i <= len && index < 12; i++) {
                if (boardStateStr[i] == ';' || boardStateStr[i] == '\0') {
                    char numStr[20];
                    int numLen = i - offset;
                    strncpy(numStr, &boardStateStr[offset], numLen);
                    numStr[numLen] = '\0';
                    
                    match.board->pits[index] = atoi(numStr);
                    index++;
                    offset = i + 1;
                }
            }

            // Parser la séquence de coups: "3;7;2;9"
            match.moveCapacity = 20;
            match.moveCount = 0;
            match.moveSequences = (int*)calloc(match.moveCapacity, sizeof(int));
            
            if (strlen(moveSeqStr) > 0) {
                index = 0;
                offset = 0;
                len = (int)strlen(moveSeqStr);
                
                for (int i = 0; i <= len && index < match.moveCapacity; i++) {
                    if (moveSeqStr[i] == ';' || moveSeqStr[i] == '\0') {
                        char numStr[20];
                        int numLen = i - offset;
                        strncpy(numStr, &moveSeqStr[offset], numLen);
                        numStr[numLen] = '\0';
                        
                        match.moveSequences[index] = atoi(numStr);
                        index++;
                        offset = i + 1;
                    }
                }
                match.moveCount = index;
            }

            break;
        }
    }

    if (found) {
        printf("Match %d chargé depuis le CSV\n", matchId);
    } else {
        fprintf(stderr, "Match %d non trouvé dans le CSV\n", matchId);
    }

    fclose(file);
    return match;
}

void matchUpdateInCSV(const ServerMatch *match) {
    if (match == NULL) {
        fprintf(stderr, "Match pointer is NULL\n");
        return;
    }

    FILE* file = fopen(MATCHES_CSV_PATH, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir matches.csv\n");
        return;
    }

    char lines[1000][3000];
    int lineCount = 0;
    int foundAtLine = -1;

    // Lire l'en-tête
    if (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL) {
        lineCount++;
    }

    // Lire toutes les lignes
    while (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL && lineCount < 1000) {
        int id;
        sscanf(lines[lineCount], "%d,", &id);
        
        if (id == match->id && foundAtLine == -1) {
            foundAtLine = lineCount;
        }
        lineCount++;
    }
    fclose(file);

    if (foundAtLine == -1) {
        fprintf(stderr, "Match %d non trouvé, impossible de mettre à jour\n", match->id);
        return;
    }

    // Préparer la nouvelle ligne
    char boardStateStr[200] = "";
    int offset = 0;
    for (int i = 0; i < 12; i++) {
        if (i > 0) {
            offset += sprintf(boardStateStr + offset, ";");
        }
        offset += sprintf(boardStateStr + offset, "%d", match->board->pits[i]);
    }

    char moveSeqStr[500] = "";
    offset = 0;
    for (int i = 0; i < match->moveCount; i++) {
        if (i > 0) {
            offset += sprintf(moveSeqStr + offset, ";");
        }
        offset += sprintf(moveSeqStr + offset, "%d", match->moveSequences[i]);
    }

    char newLine[3000];
    sprintf(newLine, "%d,\"%s\",\"%s\",%d,%d,%d,\"%s\",\"%s\",%d,%d\n",
            match->id,
            match->player1 ? match->player1->name : "",
            match->player2 ? match->player2->name : "",
            match->winner,
            match->scores[0],
            match->scores[1],
            boardStateStr,
            moveSeqStr,
            match->board->whoseTurn,
            match->board->sens);

    // Réécrire le fichier
    file = fopen(MATCHES_CSV_PATH, "w");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir matches.csv en écriture\n");
        return;
    }

    for (int i = 0; i < lineCount; i++) {
        if (i == foundAtLine) {
            fprintf(file, "%s", newLine);
        } else {
            fprintf(file, "%s", lines[i]);
        }
    }

    fclose(file);
    printf("Match %d mis à jour dans le CSV\n", match->id);
}

bool matchExistsInCSV(int matchId) {
    FILE* file = fopen(MATCHES_CSV_PATH, "r");
    if (file == NULL) {
        return false;
    }

    char line[3000];
    fgets(line, sizeof(line), file);  // Ignorer en-tête

    while (fgets(line, sizeof(line), file) != NULL) {
        int id;
        sscanf(line, "%d,", &id);
        
        if (id == matchId) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

void matchDeleteFromCSV(int matchId) {
    FILE* file = fopen(MATCHES_CSV_PATH, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir matches.csv\n");
        return;
    }

    char lines[1000][3000];
    int lineCount = 0;

    // Lire tout le fichier
    while (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL && lineCount < 1000) {
        lineCount++;
    }
    fclose(file);

    // Réécrire sans la ligne du match
    file = fopen(MATCHES_CSV_PATH, "w");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir matches.csv en écriture\n");
        return;
    }

    for (int i = 0; i < lineCount; i++) {
        if (i == 0) {
            // Toujours garder l'en-tête
            fprintf(file, "%s", lines[i]);
            continue;
        }

        int id;
        sscanf(lines[i], "%d,", &id);
        
        if (id != matchId) {
            fprintf(file, "%s", lines[i]);
        }
    }

    fclose(file);
    printf("Match %d supprimé du CSV\n", matchId);
}

void matchListAll(int *matchIds, int *count) {
    if (matchIds == NULL || count == NULL) {
        fprintf(stderr, "Paramètres NULL dans matchListAll\n");
        return;
    }

    *count = 0;

    FILE* file = fopen(MATCHES_CSV_PATH, "r");
    if (file == NULL) {
        return;  // Fichier n'existe pas, liste vide
    }

    char line[3000];
    fgets(line, sizeof(line), file);  // Ignorer en-tête

    while (fgets(line, sizeof(line), file) != NULL && *count < 1000) {
        int id;
        if (sscanf(line, "%d,", &id) == 1) {
            matchIds[*count] = id;
            (*count)++;
        }
    }

    fclose(file);
}

void matchResume(ServerMatch *match, Client *p1, Client *p2) {
    if (match == NULL || p1 == NULL || p2 == NULL) {
        fprintf(stderr, "Paramètres NULL dans matchResume\n");
        return;
    }

    // Lier les joueurs au match
    match->player1 = p1;
    match->player2 = p2;

    // Lier le match aux clients
    p1->match = match;
    p1->opponent = p2;
    p1->player = PLAYER1;

    p2->match = match;
    p2->opponent = p1;
    p2->player = PLAYER2;

    printf("Match %d repris : %s vs %s\n", match->id, p1->name, p2->name);
}