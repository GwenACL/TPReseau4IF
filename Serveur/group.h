#ifndef GROUP_H
#define GROUP_H

#include "server.h"
#include "client.h"

typedef struct
{
   int nbMembres;
   char name[BUF_SIZE];
   Client membres[10];
}Group;

#endif /* guard */
