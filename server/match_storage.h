#ifndef MATCH_STORAGE_H
#define MATCH_STORAGE_H

#include "server_match.h"
#include "client_server_side.h"
#include <stdbool.h>

// Fonctions de persistance des matchs
void matchStoreInCSV(const ServerMatch *match);
ServerMatch matchLoadFromCSV(int matchId);
void matchUpdateInCSV(const ServerMatch *match);
bool matchExistsInCSV(int matchId);
void matchDeleteFromCSV(int matchId);

// Fonctions utilitaires
void matchListAll(int *matchIds, int *count);  // Liste tous les matchs sauvegardés
void matchResume(ServerMatch *match, Client *p1, Client *p2);  // Reprendre un match

#endif