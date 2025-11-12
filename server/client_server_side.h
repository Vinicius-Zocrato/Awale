#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"

typedef struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char profile[BUF_SIZE];

   // Games
   struct Client *opponent;
   struct Client *challengedFrom;
   ServerMatch *match;
   int player;

   // Friends
   char friends[BUF_SIZE][20];
   int actualFriends;
   struct Client *friendRequests[20];
   int actualFriendRequests;
   struct Client *friendRequestsSent[20];
   int actualFriendRequestsSent;
   int private;

} Client;

#endif
