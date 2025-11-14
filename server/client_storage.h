#ifndef CLIENT_STORAGE_H
#define CLIENT_STORAGE_H

#include "client_server_side.h"
#include <stdbool.h>

#define CLIENTS_CSV_PATH "data/clients.csv"

void clientStoreInCSV(const Client *client);
Client clientLoadFromCSV(const char *username);
void clientUpdateInCSV(const Client *client);
bool clientExistInCSV(const char *username);
bool data_signup(const char *username, const char *mdp);
bool data_login(const char *username, const char *mdp);

#endif