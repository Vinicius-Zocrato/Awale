#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

// Network includes
#include "server.h"
#include "client_server_side.h"
#include "../server_match.h"

// AWALE includes
#include "../board.h"

#include "client_storage.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE]; // 1024 bits cap for buffer

   int actual = 0;        // client index
   int actualMatches = 0; // matches index

   int max = sock; // select needs the biggest value of socket so that it loops through all of the list of sockets

   Client clients[MAX_CLIENTS];      // all connected clients (capped to 100)
   ServerMatch matches[MAX_MATCHES]; // all matches being played (capped to 50)

   fd_set rdfs;

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      // add keyboard to rdfs
      FD_SET(STDIN_FILENO, &rdfs);

      // add the connection socket
      FD_SET(sock, &rdfs);

      // add socket of each client
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      // Select is a blocking call, this will only be exited when a client connects
      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1) // null pour write, except et timeout
      {
         perror("select()");
         exit(errno);
      }

      // to stop the server properly if theres keyboard entries server-side
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         break;
      }

      else if (FD_ISSET(sock, &rdfs))
      {
         // New client connecting
         SOCKADDR_IN csin = {0};
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         // updated max so that rdfs reads all the clients
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = {csock};
         // strncpy(c.name, buffer, BUF_SIZE - 1);
         c.name[0] = '\0';
         c.challengedFrom = NULL;
         c.opponent = NULL;
         c.match = NULL;
         c.actualFriendRequests = 0;
         c.actualFriends = 0;
         c.actualFriendRequestsSent = 0;
         c.private = 0;
         clients[actual] = c;
         actual++;
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
            // identify which client is sending info
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client *client = &clients[i];
               printf("Client connected on sock %d \n", client->sock);
               int c = read_client(clients[i].sock, buffer);
               // Client déconnecté
               if (c == 0)
               {
                  printf("Disconnecting client on port %d", client->sock);
                  client_disconnected(client, matches, &actualMatches);
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
               }
               else
               {
                  message_analyzer(matches, &actualMatches, clients, client, actual, buffer);
                  break;
               }
            }
         }
      }
   }
   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   (*actual)--;
}

static void remove_match(ServerMatch *matches, ServerMatch *match, int *currentMatches)
{
   int to_remove = -1;
   for (int i = 0; i < *currentMatches; i++)
   {
      if (matches[i].player1 == match->player1 && matches[i].player2 == match->player2)
      {
         to_remove = i;
         break;
      }
   }
   if (to_remove != -1)
   {
      memmove(matches + to_remove, matches + to_remove + 1, (*currentMatches - to_remove - 1) * sizeof(ServerMatch));
      (*currentMatches)--;
   }
}

static void client_disconnected(Client *client, ServerMatch *matches, int *currentMatches)
{
   if (client->opponent != NULL)
   {
      write_client(client->opponent->sock, "Your opponent has disconnected. You won!\n");
      remove_match(matches, client->match, currentMatches);
      client->opponent->opponent = NULL;
      client->opponent->match = NULL;
      client->opponent = NULL;
      client->match = NULL;
   }
   if (client->challengedFrom != NULL)
   {
      write_client(client->challengedFrom->sock, "Your opponent has disconnected. Challenge someone else to play!\n");
      client->challengedFrom->challengedFrom = NULL;
      client->challengedFrom = NULL;
   }
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      // we don't send message to the sender
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static void send_message_to_client_by_name(Client *clients, const char *name, int actual, const char *buffer, Client *sender)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         strncpy(message, sender->name, BUF_SIZE - 1);
         strncat(message, ": ", sizeof message - strlen(message) - 1);
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int username_is_name_taken(Client *clients, int actual, const char *name)
{
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         return 1;
      }
   }
   return 0;
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }
   {
      // Après avoir essayé de débouger les connection refused, un modèle d'intelligence
      // artificielle m'a dit que REUSEADDR permettait de réduire le temps de latence
      // pour me reconnecter sur le serveur

      int opt = 1;
      if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
      {
         perror("setsockopt(SO_REUSEADDR)");
      }
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }
   printf("Server listening on port %d\n", ntohs(sin.sin_port));

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   // recv sert à lire les données envoyées par le client. C'est un appel bloquant de base
   //  (ça attendra à ce que le client envoie quelque chose), mais vu que l'appel du select
   //  est déjà bloquant, on est sûrs d'avoir de l'information, alors ça bloquera jamais

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      // if recv error we disconnect the client
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}

