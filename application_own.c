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

    int key_arr[] = { 3, 6, 8, 10, 17, 20, 25 };
    for (i = 0; i < 7; ++i) {
        key = i + 1;
        sprintf(data, "data%d", i + 1);
        pst_insert(td, key_arr[i], data, strlen(data) + 1);
        printf("Added data\n");
    }

    printf("Node count is: %d\n", pst_get_nodecount(td));

    pst_delete(td, 3);
    printf("Node count after deletion: %d\n", pst_get_nodecount(td));

    char data_to_retrieve[MAXDATASIZE];
    printf("Copied size is %d \n", pst_get(td, 8, data_to_retrieve));
    printf("Data is: %s \n", data_to_retrieve);

    char new_data[MAXDATASIZE] = "Updated data for key 8";
    int update_status = pst_update(td, 8, new_data, strlen(new_data) + 1);
    if (update_status == PST_SUCCESS) {
        printf("Data for key 8 updated successfully!\n");
    } else {
        printf("Failed to update data for key 8\n");
    }

    char updated_data[MAXDATASIZE];
    printf("Copied size after update is %d \n", pst_get(td, 8, updated_data));
    printf("Updated Data is: %s \n", updated_data);

    int count = 0;
    int N = 10;
    int key1 = 8;
    int key2 = 20;
    long keys[5];
    int return_value = pst_findkeys(td, key1, key2, N, keys);
    printf("Number of values in keys is: %d\n", return_value);
    for (int i = 0; i < return_value; i++) {
        printf("%ld ", keys[i]);
    }
    printf("\n");

    printf("Node count BEFORE DELETION: %d\n", pst_get_nodecount(td));

    pst_destroy(TREENAME);
    pst_close(td);

    return 0;
}
