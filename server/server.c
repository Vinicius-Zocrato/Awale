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
#include "../src/match.h"
#include "../src/player.h"
#include "../src/board.h"

// #include "../data/users.h"

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

   load_users();
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
   save_users();
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

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
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
               int c = read_client(clients[i].sock, buffer);

               // Client déconnecté
               if (c == 0)
               {
                  client_disconnected(client, matches, &actualMatches);
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
               }
               else
               {
                  message_analyser(matches, &actualMatches, clients, client, actual, buffer);
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
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void remove_match(ServerMatch *matches, ServerMatch *match, int *currentMatches)
{
   // Get index of match
   int to_remove = -1;
   for (int i = 0; i < *currentMatches; i++)
   {
      if (matches[i].sud == match->sud && matches[i].nord == match->nord)
      {
         to_remove = i;
         break;
      }
   }
   if (to_remove != -1)
   {
      /* we remove the match in the array */
      memmove(matches + to_remove, matches + to_remove + 1, (*currentMatches - to_remove - 1) * sizeof(ServerMatch));
      /* number of matches - 1 */
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
      /* we don't send message to the sender */
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

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
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
static void list_of_online_clients(Client *clients, Client sender, int actual) // TODO: pointer
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
      write_client(sender->sock, "You have already been challenged. First answer with !yes or !no\n");
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
         strncat(message, " is challenging you!\nWrite !yes to accept an !no to refuse", sizeof message - strlen(message) - 1);
         // strncat(message, buffer, sizeof message - strlen(message) - 1);
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

static void accept_challenge(ServerMatch *matches, int *currentMatches, Client *sender, const char *buffer)
{
   // printf("[SERVER DEBUG] accept_challenge\n");

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

   int i = 0;
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
      matches[*currentMatches].sud = challenger;
      matches[*currentMatches].nord = sender;
   }
   else
   {
      matches[*currentMatches].nord = challenger;
      matches[*currentMatches].sud = sender;
   }
   for (int j = 0; j < 12; j++)
   {
      matches[*currentMatches].tab[j] = 4;
   }

   matches[*currentMatches].joueur = PLAYER1;
   matches[*currentMatches].pointsNord = 0;
   matches[*currentMatches].pointsSud = 0;
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
   challenger->player = PLAYER1;
   sender->player = PLAYER2;

   strncpy(message, "It's ", BUF_SIZE - 1);
   strncat(message, matches[*currentMatches].sud->name, BUF_SIZE - strlen(message) - 1);
   strncat(message, "'s turn to play\n", BUF_SIZE - strlen(message) - 1);
   write_client(matches[*currentMatches].nord->sock, message);

   write_client(matches[*currentMatches].sud->sock, "It's your turn to play\n");

   char *table = malloc(256 * sizeof(char));

   printTable(matches[*currentMatches].tab, challenger->player, matches[*currentMatches].pointsSud, matches[*currentMatches].pointsNord, table, 256, "Your points", "Rival");
   write_client(challenger->sock, table);

   printTable(matches[*currentMatches].tab, sender->player, matches[*currentMatches].pointsSud, matches[*currentMatches].pointsNord, table, 256, "Your points", "Rival");
   write_client(sender->sock, table);

   (*currentMatches)++;
   free(table);

   // printf("[SERVER DEBUG] accept_challenge end\n");
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

static void play_move(ServerMatch *matches, Client *sender, int relatifMove, int *currentMatches)
{
   // printf("[SERVER DEBUG] play_move\n");
   // printf("[SERVER DEBUG] relatifMove: %d\n", relatifMove);
   if (sender->match == NULL)
   {
      write_client(sender->sock, "You are not in a game. To start one you can !challenge a player\n");
      return;
   }
   if (sender->player != sender->match->joueur)
   {
      write_client(sender->sock, "It's not your turn to play. Wait for your opponet to play his move\n");
      return;
   }

   int move = sender->player == PLAYER1 ? relatifMove : relatifMove + 6;
   int gameEnded = gameLoop(sender->match->tab, &(sender->match->joueur), move, &(sender->match->pointsSud), &(sender->match->pointsNord));

   if (gameEnded == -1)
   {
      write_client(sender->sock, "Invalid move, try again\n");
      return;
   }
   else if (gameEnded == 0)
   {
      // printf("[gameEnded DEBUG] sender->player %i\n", sender->player);
      // printf("[gameEnded DEBUG] sender->opponent->player %i\n", sender->opponent->player);
      // printf("[gameEnded DEBUG] pointsSud %i, pointsNord %i\n", sender->match->pointsSud, sender->match->pointsNord);
      char *table = malloc(256 * sizeof(char));
      printTable(sender->match->tab, sender->player, sender->match->pointsSud, sender->match->pointsNord, table, 256, "Your points", "Rival");
      write_client(sender->sock, table);
      printTable(sender->match->tab, sender->opponent->player, sender->match->pointsSud, sender->match->pointsNord, table, 256, "Your points", "Rival");
      write_client(sender->opponent->sock, table);
      free(table);
   }
   else if (gameEnded == 1 || gameEnded == 2)
   {
      char *table = malloc(256 * sizeof(char));
      if ((gameEnded == 1 && sender->player == PLAYER1) || (gameEnded == 2 && sender->player == PLAYER2)) // Sender won
      {
         printTable(sender->match->tab, sender->player, sender->match->pointsSud, sender->match->pointsNord, table, 256, "Your points", "Rival");
         write_client(sender->sock, table);
         write_client(sender->sock, "You won!\n");
         printTable(sender->match->tab, sender->opponent->player, sender->match->pointsSud, sender->match->pointsNord, table, 256, "Your points", "Rival");
         write_client(sender->opponent->sock, table);
         write_client(sender->opponent->sock, "You lost!\n");
      }
      else // Opponent won
      {
         printTable(sender->match->tab, sender->opponent->player, sender->match->pointsSud, sender->match->pointsNord, table, 256, "Your points", "Rival");
         write_client(sender->opponent->sock, table);
         write_client(sender->opponent->sock, "You won!\n");
         printTable(sender->match->tab, sender->player, sender->match->pointsSud, sender->match->pointsNord, table, 256, "Your points", "Rival");
         write_client(sender->sock, table);
         write_client(sender->sock, "You lost!\n");
      }

      remove_match(matches, sender->match, &currentMatches);

      sender->match = NULL;
      sender->opponent->match = NULL;
      sender->opponent->opponent = NULL;
      sender->opponent = NULL;
      free(table);
   }
   if (1)
   {
      char *table = malloc(256 * sizeof(char));
      printTable(sender->match->tab, sender->player, sender->match->pointsSud, sender->match->pointsNord, table, 256, sender->name, sender->opponent->name);
      // printf("[gameEnded DEBUG] sender->match->observers %i\n", sender->match->observers[0]);
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
         if (sender->match->observers[i] != NULL)
         {
            write_client(sender->match->observers[i]->sock, table);
         }
      }
      free(table);
   }
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
      if (matches[i].private == 1 && strcmp(sender->name, matches[i].sud->name) != 0 && strcmp(sender->name, matches[i].nord->name) != 0)
      {
         int isFriend = 0;
         for (int j = 0; j < sender->actualFriends; j++)
         {
            if (strcmp(sender->friends[j], matches[i].sud->name) == 0 || strcmp(sender->friends[j], matches[i].nord->name) == 0)
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
      snprintf(buffer, BUF_SIZE, "%s vs %s\n", matches[i].sud->name, matches[i].nord->name);
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
   play_move(matches, sender, move - 1, currentMatches); // Function takes 0 to 5 as a move
}

static void watch_match(ServerMatch *matches, int currentMatches, Client *sender, const char *buffer)
{
   char *name = buffer + 7;
   ServerMatch *match = NULL;
   for (int i = 0; i < currentMatches; i++)
   {
      if (strcmp(matches[i].sud->name, name) == 0 || strcmp(matches[i].nord->name, name) == 0)
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
                  if (strcmp(sender->friends[j], match->sud->name) == 0 || strcmp(sender->friends[j], match->nord->name) == 0)
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

         /*printf("[SERVER DEBUG] Sender: %s\n", sender->name);
         printf("[SERVER DEBUG] Friends:\n");
         for(int j = 0; j < sender->actualFriends; j++)
         {
            printf("[SERVER DEBUG] \t%s\n", sender->friends[j]);
         }
         printf("[SERVER DEBUG] Friend requests:\n");
         for(int j = 0; j < sender->actualFriendRequests; j++)
         {
            printf("[SERVER DEBUG] \t%s\n", sender->friendRequests[j]->name);
         }
         printf("[SERVER DEBUG] Friend requests sent:\n");
         for(int j = 0; j < sender->actualFriendRequestsSent; j++)
         {
            printf("[SERVER DEBUG] \t%s\n", sender->friendRequestsSent[j]->name);
         }

         printf("[SERVER DEBUG] Client: %s\n", clients[i].name);
         printf("[SERVER DEBUG] Friends:\n");
         for(int j = 0; j < clients[i].actualFriends; j++)
         {
            printf("[SERVER DEBUG] \t%s\n", clients[i].friends[j]);
         }
         printf("[SERVER DEBUG] Friend requests:\n");
         for(int j = 0; j < clients[i].actualFriendRequests; j++)
         {
            printf("[SERVER DEBUG] \t%s\n", clients[i].friendRequests[j]->name);
         }
         printf("[SERVER DEBUG] Friend requests sent:\n");
         for(int j = 0; j < clients[i].actualFriendRequestsSent; j++)
         {
            printf("[SERVER DEBUG] \t%s\n", clients[i].friendRequestsSent[j]->name);
         }*/

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

         // Remove friend request
         memmove(sender->friendRequests + i, sender->friendRequests + i + 1, (sender->actualFriendRequests - i - 1) * sizeof(Client *));
         sender->actualFriendRequests--;
         // Remove friend request sent
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

         // Remove friend request
         memmove(sender->friendRequests + i, sender->friendRequests + i + 1, (sender->actualFriendRequests - i - 1) * sizeof(Client *));
         sender->actualFriendRequests--;
         // Remove friend request sent
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
   write_client(sender->sock, "!list: list of connected clients\n");
   write_client(sender->sock, "!msg <name> <message>: send a message to a client\n");
   write_client(sender->sock, "!challenge <name>: challenge a client\n");
   write_client(sender->sock, "!newprofile <profile>: change your profile\n");
   write_client(sender->sock, "!profile <name>: view the profile of a client\n");
   write_client(sender->sock, "!matches: list of ongoing matches\n");
   write_client(sender->sock, "!watch <name>: watch a match\n");
   write_client(sender->sock, "!friend <name>: send a friend request\n");
   write_client(sender->sock, "!accept <name>: accept a friend request\n");
   write_client(sender->sock, "!reject <name>: reject a friend request\n");
   write_client(sender->sock, "!private: go in private mode\n");
   write_client(sender->sock, "!public: go in public mode\n");
   write_client(sender->sock, "!commands: list of commands\n");
}

// Message analyser-----------------------------------------------------------------------------------------------------------------

static void message_analyser(ServerMatch *matches, int *currentMatches, Client *clients, Client *sender, int actual, const char *buffer)
{
   if (sender->name[0] == '\0')
   {
      // printf("[SERVER DEBUG] %s\n", buffer);
      if (buffer[0] == '!')
      {
         if (strncmp(buffer, "!login ", 7) == 0)
         {
            char *loginfo = buffer + 7;
            // Returns first token
            char *user = strtok(loginfo, " ");
            char *pswrd = strtok(NULL, " ");
            // printf("[SERVER DEBUG] %s %s\n", user, pswrd);

            if (user == NULL || pswrd == NULL)
            {
               write_client(sender->sock, "Invalid login. Correct usage is !login [username] [password]\n");
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
            write_client(sender->sock, "Welcome back, try !commands if it's been a while\n");
            return;
         }
         else if (strncmp(buffer, "!singin ", 8) == 0)
         {
            // printf("[SERVER DEBUG] in singin %s\n", buffer);
            char *loginfo = buffer + 8;
            // Returns first token
            char *user = strtok(loginfo, " ");
            char *pswrd = strtok(NULL, " ");

            if (user == NULL || pswrd == NULL)
            {
               write_client(sender->sock, "Invalid singin. Correct usage is !singin [username] [password]\n");
               return;
            }
            if (!singin(user, pswrd))
            {
               write_client(sender->sock, "This user is already taken, usernames are unique. Try an other one!\n");
               return;
            }

            // printf("[SERVER DEBUG] %s %s\n", user, pswrd);
            strcpy(sender->name, user);
            write_client(sender->sock, "Welcome, try !commands if it's been a while\n");
            return;
         }
      }

      write_client(sender->sock, "Try !login or !singin to continue\n");
      return;
   }
   // printf("[SERVER DEBUG] %s\n", buffer);
   if (buffer[0] == '!')
   {
      if (strncmp(buffer, "!list", 5) == 0)
      {
         list_of_online_clients(clients, *sender, actual);
      }

      else if (strncmp(buffer, "!msg ", 5) == 0)
      {
         char *name = buffer + 5;
         char *msg = strchr(name, ' ');
         if (msg != NULL)
         {
            *msg = 0;
            msg++;
            send_message_to_client_by_name(clients, name, actual, msg, sender);
         }
      }

      else if (strncmp(buffer, "!challenge ", 11) == 0)
      {
         char *name = buffer + 11;
         challenge(clients, sender, name, actual, buffer);
      }
      else if (strncmp(buffer, "!yes ", 4) == 0)
      {
         accept_challenge(matches, currentMatches, sender, buffer);
      }
      else if (strncmp(buffer, "!no ", 3) == 0)
      {
         refuse_challenge(sender, buffer);
      }
      else if (strncmp(buffer, "!play ", 5) == 0)
      {
         char *move = buffer + 5;
         play_command(sender, move, matches, currentMatches, buffer);
      }
      else if (strncmp(buffer, "!newprofile ", 12) == 0)
      {
         char *new_profile = buffer + 12;
         change_client_profile(sender, new_profile);
      }
      else if (strncmp(buffer, "!profile ", 9) == 0)
      {
         char *name = buffer + 9;
         view_client_profile(clients, *sender, actual, name);
      }
      else if (strncmp(buffer, "!matches", 7) == 0)
      {
         send_list_of_ongoing_matches(matches, *currentMatches, sender);
      }
      else if (strncmp(buffer, "!watch ", 7) == 0)
      {
         watch_match(matches, *currentMatches, sender, buffer);
      }
      else if (strncmp(buffer, "!friend ", 8) == 0)
      {
         char *name = buffer + 8;
         send_friend_req(clients, sender, name, actual);
      }
      else if (strncmp(buffer, "!accept ", 8) == 0)
      {
         char *name = buffer + 8;
         accept_friend_req(sender, name);
      }
      else if (strncmp(buffer, "!reject ", 7) == 0)
      {
         char *name = buffer + 7;
         reject_friend_req(sender, name);
      }
      else if (strncmp(buffer, "!private", 8) == 0)
      {
         go_private(sender);
      }
      else if (strncmp(buffer, "!public", 7) == 0)
      {
         go_public(sender);
      }
      else if (strncmp(buffer, "!command", 8) == 0)
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
      send_message_to_all_clients(clients, *sender, actual, buffer, 0);
   }
}
