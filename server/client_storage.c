#include "client_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void clientStoreInCSV(const Client *client) {
    if (client == NULL) {
        fprintf(stderr, "Client pointer is NULL\n");
        return;
    }

    FILE* file = fopen(CLIENTS_CSV_PATH, "a");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir clients.csv\n");
        return;
    }
    
    // Vérifier si fichier vide → écrire l'en-tête
    long pos = ftell(file);
    if (pos == 0) {
        fprintf(file, "username,profile,friends,private\n");
    }

    // Convertir la liste d'amis en chaîne
    char friendsStr[BUF_SIZE * 20] = "";
    int offset = 0;
    for (int i = 0; i < client->actualFriends; i++) {
        if (i > 0) {
            offset += sprintf(friendsStr + offset, ";");
        }
        offset += sprintf(friendsStr + offset, "%s", client->friends[i]);
    }

    // Écrire la ligne
    fprintf(file, "%s,\"%s\",\"%s\",%d\n", 
        client->name,
        client->profile,
        friendsStr,
        client->private);

    fclose(file);
}

Client clientLoadFromCSV(const char *username) {
    Client client;
    
    // Initialiser avec des valeurs par défaut (= non trouvé)
    client.sock = -1;
    strcpy(client.name, username);
    strcpy(client.profile, "");
    client.actualFriends = 0;
    client.private = 0;
    client.opponent = NULL;
    client.challengedFrom = NULL;
    client.match = NULL;
    client.player = -1;
    client.actualFriendRequests = 0;
    client.actualFriendRequestsSent = 0;

    FILE* file = fopen(CLIENTS_CSV_PATH, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier clients.csv\n");
        return client;
    }

    char line[2000];
    bool found = false;

    // Ignorer l'en-tête 
    fgets(line, sizeof(line), file);

    // Chercher le client
    while (fgets(line, sizeof(line), file) != NULL) {
        char name[BUF_SIZE];
        sscanf(line, "%[^,]", name);

        if (strcmp(name, username) == 0) {
            found = true;
            
            char profile[BUF_SIZE];
            char friendsStr[BUF_SIZE * 20] = "";
            int priv;

            // Parser la ligne complète
            sscanf(line, "%[^,],\"%[^\"]\",\"%[^\"]\",%d", 
                   name, profile, friendsStr, &priv);

            strcpy(client.name, name);
            strcpy(client.profile, profile);
            client.private = priv;

            // Parser la liste d'amis: "Bob;Charlie;Diana" → tableau
            if (strlen(friendsStr) > 0) {
                int index = 0;
                int offset = 0;
                int len = (int)strlen(friendsStr);
                
                for (int i = 0; i <= len && index < BUF_SIZE; i++) {
                    // Si on trouve un ';' ou la fin de chaîne
                    if (friendsStr[i] == ';' || friendsStr[i] == '\0') {
                        // Extraire le sous-string entre offset et i
                        char friendName[20];
                        int friendLen = i - offset;
                        strncpy(friendName, &friendsStr[offset], friendLen);
                        friendName[friendLen] = '\0';
                        
                        // Copier dans le tableau d'amis
                        strcpy(client.friends[index], friendName);
                        index++;
                        
                        // Passer au prochain ami (après le ';')
                        offset = i + 1;
                    }
                }
                client.actualFriends = index;
            }
            
            break;
        }
    }

    if (!found) {
        // Client non trouvé → le créer
        printf("Nouveau client : %s\n", username);
        clientStoreInCSV(&client);
    } else {
        printf("Client chargé : %s (profil: %s, amis: %d)\n", 
               client.name, client.profile, client.actualFriends);
    }

    fclose(file);
    return client;
}

void clientUpdateInCSV(const Client *client) {
    if (client == NULL) {
        fprintf(stderr, "Client pointer is NULL\n");
        return;
    }

    FILE* file = fopen(CLIENTS_CSV_PATH, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir clients.csv\n");
        return;
    }

    char lines[1000][2000];
    int lineCount = 0;
    int foundAtLine = -1;

    // Lire l'en-tête
    if (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL) {
        lineCount++;
    }

    // Lire toutes les lignes
    while (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL && lineCount < 1000) {
        char name[BUF_SIZE];
        // Parser juste le nom (avant la première virgule)
        int parsed = sscanf(lines[lineCount], "%[^,]", name);
        
        if (parsed == 1 && strcmp(name, client->name) == 0 && foundAtLine == -1) {
            foundAtLine = lineCount;
            printf("DEBUG: Found %s at line %d\n", name, lineCount);  // Debug
        }
        lineCount++;
    }
    fclose(file);

    if (foundAtLine == -1) {
        fprintf(stderr, "Client %s non trouvé, impossible de mettre à jour\n", client->name);
        return;
    }

    // Préparer la nouvelle ligne
    char friendsStr[BUF_SIZE * 20] = "";
    int offset = 0;
    for (int i = 0; i < client->actualFriends; i++) {
        if (i > 0) {
            offset += sprintf(friendsStr + offset, ";");
        }
        offset += sprintf(friendsStr + offset, "%s", client->friends[i]);
    }

    char newLine[2000];
    sprintf(newLine, "%s,\"%s\",\"%s\",%d\n", 
            client->name,
            client->profile,
            friendsStr,
            client->private);

    // Réécrire le fichier
    file = fopen(CLIENTS_CSV_PATH, "w");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir clients.csv en écriture\n");
        return;
    }

    for (int i = 0; i < lineCount; i++) {
        if (i == foundAtLine) {
            fprintf(file, "%s", newLine);
            printf("DEBUG: Replaced line %d\n", i);  // Debug
        } else {
            fprintf(file, "%s", lines[i]);
        }
    }

    fclose(file);
    printf("Client %s mis à jour\n", client->name);
}

bool clientExistInCSV(const char *username) {

    bool found = false;

    FILE* file = fopen(CLIENTS_CSV_PATH, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier clients.csv\n");
        return found;
    }

    char line[2000];

    // Ignorer l'en-tête 
    fgets(line, sizeof(line), file);

    // Chercher le client
    while (fgets(line, sizeof(line), file) != NULL) {
        char name[BUF_SIZE];
        sscanf(line, "%[^,]", name);

        if (strcmp(name, username) == 0) {
            found = true;
            break;
            }
        }

    fclose(file);
    return found;
}