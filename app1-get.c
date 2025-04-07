#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pstree.h"

#define TREENAME "/tree1"

int
main(int argc, char **argv)
{
    int td; // tree desciptor - to refer to the shared tree
    int maxdatasize;
    char *buffer; // points to buffer that will store data
    long key;
    int i;
    int n;
    int nodecount;

    
    td = pst_open (TREENAME);
    maxdatasize = pst_get_maxdatasize(td);
    buffer = (char *) malloc(maxdatasize);

    nodecount = pst_get_nodecount(td);
    printf ("there are %d nodes in the tree\n", nodecount);
    
    for (i = 0; i < 10; ++i) {
        key = i + 1;
        n = pst_get (td, key, buffer);
        
        if (n >= 0) {
            // received data; data can be of zero length
            printf ("data received; datasize=%d\n", n);
        }
        else {
            printf ("could not find key or there is an error\n");
        }
    }
    
    pst_close(td);
    
    // We are done.  There is also nobody using the tree.
    pst_destroy(TREENAME);
    
	return 0;
}
