#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "server.h"
#include "client.h"
#include "group.h"

static void init(const char *executable_file)
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
   /*init the server root directory path*/
   int root_dir_path_len = position(executable_file, '/',1);
   root = (char *)malloc(sizeof(char) * root_dir_path_len);
   strncpy(root, executable_file, root_dir_path_len);
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
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   /* an array for all groups */
   Group groups[MAX_GROUPS];
   int nbGroups = 0;

   fd_set rdfs;

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if (read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* checks that username not already used*/
         bool used = false;

         for (i = 0; i < actual; i++)
         {
            if (strcmp(buffer, clients[i].name) == 0)
            {
               write_client(csock, "This username is already being used, please try to connect with another one.");
               closesocket(csock);
               used = true;
               break;
            }
         }

         /* if usernamed used we don't add the client to the array*/
         if (used)
         {
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = {csock};
         strncpy(c.name, buffer, BUF_SIZE - 1);
         clients[actual] = c;

         printf("%s connected %s", c.name, CRLF); /*server log*/

         actual++;
         //send_history(c);
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  closesocket(clients[i].sock);

                  printf("%s disconnected%s", clients[i].name, CRLF); /*server log*/

                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else if (strncmp(buffer,"create public group",12)==0)
               {
                  create_public_group(groups, nbGroups, client, buffer);
                  nbGroups++;
                  int i = 0;
                  /*
                  printf("nb groupes après création : %d %s",nbGroups,CRLF);
                  printf("nb membres du groupe 0 : %d %s",groups[0].nbMembres,CRLF);
                  for (i = 0; i < groups[0].nbMembres;i++)
                  {
                     printf("membre %d : %s %s",i,groups[0].membres[i].name,CRLF);
                  }
                  */
               }
               else if (strncmp(buffer,"join",4)==0)
               {
                  add_member_to_public_group(groups, nbGroups, client, buffer);
               }
               else if (strncmp(buffer, "@", 1) == 0)
               {
                  send_message_to_one_client(clients, client, actual, buffer);
               }
               else if (strncmp(buffer,"~",1) == 0)
               {
                   send_message_to_a_group(clients,client,groups,nbGroups,buffer);
               }
               break;
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

static char *get_date_heure()
{
   int h, min, s, day, mois, an;
   time_t now;
   time(&now);
   struct tm *local = localtime(&now);
   h = local->tm_hour;
   min = local->tm_min;
   s = local->tm_sec;
   day = local->tm_mday;
   mois = local->tm_mon + 1;
   an = local->tm_year + 1900;
   char *date = (char *)malloc(30);
   sprintf(date, "%02d/%02d/%d %02d:%02d:%02d", day, mois, an, h, min, s);
   return date;
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void push_history(const char *client_name, const char *message)
{

   char *filename = (char *)malloc(strlen(root) + strlen(HISTORIES_DIR) + strlen(client_name) + strlen(HISTORY_FILENAME) + 3);

   sprintf(filename, "%s%c%s", root, '/', HISTORIES_DIR);

   if (opendir(filename) == NULL)
   {
      mkdir(filename, 0700);
   }

   sprintf(filename, "%s%c%s", filename, '/', client_name);

   if (opendir(filename) == NULL)
   {
      mkdir(filename, 0700);
   }
   FILE *fptr;

   sprintf(filename, "%s%c%s", filename, '/', HISTORY_FILENAME);

   if ((fptr = fopen(filename, "a")) != NULL)
   {
      fputs(message, fptr);
      fputs(CRLF, fptr);
   }
   free(filename);
   fclose(fptr);
}

static int get_client_from_name(Client *clients, int actual, const char* client_name){
   for(int i=0; i<actual; i++){
      if(!strcmp(clients[i].name, client_name)){
         return i;
      }
   }
   return -1;
}

static int get_group_from_name(Group *groups, int nbGroups, const char* group_name){
   for(int i=0; i<nbGroups; i++){
      if(!strcmp(groups[i].name, group_name)){
         return i;
      }
   }
   return -1;
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
         strncpy(message, "", BUF_SIZE - 1);
         if (from_server == 0)
         {
            strncpy(message, get_date_heure(), BUF_SIZE - 1);
            strncat(message, " - ", sizeof message - strlen(message) - 1);
            strncat(message, sender.name, sizeof message - strlen(message) - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);

         push_history(clients[i].name, message);

         write_client(clients[i].sock, message);
      }
   }
}

// renvoie la position d'un charactère dans une chaine, si non-présent renvoie -1
// si last = 0 renvoie la premiere occurence du caractère sinon renvoie la dernière
static int position(const char *chaine, char carac, int last)
{
   char *found;
   if (!last)
   {
      found = strchr(chaine, carac);
   }
   else
   {
      found = strrchr(chaine, carac);
   }
   
   if (found == NULL)
   {
      return -1;
   }
   return (found - chaine) / sizeof(char);

   /*  int taille = strlen(chaine);
     int pos = 0;
     int i = 0;
     for (i = 0; i < taille; i++)
     {
        if (chaine[i] == carac)
        {
           return i;
        }
     }
     return -1;*/
}

static void send_message_to_one_client(Client *clients, Client sender, int actual, const char *buffer)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   int pos = position(buffer, ' ', 0);

   // si il y a un message après le nom du destinataire
   if (pos != -1)
   {
      //il faut checker qu'il est dans la base de données
      int pos2 = 0;
      char name[100];

      strncpy(name, buffer + 1, pos - 1);

      strncpy(message, get_date_heure(), BUF_SIZE - 1);
      strncat(message, " - ", sizeof message - strlen(message) - 1);
      printf("%s\n", message);
      strncat(message, sender.name, sizeof message - strlen(message) - 1);
      strncat(message, " :", sizeof message - strlen(message) - 1);
      char newmessage[strlen(buffer)];
      strcpy(newmessage, buffer + pos);
      strncat(message, newmessage, sizeof message - strlen(message) - 1);

      pos2 = get_client_from_name(clients, actual, name);
      // si le client est connecté
      if (pos2 != -1)
      {
         write_client(clients[pos2].sock, message);
      }
      push_history(name, message);
   }
}

static void send_message_to_a_group(Client *clients, Client sender, Group *groups, int nbGroups, const char *buffer)
{

   /*find the group from name in the array*/
   char name[MAX_NAME]; 
   int pos = position(buffer,' ');
   strncpy(name,buffer+1,pos-1);
   //printf("nom récupéré à aller comparer(envoie de mess) : %s %s",name,CRLF);
   int pos2 = get_group_from_name(groups,nbGroups,name);
   //printf("position récupérée : %d %s",pos2,CRLF);

   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < groups[pos2].nbMembres; i++)
   {
      //printf("membre %d : %s %s",i,groups[pos2].membres[i].name,CRLF);
      /* we don't send message to the sender */
      if (sender.sock != groups[pos2].membres[i].sock)
      {
         strcpy(message,"[");
         strcat(message,name);
         strcat(message,"] ");
         strncat(message, sender.name, BUF_SIZE - 1);
         strncat(message, " :", sizeof message - strlen(message) - 1);
         char newmessage[strlen(buffer)];
         strcpy(newmessage,buffer+pos);
         strncat(message, newmessage, sizeof message - strlen(message) - 1);
         write_client(groups[pos2].membres[i].sock, message);
      }
   }
}