// Commands---------------------------------------------------------------------------------------

static void list_of_online_clients(Client *clients, Client sender, int actual)
{
   int i = 0;
   char buffer[BUF_SIZE];
   buffer[0] = 0;
   strncat(buffer, "Connected clients:\n", BUF_SIZE - strlen(buffer) - 1);
   for (i = 0; i < actual; i++)
   {
      if (clients[i].private == 1)
      {
         int isFriend = 0;
         for (int j = 0; j < sender.actualFriends; j++)
         {
            if (strcmp(sender.friends[j], clients[i].name) == 0)
            {
               isFriend = 1;
               break;
            }
         }
         if (!isFriend)
         {
            continue;
         }
      }
      strncat(buffer, "Client ", BUF_SIZE - strlen(buffer) - 1);
      snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer) - 1, "%d", i + 1);
      strncat(buffer, ": ", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, clients[i].name, BUF_SIZE - strlen(buffer) - 1);
      if (clients[i].opponent != NULL)
      {
         strncat(buffer, " [in match]", BUF_SIZE - strlen(buffer) - 1);
      }
      else
      {
         strncat(buffer, " [available]", BUF_SIZE - strlen(buffer) - 1);
      }
      strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
   }
   write_client(sender.sock, buffer);
}

static void challenge(Client *clients, Client *sender, const char *name, int actual, const char *buffer)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   int found = 0;

   if (strcmp(sender->name, name) == 0)
   {
      write_client(sender->sock, "You can't challenge yourself\n");
      return;
   }
   if (sender->opponent != NULL)
   {
      write_client(sender->sock, "You are already in a game\n");
      return;
   }
   if (sender->challengedFrom == sender)
   {
      write_client(sender->sock, "You have already challenged someone. Wait for an answer.\n");
      return;
   }
   if (sender->challengedFrom != NULL)
   {
      write_client(sender->sock, "You have already been challenged. First answer with /yes or /no\n");
      return;
   }

   for (i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         found = 1;
         if (clients[i].opponent != NULL || clients[i].challengedFrom != NULL)
         {
            write_client(sender->sock, "This client may already be in a game\n");
            return;
         }

         if (clients[i].private == 1)
         {
            int isFriend = 0;
            for (int j = 0; j < sender->actualFriends; j++)
            {
               if (strcmp(sender->friends[j], clients[i].name) == 0)
               {
                  isFriend = 1;
                  break;
               }
            }
            if (!isFriend)
            {
               write_client(sender->sock, "This client is in private mode\n");
               return;
            }
         }
         strncpy(message, sender->name, BUF_SIZE - 1);
         strncat(message, " is challenging you!\nWrite /yes to accept an /no to refuse", sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
         clients[i].challengedFrom = sender;
         sender->challengedFrom = sender;
      }
   }
   if (!found)
   {
      write_client(sender->sock, "Client not found\n");
   }
}
static void printTable(const Board *board, int playerNum, int pointsP1, int pointsP2,
                       char *out, int outSize, const char *labelYou, const char *labelRival)
{
   if (out == NULL || outSize <= 0 || board == NULL)
      return;

   int len = 0;
   int yourPoints = (playerNum == 0) ? pointsP1 : pointsP2;
   int rivalPoints = (playerNum == 0) ? pointsP2 : pointsP1;

   len += snprintf(out + len, outSize - len, "%s: %d    %s: %d\n",
                   labelYou, yourPoints, labelRival, rivalPoints);

   len += snprintf(out + len, outSize - len, "    ");
   for (int i = 11; i >= 6 && len < outSize; i--)
   {
      len += snprintf(out + len, outSize - len, " %2d ", board->pits[i]);
   }
   len += snprintf(out + len, outSize - len, "\n");

   // Separator line
   len += snprintf(out + len, outSize - len, "  ");
   for (int i = 0; i < 6 && len < outSize; i++)
   {
      len += snprintf(out + len, outSize - len, "----");
   }
   len += snprintf(out + len, outSize - len, "\n  ");

   for (int i = 0; i <= 5 && len < outSize; i++)
   {
      len += snprintf(out + len, outSize - len, " %2d ", board->pits[i]);
   }
   len += snprintf(out + len, outSize - len, "\n");
}

