#include "player.h"

void playerPrint(const Player *p)
{
    if (p == NULL) {
        fprintf(stderr, "Player pointer is NULL in PRINT.\n");
        return;
    }

    printf("Player ID: %d\n", p->id);
    printf("Username: %s\n", p->username);
    if(p->matchIds != NULL) {
        if(p->matchIds[0] == 0) {
            printf("No matches played\n\n");
            return;
        }
        printf("Match IDs: ");
        for(int i = 0; i < p->matchCount; i++) {
            if (p->matchIds[i] == 0) break;
            printf("%d ", p->matchIds[i]);
        }
        printf("\n");
    } else {
        printf("MATCHIDS NULL in playerPrint\n");
    }
    printf("\n");
}

void playerInit(Player *p, int id, const char *username)
{
    if (p == NULL) {
        fprintf(stderr, "Player pointer is NULL in INIT.\n");
        return;
    }

    p->id = id;

    if (username == NULL || strlen(username) > 49) {
        fprintf(stderr, "Invalid username in playerPrint.\n");
    }
    else{
        strncpy(p->username, username, sizeof(p->username));
        p->username[sizeof(p->username)] = '\0'; // Ensure null-termination
    }

    p->matchCount = 10;
    p->matchIds = (int*)malloc(p->matchCount * sizeof(int));
}

void playerAddMatch(Player *p, int matchId)
{
    if (p == NULL) {
        fprintf(stderr, "Player pointer is NULL in ADDMATCH.\n");
        return;
    }

    // Check if we need to resize the matchIds array
    int currentCount = 0;
    while (currentCount < p->matchCount && p->matchIds != NULL && p->matchIds[currentCount] != 0) {
        currentCount++;
    }

    if (currentCount >= p->matchCount) {
        // Resize the array
        int newSize = p->matchCount * 2;
        int *newMatchIds = (int *)malloc(newSize * sizeof(int));
        if (newMatchIds == NULL) {
            fprintf(stderr, "Memory allocation failed in ADDMATCH.\n");
            return;
        }

        // Copy old match IDs to new array
        for (int i = 0; i < p->matchCount; i++) {
            newMatchIds[i] = p->matchIds[i];
        }
        // Initialize the rest of the new array to 0
        for (int i = p->matchCount; i < newSize; i++) {
            newMatchIds[i] = 0;
        }

        // Free old array and update player struct
        free(p->matchIds);
        p->matchIds = newMatchIds;
        p->matchCount = newSize;
    }

    // Add the new match ID
    p->matchIds[currentCount] = matchId;
}

Player playerFindPlayerInCsv(int playerId)
{
    //TODO: implement
    Player p;
    p.id = playerId;
    return p;
}

void playerStoreInCSV(const Player *p)
{
    if (p == NULL) {
        fprintf(stderr, "Match pointer is NULL in mSTOREINC SV.\n");
        return;
    }
    //TODO: implement
}

void playerDestroy(Player *p)
{
    if (p == NULL) {
        fprintf(stderr, "Player pointer is NULL in DESTROY.\n");
        return;
    }
    if (p->matchIds != NULL) {
        free(p->matchIds);
        p->matchIds = NULL;
    }
}