static void create_public_group(Group * groups, int nbGroups, Client creator, const char* buffer)
{
   //donne le nom au groupe
   //int pos = position(buffer+12)
   strncpy(groups[nbGroups].name,buffer+20,MAX_NAME-1);
   //printf("nom du groupe : %s %s",groups[nbGroups].name,CRLF);
   
   //rajoute le créateur au groupe
   groups[nbGroups].membres[0] = creator;
   groups[nbGroups].nbMembres = (groups[nbGroups].nbMembres+1);


}

static void add_member_to_public_group(Group * groups, int nbGroups, Client joiner, const char* buffer)
{
   char name[MAX_NAME]; 
   strncpy(name,buffer+5,strlen(buffer)-4);
   //printf("nom récupéré à aller comparer : %s %s",name,CRLF);
   int pos;
   pos = get_group_from_name(groups,nbGroups,name);
   //printf("position du groupe : %d %s",pos,CRLF);
   //printf("nb membres avant ajout : %d %s",groups[pos].nbMembres, CRLF);
   //à gérer
   if (pos == 9)
   {
   }
   else 
   {
   groups[pos].membres[groups[pos].nbMembres] = joiner;
   groups[pos].nbMembres = (groups[pos].nbMembres+1);
   }
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

   int optval = 1;
   setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int *));
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
   init(argv[0]);

   app();

   end();

   return EXIT_SUCCESS;
}