static int gameLoop(ServerMatch *m, int pit, int playerNum)
{
   if (m == NULL || m->board == NULL)
   {
      printf("match or board not instanciated\n");
      return -1;
   }

   if (!boardIsMoveLegal(m->board, pit, playerNum))
   {
      printf("move %d by player %d not legal\n", pit, playerNum);
      return -1;
   }

   int points = boardMove(m->board, pit);
   if (points < 0)
      return -1;

   // Update scores (boardMove already switched player, so we use the previous player)
   m->scores[playerNum] += points;
   m->joueur = m->board->whoseTurn;

   // Check if game is over
   if (matchIsGameOver(m))
   {
      // Determine winner based on final scores
      if (m->scores[0] > m->scores[1])
         return 1;
      else if (m->scores[1] > m->scores[0])
         return 2;
      else
         return 0;
   }

   return 0; // Game continues
}

static void accept_challenge(ServerMatch *matches, int *currentMatches, Client *sender, const char *buffer)
{
   if (sender->challengedFrom == NULL)
   {
      write_client(sender->sock, "You have no challenge to accept\n");
      return;
   }
   if (sender->challengedFrom == sender)
   {
      write_client(sender->sock, "Wait for your opponent's response\n");
      return;
   }
   if (sender->opponent != NULL)
   {
      write_client(sender->sock, "You are already in a game\n");
      return;
   }

   char message[BUF_SIZE];
   message[0] = 0;

   Client *challenger = sender->challengedFrom;

   sender->challengedFrom = NULL;
   challenger->challengedFrom = NULL;
   sender->opponent = challenger;
   challenger->opponent = sender;

   strncpy(message, sender->name, BUF_SIZE - 1);
   strncat(message, " has accepted your challenge!\n", sizeof message - strlen(message) - 1);
   write_client(challenger->sock, message);
   strncat(message, "Challenge accepted!\n", BUF_SIZE - strlen(message) - 1);
   write_client(sender->sock, message);

   if (rand() % 2 == 0)
   {
      matches[*currentMatches].player1 = challenger;
      matches[*currentMatches].player2 = sender;
      challenger->player = 0; // Player 1
      sender->player = 1;     // Player 2
   }
   else
   {
      matches[*currentMatches].player2 = challenger;
      matches[*currentMatches].player1 = sender;
      sender->player = 0;     // Player 1
      challenger->player = 1; // Player 2
   }

   matchInit(&matches[*currentMatches], *currentMatches, 0, 1, 1);

   if (sender->private == 1 || challenger->private == 1)
   {
      matches[*currentMatches].private = 1;
   }
   else
   {
      matches[*currentMatches].private = 0;
   }

   sender->match = &matches[*currentMatches];
   challenger->match = &matches[*currentMatches];

   strncpy(message, "It's ", BUF_SIZE - 1);
   strncat(message, matches[*currentMatches].player1->name, BUF_SIZE - strlen(message) - 1);
   strncat(message, "'s turn to play\n", BUF_SIZE - strlen(message) - 1);
   write_client(matches[*currentMatches].player2->sock, message);

   write_client(matches[*currentMatches].player1->sock, "It's your turn to play\n");

   char *table = malloc(512 * sizeof(char));

   printTable(matches[*currentMatches].board, challenger->player,
              matches[*currentMatches].scores[0], matches[*currentMatches].scores[1],
              table, 512, "Your points", "Rival");
   write_client(challenger->sock, table);

   printTable(matches[*currentMatches].board, sender->player,
              matches[*currentMatches].scores[0], matches[*currentMatches].scores[1],
              table, 512, "Your points", "Rival");
   write_client(sender->sock, table);

   (*currentMatches)++;
   free(table);
}

