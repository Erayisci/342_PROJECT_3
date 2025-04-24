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
        //printf ("added data\n");
    }
    inorder_traversal();
    printf("node count is: %d", pst_get_nodecount(9));
    pst_delete(9, 3);
    printf("node count after deletion: %d", pst_get_nodecount(9));
    inorder_traversal();
    char data_to_retrieve[MAXDATASIZE];
    printf("copied size is %d \n",pst_get(9,8,data_to_retrieve));
    printf("Data is %s \n", data_to_retrieve);
    pst_close(td);

    
	return 0;
}
