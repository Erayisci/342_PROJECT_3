#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <time.h>
#include "pstree.h"

#define NUM_OPERATIONS 10000  

void generate_data(long key, char *data) {
    sprintf(data, "data%ld", key);  
}


void measure_delete_time(int td) {
    clock_t start_time, end_time;
    double elapsed_time;

    long key;
    char data[1024];

    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        key = i + 1;
        generate_data(key, data);  
        if (pst_insert(td, key, data, strlen(data) + 1) == PST_ERROR) {
            printf("Insert failed for key %ld!\n", key);
        }
    }

    start_time = clock(); 

    key = NUM_OPERATIONS;  
    if (pst_delete(td, key) == PST_ERROR) {
        printf("Delete failed for key %ld!\n", key);
    }

    end_time = clock(); 

    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Last Delete operation for key %ld took: %f seconds\n", key, elapsed_time);
}

int main() {
    int td;

    pst_create("/tree1", 1000000000, 1024);
    td = pst_open("/tree1");

    printf("Measuring Last Delete Time\n");
    measure_delete_time(td);

    pst_close(td);
    pst_destroy("/tree1");

    return 0;
}
