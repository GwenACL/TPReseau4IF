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
   int root_dir_path_len = position(executable_file, '/', 1);
   root = (char *)malloc(sizeof(char) * root_dir_path_len);
   strncpy(root, executable_file, root_dir_path_len);
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
   free(root);
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
      strcpy(buffer, "");
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
         send_history(c);
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
               }
               else if (strncmp(buffer,"create public group",19)==0)
               {
                  create_public_group(groups, nbGroups, client, buffer);
                  nbGroups++;
               }
               else if (strncmp(buffer,"create private group",20)==0)
               {
                  create_private_group(groups, nbGroups, client, buffer);
                  nbGroups++;
               }
               else if (strncmp(buffer,"join public group",17)==0)
               {
                  add_member_to_public_group(groups, nbGroups, client, buffer);
               }
               else if (strncmp(buffer,"join private group",18)==0)
               {
                  add_member_to_private_group(groups, nbGroups, client, buffer);
               }
               else if (strncmp(buffer,"@",1) == 0)
               {
                  send_message_to_one_client(clients, client, actual, buffer);
               }
               else if (strncmp(buffer, "~", 1) == 0)
               {
                  send_message_to_a_group(clients, client, groups, nbGroups, buffer);
               }
               else if (strncmp(buffer,"available",1) == 0)
               {
                  see_connected(clients,actual,groups,nbGroups,client);
               }
               else
               {
                  write_client(client.sock, "Invalid command");
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

   DIR *history_dir = opendir(filename);
   if (history_dir == NULL)
   {
      mkdir(filename, 0700);
   }
   closedir(history_dir);

   sprintf(filename, "%s%c%s", filename, '/', client_name);

   history_dir = opendir(filename);
   if (history_dir == NULL)
   {
      mkdir(filename, 0700);
   }
   closedir(history_dir);
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

static void save_sender_message(const char *sender_name, const char *message)
{
   char saved_message[BUF_SIZE];
   saved_message[0] = 0;
   char *date = get_date_heure();
   sprintf(saved_message, "%s%s%s%s", message, " (sending datetime : ", date, " )");
   push_history(sender_name, saved_message);
   free(date);
}

static void send_history(Client client)
{
    char message[BUF_SIZE];
   message[0] = 0;

   char *filename = (char *)malloc(strlen(root) + strlen(HISTORIES_DIR) + strlen(client.name) + strlen(HISTORY_FILENAME) + 3);
   sprintf(filename, "%s%c%s", root, '/', HISTORIES_DIR);
   DIR *history_dir = opendir(filename);
   if (history_dir == NULL)
   {
      return;
   }
   closedir(history_dir);

   sprintf(filename, "%s%c%s", filename, '/', client.name);

   DIR *client_dir = opendir(filename);
   if (client_dir == NULL)
   {
      return;
   }
   closedir(client_dir);

   sprintf(filename, "%s%c%s", filename, '/', HISTORY_FILENAME);

   FILE *history_file = fopen(filename, "r");
   free(filename);
   if (history_file)
   {
      fgets(message, BUF_SIZE, history_file);
      while (!feof(history_file))
      {
         write_client(client.sock, message);
         fgets(message, BUF_SIZE, history_file);
      }
   }
   fclose(history_file);
}

static int get_client_from_name(Client *clients, int actual, const char *client_name)
{
   for (int i = 0; i < actual; i++)
   {
      if (!strcmp(clients[i].name, client_name))
      {
         return i;
      }
   }
   return -1;
}

static int get_group_from_name(Group *groups, int nbGroups, const char *group_name)
{
   for (int i = 0; i < nbGroups; i++)
   {
      if (!strcmp(groups[i].name, group_name))
      {
         return i;
      }
   }
   return -1;
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
}

static void send_message_to_one_client(Client *clients, Client sender, int actual, const char *buffer)
{

   save_sender_message(sender.name, buffer);
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   int pos = position(buffer, ' ', 0);
   printf("pos : %d %s", pos, CRLF);

   // si il y a un message après le nom du destinataire
   if (pos != -1)
   {
      int pos2 = 0;
      char name[100];

      strcpy(name,"");
      strncat(name, buffer + 1, pos - 1);

      char *date = get_date_heure();
      strncpy(message, date, BUF_SIZE - 1);
      free(date);
      strncat(message, " - ", sizeof message - strlen(message) - 1);
      strncat(message, sender.name, sizeof message - strlen(message) - 1);
      strncat(message, " :", sizeof message - strlen(message) - 1);
      char newmessage[strlen(buffer)];
      strcpy(newmessage, buffer + pos);
      strncat(message, newmessage, sizeof message - strlen(message) - 1);

      pos2 = get_client_from_name(clients, actual, name);
      printf("pos2 : %d %s", pos2, CRLF);

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

   save_sender_message(sender.name, buffer);
   /*find the group from name in the array*/
   char name2[MAX_NAME];
   strcpy(name2, "");
   int pos = position(buffer, ' ', 0);
   strncat(name2, buffer + 1, pos - 1);
   strcat(name2, "\0");
   int pos2 = get_group_from_name(groups, nbGroups, name2);

   /* we make sure that the sender is part of the group*/
   bool part = false;
   int i = 0;
   for (i = 0; i < groups[pos2].nbMembres; i++)
   {
      if (groups[pos2].membres[i].sock == sender.sock)
      {
         part = true;
         break;
      }
   }

   if (part)
   {
      char message[BUF_SIZE];
      message[0] = 0;
      for (i = 0; i < groups[pos2].nbMembres; i++)
      {
         /* we don't send message to the sender */
         if (sender.sock != groups[pos2].membres[i].sock)
         {
            char *date = get_date_heure();
            strncpy(message, date, BUF_SIZE - 1);
            free(date);
            strncat(message, " - ", sizeof message - strlen(message) - 1);
            strncat(message, "[", sizeof message - strlen(message) - 1);
            strcat(message, name2);
            strcat(message, "] ");
            strncat(message, sender.name, BUF_SIZE - 1);
            strncat(message, " :", sizeof message - strlen(message) - 1);
            char newmessage[strlen(buffer)];
            strcpy(newmessage, buffer + pos);
            strncat(message, newmessage, sizeof message - strlen(message) - 1);
            write_client(groups[pos2].membres[i].sock, message);
            push_history(groups[pos2].membres[i].name, message);
         }
      }
   }

   else
   {
      write_client(sender.sock, "You cannot send a message to a group you're not a part of.");
   }
}

static void create_public_group(Group *groups, int nbGroups, Client creator, const char *buffer)
{
   // adds name to the group
   strncpy(groups[nbGroups].name, buffer + 20, MAX_NAME - 1);

   // adds creator to the group
   groups[nbGroups].membres[0] = creator;
   groups[nbGroups].nbMembres = (groups[nbGroups].nbMembres+1);

   //sets group as public
   groups[nbGroups].private = false;


}

static void create_private_group(Group * groups, int nbGroups, Client creator, const char* buffer)
{
   //adds name and password to the group
   int pos = position(buffer+21,' ',0);
   strncpy(groups[nbGroups].name,buffer+21,pos);
   strncat(groups[nbGroups].password,buffer+22+pos,BUF_SIZE-1);

   
   //adds creator to the group
   groups[nbGroups].membres[0] = creator;
   groups[nbGroups].nbMembres = (groups[nbGroups].nbMembres+1);

      //sets group as public
   groups[nbGroups].private = true;


}

static void add_member_to_public_group(Group *groups, int nbGroups, Client joiner, const char *buffer)
{
   char name[MAX_NAME]; 
   strcpy(name,"");
   strncat(name,buffer+18,strlen(buffer)-18);
   int pos;
   pos = get_group_from_name(groups,nbGroups,name);
   if (pos == -1)
   {
      write_client(joiner.sock,"There is no group with that name. Use 'availables' command to see online groups and members.");
   }
   else 
   {
      int i;
      bool part = false;
      for(i = 0; i < groups[pos].nbMembres; i++)
      {
         if (joiner.sock == groups[pos].membres[i].sock)
         {
            write_client(joiner.sock,"You already are part of the group.");
            part = true;
            break;
         }
      }
      if (!part)
      {
         if (pos == 9)
         {
            write_client(joiner.sock,"Sorry, it appears that this group is already full.");
         }
         else 
         {
         groups[pos].membres[groups[pos].nbMembres] = joiner;
         groups[pos].nbMembres = (groups[pos].nbMembres+1);
         }
      }
   }
}

static void add_member_to_private_group(Group * groups, int nbGroups, Client joiner, const char* buffer)
{
   char name3[MAX_NAME]; 
   char password[BUF_SIZE];
   int posNom = position(buffer+19,' ',0);
   strcpy(name3,"");
   strncat(name3,buffer+19,posNom);
   strcpy(password,"");
   strncat(password,buffer+20+posNom,BUF_SIZE-1);

   int pos;
   pos = get_group_from_name(groups,nbGroups,name3);

   if (strcmp(password,groups[pos].password)!=0)
   {
      write_client(joiner.sock, "Wrong password.");
   }

   else
   {
      int i;
      bool part = false;
      for(i = 0; i < groups[pos].nbMembres; i++)
      {
         if (joiner.sock == groups[pos].membres[i].sock)
         {
            write_client(joiner.sock,"You already are part of the group.");
            part = true;
            break;
         }
      }
      if (!part)
      {
         if (pos == 9)
         {
            write_client(joiner.sock,"Sorry, it appears that this group is already full.");
         }
         else 
         {
         groups[pos].membres[groups[pos].nbMembres] = joiner;
         groups[pos].nbMembres = (groups[pos].nbMembres+1);
         }
      }
   }
}

// Fonction pour échanger deux nombres
void swap(char *x, char *y) {
   char t = *x; *x = *y; *y = t;
}

// Fonction pour inverser `buffer[i…j]`
char* reverse(char *buffer, int i, int j)
{
   while (i < j) {
      swap(&buffer[i++], &buffer[j--]);
   }

   return buffer;
}

// Fonction itérative pour implémenter la fonction `itoa()` en C
char* itoa(int value, char* buffer, int base)
{
// entrée invalide
   if (base < 2 || base > 32) {
      return buffer;
   }

// considère la valeur absolue du nombre
   int n = abs(value);

   int i = 0;
   while (n)
   {
      int r = n % base;

      if (r >= 10) {
         buffer[i++] = 65 + (r - 10);
      }
      else {
         buffer[i++] = 48 + r;
      }

   n = n / base;
   }

// si le nombre est 0
   if (i == 0) {
      buffer[i++] = '0';
   }

// Si la base est 10 et la valeur est négative, la string résultante
// est précédé d'un signe moins (-)
// Avec toute autre base, la valeur est toujours considérée comme non signée
   if (value < 0 && base == 10) {
      buffer[i++] = '-';
   }

   buffer[i] = '\0'; // string de fin nulle

// inverse la string et la renvoie
   return reverse(buffer, 0, i - 1);
}

static void see_connected(Client * clients, int actual, Group * groups, int nbGroups, Client client)
{
   write_client(client.sock,"Online members and groups : \r\n \r\n");
   write_client(client.sock, "Groups : \r\n");
   char message[BUF_SIZE];
   char buff[BUF_SIZE];

   int i;
   for (i = 0; i<nbGroups;i++)
   {
      strncpy(message,groups[i].name,BUF_SIZE);
      if (groups[i].private)
      {
         strcat(message, " -");
      }
      else
      {
         strcat(message, " +");
      }
      strcat(message, " [");
      strcat(message, itoa(groups[i].nbMembres,buff,10));
      strcat(message, "/10] \r\n");
      write_client(client.sock, message);
   }

   write_client(client.sock, "\r\nMembers : \r\n");

   for (i = 0; i<actual;i++)
   {
      strncpy(message,clients[i].name,BUF_SIZE);
      strcat(message,"\r\n");
      write_client(client.sock, message);
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
