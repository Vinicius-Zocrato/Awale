#ifndef SERVER_MATCH_H
#define SERVER_MATCH_H

#include "./server/server.h"

typedef struct Client Client;

typedef struct
{
   int tab[12];
   Client *player1;
   Client *player2;
   Client *observers[MAX_CLIENTS];
   int joueur;
   int pointsP1;
   int pointsP2;
   int private;

} ServerMatch;

#endif