static void refuse_challenge(Client *sender, const char *buffer)
{
   // printf("[SERVER DEBUG] refuse_challenge\n");

   if (sender->challengedFrom == NULL)
   {
      write_client(sender->sock, "You have no challenge to refuse\n");
      return;
   }
   if (sender->challengedFrom == sender)
   {
      write_client(sender->sock, "Wait for your opponent's response\n");
      return;
   }
   if (sender->opponent != NULL)
   {
      write_client(sender->sock, "You are already in a game\n");
      return;
   }

   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;

   Client *challenger = sender->challengedFrom;

   sender->challengedFrom = NULL;
   challenger->challengedFrom = NULL;
   strncpy(message, sender->name, BUF_SIZE - 1);
   strncat(message, " has refused your challenge\n", sizeof message - strlen(message) - 1);
   write_client(challenger->sock, message);
   write_client(sender->sock, "Challenge refused\n");

   // printf("[SERVER DEBUG] refuse_challenge end\n");
}

static void play_move(ServerMatch *matches, Client *sender, int relativeMove, int *currentMatches)
{
   if (sender->match == NULL)
   {
      write_client(sender->sock, "You are not in a game. To start one you can !challenge a player\n");
      return;
   }
   if (sender->player != sender->match->joueur)
   {
      printf("%d", sender->player);
      write_client(sender->sock, "It's not your turn to play. Wait for your opponet to play his move\n");
      return;
   }

   int move = relativeMove;
   if (sender->player == PLAYER2)
   {
      move += 6;
   }
   printf("relative move: %d\n", relativeMove);

   // Execute move and check game status
   int gameEnded = gameLoop(sender->match, move, sender->player);

   if (gameEnded == -1)
   {
      write_client(sender->sock, "Invalid move, try again\n");
      return;
   }

   char *table = malloc(512 * sizeof(char));

   if (gameEnded == 0)
   {
      // Game continues
      printTable(sender->match->board, sender->player,
                 sender->match->scores[0], sender->match->scores[1],
                 table, 512, "Your points", "Rival");
      write_client(sender->sock, table);

      printTable(sender->match->board, sender->opponent->player,
                 sender->match->scores[0], sender->match->scores[1],
                 table, 512, "Your points", "Rival");
      write_client(sender->opponent->sock, table);
   }
   else if (gameEnded == 1 || gameEnded == 2)
   {
      // Game ended
      if ((gameEnded == 1 && sender->player == 0) || (gameEnded == 2 && sender->player == 1))
      {
         // Sender won
         printTable(sender->match->board, sender->player,
                    sender->match->scores[0], sender->match->scores[1],
                    table, 512, "Your points", "Rival");
         write_client(sender->sock, table);
         write_client(sender->sock, "You won!\n");

         printTable(sender->match->board, sender->opponent->player,
                    sender->match->scores[0], sender->match->scores[1],
                    table, 512, "Your points", "Rival");
         write_client(sender->opponent->sock, table);
         write_client(sender->opponent->sock, "You lost!\n");
      }
      else
      {
         // Opponent won
         printTable(sender->match->board, sender->opponent->player,
                    sender->match->scores[0], sender->match->scores[1],
                    table, 512, "Your points", "Rival");
         write_client(sender->opponent->sock, table);
         write_client(sender->opponent->sock, "You won!\n");

         printTable(sender->match->board, sender->player,
                    sender->match->scores[0], sender->match->scores[1],
                    table, 512, "Your points", "Rival");
         write_client(sender->sock, table);
         write_client(sender->sock, "You lost!\n");
      }

      remove_match(matches, sender->match, currentMatches);
      matchDestroy(sender->match);

      sender->match = NULL;
      sender->opponent->match = NULL;
      sender->opponent->opponent = NULL;
      sender->opponent = NULL;
   }

   // Notify observers
   if (sender->match != NULL)
   {
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
         if (sender->match->observers[i] != NULL)
         {
            write_client(sender->match->observers[i]->sock, table);
         }
      }
   }

   free(table);
}

static void change_client_profile(Client *sender, const char *new_profile)
{
   strncpy(sender->profile, new_profile, sizeof(sender->profile) - 1);
   sender->profile[sizeof(sender->profile) - 1] = '\0'; // Ensure null-termination
   write_client(sender->sock, "\nProfile updated successfully.\n");
}

