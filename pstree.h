#ifndef _PST_H_
#define _PST_H_

/*
    Do not change this header file. It is the interface of the library to the applications.
 */

#define PST_SUCCESS 0  // function execution success
#define PST_ERROR  -1  // there is an error in the function execution.

int pst_create(char *treename, int maxdatasize, int maxmem);
int pst_destroy(char *treename);
int pst_open(char *treename);
int pst_close(int td);
int pst_get_maxdatasize(int td);
int pst_get_nodecount (int td);
int pst_insert(int td, long key, char *buf, int size);
int pst_update(int td, long key, char *buf, int size);
int pst_delete(int td, long key);
int pst_get(int td, long key, char *buf);
int pst_findkeys(int td, long key1, long key2, int count, long *keys);
int pst_printerror();

#endif
