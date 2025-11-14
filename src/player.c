#include "player.h"

void playerPrint(const Player *p)
{
    if (p == NULL) {
        fprintf(stderr, "Player pointer is NULL in PRINT.\n");
        return;
    }

    printf("Player ID: %d\n", p->id);
    printf("Username: %s\n", p->username);
    printf("Played Matchs : %d\n", p->matchCount);
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
    p->matchIds = (int*)calloc(p->matchCount, sizeof(int)); //changement de malloc en calloc pour éviter que la mémoire allouée contienne des valeurs aléatoires (on veut des 0)
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

Player playerLoadPlayerFromCsv(int playerId)
{
    Player player;
    player.id = -1;  // Par défaut = non trouvé
    player.matchIds = NULL;

    FILE* file = fopen("../data/players.csv", "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier players.csv\n");
        player.id =-1 ;
        return player;
    }

    char line[1000];

    // Ignorer l'en-tête 
    fgets(line, sizeof(line), file);

    // Chercher le joueur
    while (fgets(line, sizeof(line), file) != NULL)
    {
        int id;
        sscanf(line, "%d,", &id);

        if (id == playerId) {
            
            int matchCount;
            char username[50];
            char matchIdsStr[100]="";

            sscanf(line, "%d,%d,%49[^,],\"%99[^\"]\"", &id, &matchCount, username, matchIdsStr);

            player.id = id;
            player.matchCount = matchCount;
            strncpy(player.username, username, sizeof(player.username) - 1);
            player.username[sizeof(player.username) - 1] = '\0';

            // Parser matchIds: "5;12;8" → int array
            player.matchIds = (int*)calloc(matchCount, sizeof(int));
            if (strlen(matchIdsStr) > 0) {
                int index = 0;
                int offset = 0;
                int len = (int)strlen(matchIdsStr);
                
                for (int i = 0; i <= len && index < matchCount; i++) {
                    // Si on trouve un ';' ou la fin de chaîne
                    if (matchIdsStr[i] == ';' || matchIdsStr[i] == '\0') {
                        // Extraire le sous-string entre offset et i
                        char numStr[20];
                        int len = i - offset;
                        strncpy(numStr, &matchIdsStr[offset], len);
                        numStr[len] = '\0';
                        
                        // Convertir en int
                        player.matchIds[index] = atoi(numStr);
                        index++;
                        
                        // Passer au prochain nombre (après le ';')
                        offset = i + 1;
                    }
                }
            }
            break;
        }
    }

    if(player.id == -1){
        char username[50];
        printf("Nouveau joueur ! Entrez votre nom : ");
        scanf("%49s", username);  // Lire l'input
        playerInit(&player, playerId, username);  // Utiliser le nom saisi
        playerStoreInCSV(&player);
        printf("Joueur %s créé avec succès !\n", username);
    }

    fclose(file); 
    playerPrint(&player);
    return player;
}

void playerStoreInCSV(const Player *p)
{
    if (p == NULL) {
        fprintf(stderr, "Match pointer is NULL in mSTOREINC SV.\n");
        return;
    }

    FILE* file = fopen("../data/players.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier players.csv\n");
        return;
    }
    
    long pos= ftell(file);
    if(pos == 0){
        fprintf(file,"id,matchCount,username,matchIds\n");
    }

    char matchIdsBuffer[500] = ""; //si pas de match joué ça affichera juste ça 
    int offset = 0;

    if (p->matchIds != NULL) {
        for (int i = 0; i < p->matchCount; i++) {
            if (p->matchIds[i] == 0) break;
            
            //pour ne pas avoir de ";" apès le dernier id à l'affichage
            if (i > 0) {
                offset += sprintf(matchIdsBuffer + offset, ";");
            }
            offset += sprintf(matchIdsBuffer + offset, "%d", p->matchIds[i]);
        }
    }

    //Écrire les données du player dans le fichier
    fprintf(file, "%d,%d,%s,\"%s\"\n", 
        p->id, 
        p->matchCount, 
        p->username, 
        matchIdsBuffer);


    fclose(file);
}

void playerUpdateInCSV(const Player *player) {
    if (player == NULL) {
        fprintf(stderr, "Player pointer is NULL\n");
        return;
    }

    // 1. Lire toutes les lignes en mémoire
    FILE* file = fopen("../data/players.csv", "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir players.csv\n");
        return;
    }

    char lines[1000][1000];  // ✅ Tableau de lignes
    int lineCount = 0;
    int foundAtLine = -1;

    // Lire l'en-tête
    if (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL) {
        lineCount++;
    }

    // Lire toutes les lignes et chercher le joueur
    while (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL) {
        int id;
        sscanf(lines[lineCount], "%d,", &id);

        if (id == player->id && foundAtLine == -1) {
            foundAtLine = lineCount;  // ✅ Marquer la ligne trouvée
        }
        
        lineCount++;  // ✅ Incrémenter le compteur
    }

    fclose(file);  // ✅ Fermer APRÈS avoir tout lu

    // 2. Préparer la nouvelle ligne
    char matchIdsBuffer[500] = "";
    int offset = 0;
    
    if (player->matchIds != NULL) {  // ✅ Utiliser "player" partout
        for (int i = 0; i < player->matchCount; i++) {
            if (player->matchIds[i] == 0) break;
            if (i > 0) {
                offset += sprintf(matchIdsBuffer + offset, ";");
            }
            offset += sprintf(matchIdsBuffer + offset, "%d", player->matchIds[i]);
        }
    }
    
    char newLine[1000];
    sprintf(newLine, "%d,%d,%s,\"%s\"\n", 
            player->id, player->matchCount, player->username, matchIdsBuffer);
    
    // 3. Vérifier si le joueur a été trouvé
    if (foundAtLine == -1) {
        fprintf(stderr, "Joueur %d non trouvé dans le CSV\n", player->id);
        return;
    }
    
    // 4. Réécrire le fichier
    file = fopen("../data/players.csv", "w");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir players.csv en écriture\n");
        return;
    }
    
    for (int i = 0; i < lineCount; i++) {
        if (i == foundAtLine) {
            // Remplacer la ligne du joueur
            fprintf(file, "%s", newLine);
        } else {
            // Garder la ligne existante
            fprintf(file, "%s", lines[i]);
        }
    }
    
    fclose(file);
    printf("Joueur %d mis à jour dans le CSV\n", player->id);
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