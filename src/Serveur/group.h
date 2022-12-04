#ifndef GROUP_H
#define GROUP_H

#include "server.h"
#include "client.h"
#include <stdbool.h>

typedef struct
{
   int nbMembres;
   bool private;
   char name[BUF_SIZE];
   char password[BUF_SIZE];
   Client membres[10];
}Group;

#endif /* guard */
