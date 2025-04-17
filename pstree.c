#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include "pstree.h"

/*
    You will implement your library in this file. You can define your internal structures, macros here (such as: #define ..). You can also define and use global variables here. You can define and use additional functions (as many as you wish) in this file; besides the pst library functions desribed in the assignment. These additional functions will not be available for applications directly.
 
    You should not change the pstree.h header file. 
 */

int pst_errorcode;  // global -  store the error code in this variable


                 
int pst_create(char *treename, int memsize, int maxdatasize)
{
    return (PST_SUCCESS);
}
                 
int pst_destroy(char *treename)
{
    return (PST_SUCCESS);
}

int pst_open(char *treename)
{
    return (PST_SUCCESS);
}

int pst_close(int td)
{
    return (PST_SUCCESS);
}

int pst_get_maxdatasize(int tid)
{
    return (PST_SUCCESS);
}

int pst_get_nodecount (int tid)
{
    return (PST_SUCCESS);
}


int pst_insert(int td, long key, char *buf, int size)
{
    return (PST_SUCCESS);
}


int pst_update(int td, long key, char *buf, int size)
{
return (PST_SUCCESS);
}


int pst_delete(int td, long key)
{
    return (PST_ERROR);
}

int pst_get(int td, long key, char *buf)
{
    return (PST_ERROR);
}

int pst_findkeys(int td, long key1, long key2, int N, long *keys)
{
    return (PST_ERROR);
}

int pst_printerror()
{
    return (PST_SUCCESS);
}
