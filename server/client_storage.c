#include "client_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void clientStoreInCSV(const Client *client)
{
    if (client == NULL)
    {
        fprintf(stderr, "Client pointer is NULL\n");
        return;
    }

    FILE *file = fopen(CLIENTS_CSV_PATH, "a");
    if (file == NULL)
    {
        fprintf(stderr, "Erreur : impossible d'ouvrir clients.csv\n");
        return;
    }

    long pos = ftell(file);
    if (pos == 0)
    {
        fprintf(file, "username,password,profile,friends,private\n");
    }

    char friendsStr[BUF_SIZE * 20] = "";
    int offset = 0;
    for (int i = 0; i < client->actualFriends; i++)
    {
        if (i > 0)
            offset += sprintf(friendsStr + offset, ";");
        offset += sprintf(friendsStr + offset, "%s", client->friends[i]);
    }

    fprintf(file, "\"%s\",\"%s\",\"%s\",\"%s\",%d\n",
            client->name,
            client->password,
            client->profile,
            friendsStr,
            client->private);

    fclose(file);
}

Client clientLoadFromCSV(const char *username)
{
    Client client;
    memset(&client, 0, sizeof client);

    client.sock = -1;
    strncpy(client.name, username, sizeof(client.name) - 1);
    client.name[sizeof(client.name) - 1] = '\0';
    client.opponent = NULL;
    client.challengedFrom = NULL;
    client.match = NULL;
    client.player = -1;

    FILE *file = fopen(CLIENTS_CSV_PATH, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier clients.csv\n");
        return client;
    }

    char line[2000];
    bool found = false;

    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char name[BUF_SIZE];
        char password[BUF_SIZE];
        char profile[BUF_SIZE];
        char friendsStr[BUF_SIZE * 20];
        int priv;

        int parsed = sscanf(line, "\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",%d",
                            name, password, profile, friendsStr, &priv);

        if (parsed == 5 && strcmp(name, username) == 0)
        {
            found = true;

            strncpy(client.name, name, sizeof(client.name) - 1);
            client.name[sizeof(client.name) - 1] = '\0';

            strncpy(client.password, password, sizeof(client.password) - 1);
            client.password[sizeof(client.password) - 1] = '\0';

            strncpy(client.profile, profile, sizeof(client.profile) - 1);
            client.profile[sizeof(client.profile) - 1] = '\0';

            client.private = priv;

            if (strlen(friendsStr) > 0)
            {
                int index = 0;
                int offset = 0;
                int len = (int)strlen(friendsStr);

                for (int i = 0; i <= len && index < 20; i++)
                {
                    if (friendsStr[i] == ';' || friendsStr[i] == '\0')
                    {
                        int friendLen = i - offset;
                        if (friendLen > 0)
                        {
                            strncpy(client.friends[index], &friendsStr[offset], friendLen);
                            client.friends[index][friendLen] = '\0';
                            index++;
                        }
                        offset = i + 1;
                    }
                }
                client.actualFriends = index;
            }
            else
            {
                client.actualFriends = 0;
            }

            break;
        }
    }

    fclose(file);
    return client;
}

void clientUpdateInCSV(const Client *client)
{
    if (client == NULL)
    {
        fprintf(stderr, "Client pointer is NULL\n");
        return;
    }

    FILE *file = fopen(CLIENTS_CSV_PATH, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Erreur : impossible d'ouvrir clients.csv\n");
        return;
    }

    char lines[1000][2000];
    int lineCount = 0;
    int foundAtLine = -1;

    if (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL)
    {
        lineCount++;
    }

    while (fgets(lines[lineCount], sizeof(lines[0]), file) != NULL && lineCount < 1000)
    {
        char name[BUF_SIZE];
        if (sscanf(lines[lineCount], "\"%[^\"]\"", name) == 1)
        {
            if (strcmp(name, client->name) == 0 && foundAtLine == -1)
            {
                foundAtLine = lineCount;
            }
        }
        lineCount++;
    }
    fclose(file);

    if (foundAtLine == -1)
    {
        fprintf(stderr, "Client %s non trouvé, impossible de mettre à jour\n", client->name);
        return;
    }

    char friendsStr[BUF_SIZE * 20] = "";
    int offset = 0;
    for (int i = 0; i < client->actualFriends; i++)
    {
        if (i > 0)
            offset += sprintf(friendsStr + offset, ";");
        offset += sprintf(friendsStr + offset, "%s", client->friends[i]);
    }

    char newLine[2000];
    sprintf(newLine, "\"%s\",\"%s\",\"%s\",\"%s\",%d\n",
            client->name,
            client->password,
            client->profile,
            friendsStr,
            client->private);

    file = fopen(CLIENTS_CSV_PATH, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Erreur : impossible d'ouvrir clients.csv en écriture\n");
        return;
    }

    for (int i = 0; i < lineCount; i++)
    {
        if (i == foundAtLine)
        {
            fprintf(file, "%s", newLine);
        }
        else
        {
            fprintf(file, "%s", lines[i]);
        }
    }

    fclose(file);
}

bool clientExistInCSV(const char *username)
{
    bool found = false;

    FILE *file = fopen(CLIENTS_CSV_PATH, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier clients.csv\n");
        return found;
    }

    char line[2000];

    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char name[BUF_SIZE];
        if (sscanf(line, "\"%[^\"]\"", name) == 1)
        {
            if (strcmp(name, username) == 0)
            {
                found = true;
                break;
            }
        }
    }

    fclose(file);
    return found;
}

bool data_signup(const char *username, const char *mdp)
{
    if (username == NULL || mdp == NULL)
        return false;

    if (clientExistInCSV(username))
        return false;

    Client newClient;
    memset(&newClient, 0, sizeof newClient);

    newClient.sock = -1;
    newClient.opponent = NULL;
    newClient.challengedFrom = NULL;
    newClient.match = NULL;
    newClient.player = -1;
    newClient.actualFriends = 0;
    newClient.actualFriendRequests = 0;
    newClient.actualFriendRequestsSent = 0;
    newClient.private = 0;

    strncpy(newClient.name, username, sizeof(newClient.name) - 1);
    newClient.name[sizeof(newClient.name) - 1] = '\0';

    strncpy(newClient.password, mdp, sizeof(newClient.password) - 1);
    newClient.password[sizeof(newClient.password) - 1] = '\0';

    newClient.profile[0] = '\0';

    clientStoreInCSV(&newClient);
    return true;
}

bool data_login(const char *username, const char *mdp)
{
    if (username == NULL || mdp == NULL)
        return false;

    FILE *f = fopen(CLIENTS_CSV_PATH, "r");
    if (!f)
        return false;

    char line[2000];
    bool ok = false;

    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f))
    {
        char u[256], p[256];
        if (sscanf(line, "\"%[^\"]\",\"%[^\"]\"", u, p) == 2)
        {
            if (strcmp(u, username) == 0 && strcmp(p, mdp) == 0)
            {
                ok = true;
                break;
            }
        }
    }
    fclose(f);
    return ok;
}