static void view_client_profile(Client *clients, Client sender, int actual, const char *name) // TODO POINTER
{
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         if (clients[i].private == 1 && strcmp(sender.name, name) != 0)
         {
            int isFriend = 0;
            for (int j = 0; j < sender.actualFriends; j++)
            {
               if (strcmp(sender.friends[j], clients[i].name) == 0)
               {
                  isFriend = 1;
                  break;
               }
            }
            if (!isFriend)
            {
               write_client(sender.sock, "This client is in private mode\n");
               return;
            }
         }
         char buffer[BUF_SIZE];
         snprintf(buffer, BUF_SIZE, "\nProfile of %s:\n%s\n", clients[i].name, clients[i].profile);
         write_client(sender.sock, buffer);
         return;
      }
   }
   write_client(sender.sock, "Client not found.\n");
}

static void send_list_of_ongoing_matches(ServerMatch *matches, int currentMatches, Client *sender)
{
   write_client(sender->sock, "List of ongoing matches:\n");
   if (currentMatches == 0)
   {
      write_client(sender->sock, "No ongoing matches\n");
      return;
   }
   for (int i = 0; i < currentMatches; i++)
   {
      if (matches[i].private == 1 && strcmp(sender->name, matches[i].player1->name) != 0 && strcmp(sender->name, matches[i].player2->name) != 0)
      {
         int isFriend = 0;
         for (int j = 0; j < sender->actualFriends; j++)
         {
            if (strcmp(sender->friends[j], matches[i].player1->name) == 0 || strcmp(sender->friends[j], matches[i].player2->name) == 0)
            {
               isFriend = 1;
               break;
            }
         }
         if (!isFriend)
         {
            continue;
         }
      }
      char buffer[BUF_SIZE];
      snprintf(buffer, BUF_SIZE, "%s vs %s\n", matches[i].player1->name, matches[i].player2->name);
      write_client(sender->sock, buffer);
   }
}

static void play_command(Client *sender, const char *bufferm, ServerMatch *matches, int *currentMatches, const char *buffer)
{
   // printf("[SERVER DEBUG] %c\n", buffer[6]);
   if (buffer[6] < '0' || buffer[6] > '9')
   {
      write_client(sender->sock, "Invalid command. Correct usage is !play [move] (1-6)\n");
      return;
   }
   int move = buffer[6] - '0';
   play_move(matches, sender, move, currentMatches); // Function takes 0 to 5 as a move
}

static void watch_match(ServerMatch *matches, int currentMatches, Client *sender, const char *buffer)
{
   char *name = buffer + 7;
   ServerMatch *match = NULL;
   for (int i = 0; i < currentMatches; i++)
   {
      if (strcmp(matches[i].player1->name, name) == 0 || strcmp(matches[i].player2->name, name) == 0)
      {
         match = &matches[i];
         break;
      }
   }
   if (match != NULL)
   {
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
         if (match->observers[i] == NULL)
         {
            if (match->private == 1)
            {
               int isFriend = 0;
               for (int j = 0; j < sender->actualFriends; j++)
               {
                  if (strcmp(sender->friends[j], match->player1->name) == 0 || strcmp(sender->friends[j], match->player2->name) == 0)
                  {
                     isFriend = 1;
                     break;
                  }
               }
               if (!isFriend)
               {
                  write_client(sender->sock, "This match is private\n");
                  return;
               }
            }
            match->observers[i] = sender;
            write_client(sender->sock, "You are now watching the match.\n");
            return;
         }
      }
      write_client(sender->sock, "Unable to watch the match. Maximum number of observers reached.\n");
   }
   else
   {
      write_client(sender->sock, "ServerMatch not found.\n");
   }
}

