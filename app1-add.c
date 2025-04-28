#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pstree.h"


#define TREENAME "/tree1"
#define MEMSIZE 64000 // bytes
#define MAXDATASIZE 1024 // bytes

int
main(int argc, char **argv)
{
    int td;
    int i;
    long key;
    char data[MAXDATASIZE];
        
    
    pst_create(TREENAME, MEMSIZE, MAXDATASIZE);
    td = pst_open(TREENAME);

    for (i = 0; i < 10; ++i) {
        key = i + 1;
        sprintf (data, "data%d", i + 1);
        pst_insert (td, key, data, strlen(data)+1);
        printf ("added data\n");
    }
    
    pst_close(td);
    
	return 0;
}