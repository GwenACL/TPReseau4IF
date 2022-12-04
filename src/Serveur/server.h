#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <time.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_GROUPS  20
#define MAX_NAME    35 

#define BUF_SIZE    1024
#define HISTORIES_DIR "histories"
#define HISTORY_FILENAME "history.txt"

#include "client.h"
#include "group.h"

/*the server root directory path*/
static char* root;

static void init(const char* argv_0);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void push_history(const char * client_name, const char *message);
static void send_history(Client client);
static void send_message_to_one_client(Client *clients, Client client, int actual, const char *buffer);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static char* concat(const char * part1, const char* part2);
static void see_connected(Client * clients, int actual, Group * groups, int nbGroups, Client client);

static int position(const char *chaine, char carac, int last);
/*get the current date formatted as dd/MM/yyyy hh:mm:ss*/
static char* get_date_heure();
static void create_public_group(Group * groups, int nbGroups, Client creator, const char* buffer);
static void add_member_to_public_group(Group * groups, int nbGroups, Client joiner, const char* buffer);
static void send_message_to_a_group(Client *clients, Client sender, Group *groups, int nbGroups, const char *buffer);
static int create_private_group(Group * groups, int nbGroups, Client creator, const char* buffer);
static void add_member_to_private_group(Group * groups, int nbGroups, Client joiner, const char* buffer);

//finds the client index in an array of clients from its name
static int get_client_from_name(Client *clients, int actual, const char* client_name);
//finds the group index in an array of groups from its name
static int get_group_from_name(Group *groups, int nbGroups, const char* group_name);

void swap(char *x, char *y);
char* reverse(char *buffer, int i, int j);
char* itoa(int value, char* buffer, int base);
//save in the sender history a message with the sending datetime
static void save_sender_message(const char *sender_name, const char *message);

#endif /* guard */
