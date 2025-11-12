#ifndef SERVER_MATCH_H
#define SERVER_MATCH_H

#include "./server/server.h"

typedef struct Client Client;

typedef struct
{
   int tab[12];
   Client *sud;
   Client *nord;
   Client *observers[MAX_CLIENTS];
   int joueur;
   int pointsSud;
   int pointsNord;
   int private;

} ServerMatch;

#endif
