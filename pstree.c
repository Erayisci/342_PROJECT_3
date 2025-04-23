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
void *sh_memory_base; // it is the return value of mmap()
static int tree_descriptor = 9;

typedef struct
{
    long int key;
    size_t actual_data_size; //stores the size of the actual data
    size_t data_offset; // start address of the actual data stores
    size_t left_child_offset;
    size_t right_child_offset;
} Node;

typedef struct
{
    size_t size;
    size_t next_offset;
} FreeBlock;

typedef struct
{
    size_t header_size; // The size of the shared header itself (for offset calculations)
    size_t sh_mem_size;
    size_t first_free_block;// head of the free linked list
    size_t maxdatasize;
    pthread_mutex_t alloc_mtx;    // Mutex for synchronization of memory allocation
    pthread_cond_t space_avail;   // Condition variable to wait for space availability
} Header;


// Convert an offset to a pointer for the current process
static inline void *off2ptr(size_t off) {
    return (char*)sh_memory_base + off;  // Add the offset to the region base address
}

// Convert a pointer to an offset within the shared memory region
static inline size_t ptr2off(void *p) {
    return (char*)p - (char*)sh_memory_base;  // Subtract the region base to get the offset
}

size_t alloc_node(Header *hdr) {
    pthread_mutex_lock(&hdr->alloc_mtx);  // Lock the allocation mutex

    size_t prev_off = 0;
    size_t cur_off = hdr->first_free_block;  // Start at the first free block

    FreeBlock *fb;
    while (cur_off != 0) {
        fb = off2ptr(cur_off);
        if (fb->size >= sizeof(Node) + hdr->maxdatasize) {
            break;  // Found a block large enough
        }
        prev_off = cur_off;
        cur_off = fb->next_offset;
    }

    if (cur_off == 0) {
        // No space available, wait until there is space
        pthread_cond_wait(&hdr->space_avail, &hdr->alloc_mtx);
    }

    size_t new_node_off;
    if (fb->size == sizeof(Node) + hdr->maxdatasize) {
        // Perfect fit, remove it from the free list
        if (prev_off) {
            FreeBlock* prev_pointer = off2ptr(prev_off);
            prev_pointer->next_offset = fb->next_offset;
        } else {
            hdr->first_free_block = fb->next_offset;
        }
        new_node_off = cur_off;
    } else {
        // Split the block, create a new node at the end of the current block
        new_node_off = cur_off + (fb->size - (sizeof(Node) + hdr->maxdatasize));
        fb->size -= (sizeof(Node) + hdr->maxdatasize);  // Shrink the free block
    }

    // Allocate space for the node's data field and store the offset in the node
    Node *node = off2ptr(new_node_off);  // Get the node at this offset
    node->data_offset = new_node_off + sizeof(Node);  // Set the offset to the data

    pthread_mutex_unlock(&hdr->alloc_mtx);  // Unlock the mutex

    return new_node_off;
}

// void free_node(Header *hdr, size_t free_offset){
//     size_t NODE_SZ = sizeof(Node) + hdr->maxdatasize;

//     size_t curr_offset = hdr->first_free_block;
//     size_t prev_offset = 0;
//     FreeBlock *fb;
//     while(curr_offset != 0){
//         fb = off2ptr(curr_offset);
//         if(curr_offset + fb->size == free_offset) break;
//         prev_offset = curr_offset;
//         curr_offset = fb->next_offset;
//     }
//     if(curr_offset != 0){
//         FreeBlock *prevFree = off2ptr(prev_offset);
//         prevFree->size += NODE_SZ;
//         if(prevFree->next_offset == prev_offset + NODE_SZ + prevFree->size){
//             prevFree->next_offset = 
//         }
//     }
// }

                 
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

int pst_findkeys(int td, long key1, long key2, int N, long keys[])
{
    return (PST_ERROR);
}

int pst_printerror()
{
    return (PST_SUCCESS);
}
