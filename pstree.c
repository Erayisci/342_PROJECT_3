#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include "pstree.h"
#include <pthread.h>   /* pthread_create, vs. */

#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>


typedef struct
{
    long key;
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
    size_t free_list_head_offset;// head of the free linked list
    size_t tree_head_offset;
    size_t maxdatasize;
    pthread_mutex_t allocation_lock;    // Mutex for synchronization of memory allocation
    pthread_cond_t enough_space;   // Condition variable to wait for space availability
} Header;


int pst_errorcode;  // global -  store the error code in this variable
void *sh_memory_base; // it is the return value of mmap()
static int tree_descriptor = 9;
Header* header = NULL; // it will point to where sh_memor_abse will point
char* tree_name;


// Convert an offset to a pointer for the current process
static inline void *off2ptr(size_t off) {
    return (char*)sh_memory_base + off;  // Add the offset to the region base address
}

// Convert a pointer to an offset within the shared memory region
static inline size_t ptr2off(void *p) {
    return (char*)p - (char*)sh_memory_base;  // Subtract the region base to get the offset
}

size_t alloc_node(Header *hdr) {
    pthread_mutex_lock(&hdr->allocation_lock);  // Lock the allocation mutex

    size_t cur_off = hdr->free_list_head_offset;  // Start at the first free block
    printf("alloc node start \n");

    while(cur_off == 0) {
        printf("alloc node second \n");

        // No space available, wait until there is space
        pthread_cond_wait(&hdr->enough_space, &hdr->allocation_lock);
        cur_off = hdr->free_list_head_offset;
    }

    FreeBlock *fb = off2ptr(cur_off);
    size_t next_offset = fb->next_offset;
    printf("next offset is: %d\n", next_offset);
    hdr->free_list_head_offset = next_offset; // advanced the free list head

    Node* newNode = off2ptr(cur_off);
    newNode->left_child_offset = 0;
    newNode->right_child_offset = 0;
    newNode->actual_data_size = 0; // default value of 0
    newNode->data_offset = cur_off + sizeof(Node);
    // key is not set yet 
    
    pthread_mutex_unlock(&hdr->allocation_lock);  // Unlock the mutex

    return cur_off; // offset of the newly allocated field (starting point)
}

void free_node(Header *hdr, long key, size_t free_offset){
    size_t curr = hdr->tree_head_offset;
    Node* node = off2ptr(curr);
    while(node->key != key){
        if(key > node->key){
            curr = node->right_child_offset;
        }
        else{
            curr = node->left_child_offset;
        }
        if(curr == 0){
            return PST_ERROR;
        }
        node = off2ptr(curr);
    }
    // WE FOUND THE NODE TO BE DELETED WITH KEY = KEY
    size_t old = hdr->free_list_head_offset;
    hdr->free_list_head_offset = curr;
    FreeBlock* newFree = off2ptr(curr);
    newFree->next_offset = old;
    newFree->size = sizeof(Node) + hdr->maxdatasize;
    //NOTE THAT WE DID NOT CHANGE NODE'S NEXT CHILD IS GONNA BE HERE
}

                 
int pst_create(char *treename, int memsize, int maxdatasize)
{
    shm_unlink(treename);
    tree_name = treename;

    int shm_fd = shm_open(treename, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd,memsize); // set size of shared memory
    sh_memory_base = mmap(0,memsize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sh_memory_base == MAP_FAILED) { printf("Map failed\n"); return -1; }

    header = (Header*) sh_memory_base;
    header->maxdatasize = maxdatasize;
    header->sh_mem_size = memsize;
    header->header_size = sizeof(Header);
    header->free_list_head_offset = header->header_size;
    header->tree_head_offset = 0; // it is the same offset with free_list_head initailly

    
    pthread_mutexattr_t mtxattr;
    pthread_condattr_t  cndattr;

    pthread_mutexattr_init(&mtxattr);
    pthread_condattr_init(&cndattr);

    pthread_mutexattr_setpshared(&mtxattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cndattr,  PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&header->allocation_lock,  &mtxattr);
    pthread_cond_init( &header->enough_space, &cndattr);

    size_t NODE_ALLOCATION_SIZE = sizeof(Node) + maxdatasize;
    int freenode_alloc_number = (memsize - header->header_size)/NODE_ALLOCATION_SIZE;

    FreeBlock* currentFree;
    size_t currentoff = header->free_list_head_offset;
    for (int i = 0; i < freenode_alloc_number; i++)
    {
        currentFree = off2ptr(currentoff); 
        currentFree->size = NODE_ALLOCATION_SIZE;
        if (i + 1 < freenode_alloc_number) {
            currentFree->next_offset = currentoff + NODE_ALLOCATION_SIZE;
        } 
        else {
            currentFree->next_offset = 0; // last offset is 0
        }
        currentoff += NODE_ALLOCATION_SIZE;
    }

    return (PST_SUCCESS);
}
                 