static void send_friend_req(Client *clients, Client *sender, char *name, int actual)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   int found = 0;

   if (strcmp(sender->name, name) == 0)
   {
      write_client(sender->sock, "You can't add yourself as a friend\n");
      return;
   }

   for (i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         found = 1;
         for (int j = 0; j < sender->actualFriends; j++)
         {
            if (sender->friends[j] != NULL && strcmp(sender->friends[j], name) == 0)
            {
               write_client(sender->sock, "You are already friends with this user\n");
               return;
            }
         }
         for (int j = 0; j < sender->actualFriendRequests; j++)
         {
            if (sender->friendRequests[j] != NULL && strcmp(sender->friendRequests[j]->name, name) == 0)
            {
               write_client(sender->sock, "You have already sent a friend request to this user\n");
               return;
            }
         }
         clients[i].friendRequests[sender->actualFriendRequests] = sender;
         clients[i].actualFriendRequests++;
         sender->friendRequestsSent[sender->actualFriendRequestsSent] = &clients[i];
         sender->actualFriendRequestsSent++;

         strncpy(message, sender->name, BUF_SIZE - 1);
         strncat(message, " wants to add you as a friend!\nWrite !accept [name] to accept an !reject [name] to refuse", sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);

         break;
      }
   }
   if (!found)
   {
      write_client(sender->sock, "Client not found\n");
   }
}

static void accept_friend_req(Client *sender, char *name)
{
   // printf("[SERVER DEBUG] accept_friend_req(%s, %s)\n", sender->name, name);

   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   int found = 0;

   for (i = 0; i < sender->actualFriendRequests; i++)
   {
      if (strcmp(sender->friendRequests[i]->name, name) == 0)
      {
         found = 1;
         Client *friend = sender->friendRequests[i];
         strcpy(sender->friends[sender->actualFriends], friend->name);
         strcpy(friend->friends[friend->actualFriends], sender->name);
         sender->actualFriends++;
         sender->friendRequests[i]->actualFriends++;

         memmove(sender->friendRequests + i, sender->friendRequests + i + 1, (sender->actualFriendRequests - i - 1) * sizeof(Client *));
         sender->actualFriendRequests--;

         for (int j = 0; j < friend->actualFriendRequestsSent; j++)
         {
            if (strcmp(friend->friendRequestsSent[j]->name, sender->name) == 0)
            {
               memmove(friend->friendRequestsSent + j, friend->friendRequestsSent + j + 1, (friend->actualFriendRequestsSent - j - 1) * sizeof(Client *));
               friend->actualFriendRequestsSent--;
               break;
            }
         }

         // Send message to both clients
         strncpy(message, sender->name, BUF_SIZE - 1);
         strncat(message, " has accepted your friend request!\n", sizeof message - strlen(message) - 1);
         write_client(friend->sock, message);
         strncpy(message, "Friend request accepted", BUF_SIZE - 1);
         write_client(sender->sock, message);

         break;
      }
   }
   if (!found)
   {
      snprintf(message, sizeof(message), "You don't have a friend request from %s\n", name);
      write_client(sender->sock, message);
   }
}

static void reject_friend_req(Client *sender, char *name)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   int found = 0;

   for (i = 0; i < sender->actualFriendRequests; i++)
   {
      if (strcmp(sender->friendRequests[i]->name, name) == 0)
      {
         found = 1;
         Client *friend = sender->friendRequests[i];

         memmove(sender->friendRequests + i, sender->friendRequests + i + 1, (sender->actualFriendRequests - i - 1) * sizeof(Client *));
         sender->actualFriendRequests--;

         for (int j = 0; j < friend->actualFriendRequestsSent; j++)
         {
            if (strcmp(friend->friendRequestsSent[j]->name, sender->name) == 0)
            {
               memmove(friend->friendRequestsSent + j, friend->friendRequestsSent + j + 1, (friend->actualFriendRequestsSent - j - 1) * sizeof(Client *));
               friend->actualFriendRequestsSent--;
               break;
            }
         }

         // Send message to both clients
         strncpy(message, sender->name, BUF_SIZE - 1);
         strncat(message, " has rejected your friend request\n", sizeof message - strlen(message) - 1);
         write_client(friend->sock, message);
         strncpy(message, "Friend request rejected", BUF_SIZE - 1);
         write_client(sender->sock, message);

         break;
      }
   }
   if (!found)
   {
      snprintf(message, sizeof(message), "You don't have a friend request from %s\n", name);
      write_client(sender->sock, message);
   }
}

static void go_private(Client *sender)
{
   if (sender->private == 1)
   {
      write_client(sender->sock, "You are already in private mode\n");
      return;
   };
   sender->private = 1;
   write_client(sender->sock, "You are now in private mode\n");
   if (sender->actualFriends == 0)
   {
      write_client(sender->sock, "WARNING: You have no friend and will be isolated\n");
   }
}

static void go_public(Client *sender)
{
   if (sender->private == 0)
   {
      write_client(sender->sock, "You are already in public mode\n");
      return;
   };
   sender->private = 0;
   write_client(sender->sock, "You are now in public mode\n");
}

static void command_list(Client *sender)
{
   write_client(sender->sock, "\nList of commands:\n");
   write_client(sender->sock, "/list: list of connected players\n");
   write_client(sender->sock, "/msg [online-player] [message]: send a message to a player\n");
   write_client(sender->sock, "/challenge [online-player]: challenge a player\n");
   write_client(sender->sock, "/update-profile [new-username]: change your username\n");
   write_client(sender->sock, "/profile [username]: view the profile of a player\n");
   write_client(sender->sock, "/matches: list of ongoing matches\n");
   write_client(sender->sock, "/watch [name]: watch a match\n");
   write_client(sender->sock, "/friend [name]: send a friend request\n");
   write_client(sender->sock, "/accept [name]: accept a friend request\n");
   write_client(sender->sock, "/reject [name]: reject a friend request\n");
   write_client(sender->sock, "/private: go in private mode\n");
   write_client(sender->sock, "/public: go in public mode\n");
   write_client(sender->sock, "/msg-all [message]: message all\n");
   write_client(sender->sock, "/commands: list of commands\n");
   write_client(sender->sock, "/play [move]: plays a move in pit number, if game is launched\n");
}

bool login(char *username, char *password)
{
   if (strcmp(username, "test") == 0 || strcmp(password, "test") == 0)
   {
      return true;
   }
   if (strcmp(username, "test2") == 0 || strcmp(password, "test2") == 0)
   {
      return true;
   }
   if (strcmp(username, "test3") == 0 || strcmp(password, "test3") == 0)
   {
      return true;
   }

   bool succeeded = data_login(username, password);
   return succeeded;
}
// Message analyser---------------------------------