int pst_destroy(char *treename)
{

    return (PST_SUCCESS);
}

int pst_open(char *treename)
{
    int shm_fd = shm_open(treename, O_RDWR, 0666);
    if(shm_fd < 0) {
        perror("shm_open");
        return PST_ERROR;
    }

    sh_memory_base = mmap(0, header->sh_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sh_memory_base == MAP_FAILED) {
        perror("mmap");
        pst_close(shm_fd);
        return PST_ERROR;  // Failed to map shared memory
    }
    header = (Header*)sh_memory_base;  // Set up the header pointer
    return (tree_descriptor);
}

int pst_close(int td)
{
    if(td != tree_descriptor) 
        return PST_ERROR;
    int shm_fd = shm_open(tree_name, O_RDWR, 0666);
    close(shm_fd);
    return (PST_SUCCESS);
}

int pst_get_maxdatasize(int td)
{
    if(td != tree_descriptor)
        return PST_ERROR;
    return header->maxdatasize;
}

int pst_get_nodecount (int td)
{
    int nodecount = 0;
    if(td != tree_descriptor)
        return PST_ERROR;

    size_t curr = header->tree_head_offset;
    nodecount = recur_node_count(curr);
    return (nodecount);
}

int recur_node_count(size_t head_offset){
    if(head_offset == 0)
        return 0;
    
    Node* node = (Node*)off2ptr(head_offset);
    int left_child = 0; 
    int right_child = 0;
    if(node->left_child_offset != 0)
        left_child =  recur_node_count(node->left_child_offset);

    if(node->right_child_offset != 0)
        right_child = recur_node_count(node->right_child_offset);
    return 1 + left_child + right_child;
}


int pst_insert(int td, long key, char *buf, int size)
{
    if(td != tree_descriptor)
        return PST_ERROR;

    size_t curr = header->tree_head_offset;
    size_t parent = curr;
    Node* currnode;
    int first = 0;
    if(header->tree_head_offset == 0){// first insertion
        first = 1;
    }
    while(!first && curr != 0){
        currnode = (Node*)off2ptr(curr);
        parent = curr;
        if(key > currnode->key){
            printf("right child \n");
            printf("key : %d ", currnode->key);
            curr = currnode->right_child_offset;
        }
        else if(key == currnode->key){
            return PST_ERROR;
        }
        else{
            printf("left child \n");
            printf("key : %d ", currnode->key);
            curr = currnode->left_child_offset;
        }
    }
    printf("\n");
    //we found where to insert 
    size_t new_offset;
    if(first){
        printf("first insertion \n");
        header->tree_head_offset = alloc_node(header);
        new_offset = header->tree_head_offset;
    }
    else{
        printf("second insertion \n");
        new_offset = alloc_node(header);
        printf("alloc node sonrasi \n");

        Node* parent_node = (Node*)off2ptr(parent);
        if(key > parent_node->key){
            printf("(right child)inserted key: %d", key);
            parent_node->right_child_offset = new_offset;
        }
        else{
            printf("(left child)inserted key: %d", key);
            parent_node->left_child_offset = new_offset;
        }
    }
    Node* insertedNode = (Node*)off2ptr(new_offset);
    insertedNode->actual_data_size = size;
    insertedNode->key = key;
    insertedNode->data_offset = ptr2off(buf);
    insertedNode->left_child_offset = 0;
    insertedNode->right_child_offset = 0;
    
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