static void message_analyzer(ServerMatch *matches, int *currentMatches, Client *clients, Client *sender, int actual, char *buffer)
{
   if (sender->name[0] == '\0')
   {
      // printf("[SERVER DEBUG] %s\n", buffer);
      if (buffer[0] == '/')
      {
         if (strncmp(buffer, "/login ", 7) == 0)
         {
            char *loginfo = buffer + 7;
            // Returns first token
            char *user = strtok(loginfo, " ");
            char *pswrd = strtok(NULL, " ");
            printf("[SERVER DEBUG] %s %s\n", user, pswrd);

            if (user == NULL || pswrd == NULL)
            {
               write_client(sender->sock, "Invalid login. Correct usage is -login [username] [password]\n");
               return;
            }
            if (username_is_name_taken(clients, actual, user))
            {
               write_client(sender->sock, "This user is already being logged\n");
               return;
            }

            if (!login(user, pswrd))
            {
               write_client(sender->sock, "Invalid login\n");
               return;
            }

            strcpy(sender->name, user);
            write_client(sender->sock, "Welcome back, try /commands if it's been a while\n");
            return;
         }
         else if (strncmp(buffer, "/signup ", 8) == 0)
         {
            // printf("[SERVER DEBUG] in signup %s\n", buffer);
            char *loginfo = buffer + 8;
            // Returns first token
            char *user = strtok(loginfo, " ");
            char *pswrd = strtok(NULL, " ");

            if (user == NULL || pswrd == NULL)
            {
               write_client(sender->sock, "Invalid signup. Correct usage is -signup [username] [password]\n");
               return;
            }
            if (!data_signup(user, pswrd))
            {
               write_client(sender->sock, "This user is already taken, usernames are unique. Try an other one!\n");
               return;
            }

            strcpy(sender->name, user);
            write_client(sender->sock, "Welcome, try -commands if it's been a while\n");
            return;
         }
      }

      write_client(sender->sock, "Try /login or /signup to continue\n");
      return;
   }

   printf("[SERVER DEBUG] %s\n", buffer);
   if (buffer[0] == '/')
   {
      if (strncmp(buffer, "/list", 5) == 0)
      {
         list_of_online_clients(clients, *sender, actual);
      }

      else if (strncmp(buffer, "/msg ", 5) == 0)
      {
         char *name = buffer + 5;
         char *msg = strchr(name, ' ');
         if (msg != NULL)
         {
            *msg = 0;
            msg++;
            send_message_to_client_by_name(clients, name, actual, msg, sender);
         }
         else
         {
            write_client(sender->sock, "Usage: /msg [online-client] [message]\n");
         }
      }

      else if (strncmp(buffer, "/msg-all ", 8) == 0)
      {
         char *message = buffer + 8;
         if (message == NULL)
         {
            write_client(sender->sock, "Usage: /msg-all [message]");
         }
         else
         {
            write_client(sender->sock, "Message sent to all");
            send_message_to_all_clients(clients, *sender, actual, buffer, 0);
         }
      }

      else if (strncmp(buffer, "/challenge ", 11) == 0)
      {
         char *name = buffer + 11;
         if (name == NULL)
         {
            write_client(sender->sock, "Usage: /challenge [online-player]");
         }
         else
         {
            write_client(sender->sock, "Player challenged");
            challenge(clients, sender, name, actual, buffer);
         }
      }

      else if (strncmp(buffer, "/play-move ", 9) == 0)
      {
         char *name = buffer + 9;
         if (name == NULL)
         {
            write_client(sender->sock, "Usage: /play-move [move]");
         }
         else
         {
            challenge(clients, sender, name, actual, buffer);
         }
      }

      else if (strncmp(buffer, "/yes ", 4) == 0)
      {
         accept_challenge(matches, currentMatches, sender, buffer);
      }
      else if (strncmp(buffer, "/no ", 3) == 0)
      {
         refuse_challenge(sender, buffer);
      }

      else if (strncmp(buffer, "/play ", 5) == 0)
      {
         char *move = buffer + 5;
         play_command(sender, move, matches, currentMatches, buffer);
      }

      else if (strncmp(buffer, "/update-profile ", 15) == 0)
      {
         char *new_profile = buffer + 12;
         if (new_profile == NULL)
         {
            write_client(sender->sock, "Usage: /update-profile [new-username]");
         }
         else
         {
            change_client_profile(sender, new_profile);
         }
      }

      else if (strncmp(buffer, "/profile ", 9) == 0)
      {
         char *name = buffer + 9;
         if (name == NULL)
         {
            write_client(sender->sock, "Usage: /profile [username]");
         }
         view_client_profile(clients, *sender, actual, name);
      }
      else if (strncmp(buffer, "/matches", 7) == 0)
      {
         send_list_of_ongoing_matches(matches, *currentMatches, sender);
      }
      else if (strncmp(buffer, "/watch ", 7) == 0)
      {
         watch_match(matches, *currentMatches, sender, buffer);
      }
      else if (strncmp(buffer, "/friend ", 8) == 0)
      {
         char *name = buffer + 8;
         send_friend_req(clients, sender, name, actual);
      }
      else if (strncmp(buffer, "/accept ", 8) == 0)
      {
         char *name = buffer + 8;
         accept_friend_req(sender, name);
      }
      else if (strncmp(buffer, "/reject ", 7) == 0)
      {
         char *name = buffer + 7;
         reject_friend_req(sender, name);
      }
      else if (strncmp(buffer, "/private", 8) == 0)
      {
         go_private(sender);
      }
      else if (strncmp(buffer, "/public", 7) == 0)
      {
         go_public(sender);
      }
      else if (strncmp(buffer, "/command", 8) == 0 || strncmp(buffer, "/commands", 9) == 0)
      {
         command_list(sender);
      }
      else
      {
         write_client(sender->sock, "Unknown command\n");
      }
   }
   else
   {
      write_client(sender->sock, "Command not recognized");
   }
}
