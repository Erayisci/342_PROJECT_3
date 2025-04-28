#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include "pstree.h"
#include <pthread.h>   

#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>

#define PST_ERR_NONE          0   
#define PST_ERR_PST_OPEN      1   
#define PST_ERR_PST_CLOSE       2   
#define PST_ERR_PST_CREATE         3   
#define PST_ERR_PST_DESTROY    4   
#define PST_ERR_PST_GETMAXDATASIZE  5   
#define PST_ERR_GETNODECOUNT      6   
#define PST_ERR_PST_UPDATE         7
#define PST_ERR_PST_DELETE          8
#define PST_ERR_PST_GET             9
#define PST_ERR_FINDKEYS            10
#define PST_ERR_INSERT            11
#define PST_ERR_UNKNOWN       12  



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
    size_t header_size; 
    size_t sh_mem_size;
    size_t free_list_head_offset;
    size_t tree_head_offset;
    size_t maxdatasize;

    // Mutexes and condition variables stored in shared memory
    pthread_mutex_t allocation_lock;    // Mutex for synchronization of memory allocation
    pthread_cond_t enough_space;   // Condition variable to wait for space availability
    pthread_mutex_t read_write_lock;  
    pthread_cond_t read_condition;     
    pthread_mutex_t active_reader_mutex; 
    int active_readers; 
} Header;


int pst_errorcode = PST_ERR_NONE;  
void *sh_memory_base; 
static int tree_descriptor = 9;
Header* header = NULL; 
char* tree_name;



static inline void *off2ptr(size_t off) {
    return (char*)sh_memory_base + off;  
}

static inline size_t ptr2off(void *p) {
    return (char*)p - (char*)sh_memory_base;  
}

void reader_lock() {
    pthread_mutex_lock(&header->active_reader_mutex);
    header->active_readers++; 
    if (header->active_readers == 1) {
        pthread_mutex_lock(&header->read_write_lock); 
    }
    pthread_mutex_unlock(&header->active_reader_mutex);
}

void reader_unlock() {
    pthread_mutex_lock(&header->active_reader_mutex);
    header->active_readers--;  
    if (header->active_readers == 0) {
        pthread_mutex_unlock(&header->read_write_lock); 
    }
    pthread_mutex_unlock(&header->active_reader_mutex);
}

void writer_lock() {
    pthread_mutex_lock(&header->read_write_lock); 
}

void writer_unlock() {
    pthread_mutex_unlock(&header->read_write_lock);
}

size_t alloc_node(Header *hdr) {
    pthread_mutex_lock(&hdr->allocation_lock);  

    size_t cur_off = hdr->free_list_head_offset;  

    while(cur_off == 0) {
        pthread_cond_wait(&hdr->enough_space, &hdr->allocation_lock);
        cur_off = hdr->free_list_head_offset;
    }

    FreeBlock *fb = off2ptr(cur_off);
    size_t next_offset = fb->next_offset;
    hdr->free_list_head_offset = next_offset; // advanced the free list head

    Node* newNode = off2ptr(cur_off);
    newNode->left_child_offset = 0;
    newNode->right_child_offset = 0;
    newNode->actual_data_size = 0; 
    newNode->data_offset = cur_off + sizeof(Node);
    
    pthread_mutex_unlock(&hdr->allocation_lock);  // Unlock the mutex

    return cur_off; 
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
            return;
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
    ftruncate(shm_fd, memsize); 
    sh_memory_base = mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sh_memory_base == MAP_FAILED) { 
        printf("Map failed\n"); 
        pst_errorcode = PST_ERR_PST_CREATE;
        pst_printerror();
        return -1; 
    }

    header = (Header*) sh_memory_base;
    header->maxdatasize = maxdatasize;
    header->sh_mem_size = memsize;
    header->header_size = sizeof(Header);
    header->free_list_head_offset = header->header_size;
    header->tree_head_offset = 0; // Initial tree is empty

    pthread_mutexattr_t mtxattr;
    pthread_condattr_t  cndattr;
    pthread_mutexattr_init(&mtxattr);
    pthread_condattr_init(&cndattr);
    pthread_mutexattr_setpshared(&mtxattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cndattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&header->allocation_lock, &mtxattr);
    pthread_cond_init(&header->enough_space, &cndattr);
    pthread_mutex_init(&header->read_write_lock, &mtxattr);
    pthread_cond_init(&header->read_condition, &cndattr);
    pthread_mutex_init(&header->active_reader_mutex, &mtxattr);

    header->active_readers = 0; // NO READERS NOW

    size_t NODE_ALLOCATION_SIZE = sizeof(Node) + maxdatasize;
    int freenode_alloc_number = (memsize - header->header_size) / NODE_ALLOCATION_SIZE;

    FreeBlock* currentFree;
    size_t currentoff = header->free_list_head_offset;
    for (int i = 0; i < freenode_alloc_number; i++) {
        currentFree = off2ptr(currentoff); 
        currentFree->size = NODE_ALLOCATION_SIZE;
        if (i + 1 < freenode_alloc_number) {
            currentFree->next_offset = currentoff + NODE_ALLOCATION_SIZE;
        } 
        else {
            currentFree->next_offset = 0; // Last offset is 0
        }
        currentoff += NODE_ALLOCATION_SIZE;
    }

    return PST_SUCCESS;
}
                 
int pst_destroy(char *treename) {
    int td = pst_open(treename);
    if (td < 0) return pst_printerror();

    while (header->tree_head_offset != 0) {
        Node *n = off2ptr(header->tree_head_offset);
        if (pst_delete(td, n->key) != PST_SUCCESS) {
            pst_close(td);
            return pst_printerror();
        }
    }
    munmap(header, header->sh_mem_size);
    close(td);

    if (shm_unlink(treename) != 0){
        pst_errorcode = PST_ERR_PST_DESTROY;
        pst_printerror();
        return PST_ERROR;
    }

    return PST_SUCCESS;
}


int pst_open(char *treename)
{
    int shm_fd = shm_open(treename, O_RDWR, 0666);
    if(shm_fd < 0) {
        pst_errorcode = PST_ERR_PST_OPEN;
        pst_printerror();
        return PST_ERROR;
    }

    
    sh_memory_base = mmap(0, sizeof(Header), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sh_memory_base == MAP_FAILED) {
        pst_errorcode = PST_ERR_PST_OPEN;
        pst_printerror();
        close(shm_fd);
        return PST_ERROR;
    }

    header = (Header*)sh_memory_base;

    reader_lock();

    sh_memory_base = mmap(0, header->sh_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sh_memory_base == MAP_FAILED) {
        pst_errorcode = PST_ERR_PST_OPEN;
        pst_printerror();
        close(shm_fd);
        reader_unlock();
        return PST_ERROR;  
    }

    tree_name = treename;

    header = (Header*)sh_memory_base;
    reader_unlock();
    return (tree_descriptor);
}


int pst_close(int td)
{
    reader_lock();
    if(td != tree_descriptor) {
        pst_errorcode = PST_ERR_PST_CLOSE;
        pst_printerror();
        reader_unlock();
        return PST_ERROR;
    }
    int shm_fd = shm_open(tree_name, O_RDWR, 0666);
    reader_unlock();
    close(shm_fd);
    return (PST_SUCCESS);
}

int pst_get_maxdatasize(int td)
{
    reader_lock();
    if(td != tree_descriptor){
        pst_errorcode = PST_ERR_PST_GETMAXDATASIZE;
        pst_printerror();
        reader_unlock();
        return PST_ERROR;
    }
    reader_unlock();
    return header->maxdatasize;
}

int pst_get_nodecount (int td)
{
    int nodecount = 0;
    if(td != tree_descriptor){
        pst_errorcode = PST_ERR_GETNODECOUNT;
        pst_printerror();
        return PST_ERROR;
    }

    reader_lock();
        
    size_t curr = header->tree_head_offset;
    nodecount = recur_node_count(curr);

    reader_unlock();
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
    if(td != tree_descriptor){
        pst_errorcode = PST_ERR_INSERT;
        pst_printerror();
        return PST_ERROR;
    }

    writer_lock();

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
            curr = currnode->right_child_offset;
        }
        else if(key == currnode->key){
            pst_errorcode = PST_ERR_INSERT;
            pst_printerror();
            writer_unlock();
            return PST_ERROR;
        }
        else{
            curr = currnode->left_child_offset;
        }
    }
    //we found where to insert 
    size_t new_offset;
    if(first){
        header->tree_head_offset = alloc_node(header);
        new_offset = header->tree_head_offset;
    }
    else{
        new_offset = alloc_node(header);

        Node* parent_node = (Node*)off2ptr(parent);
        if(key > parent_node->key){
            parent_node->right_child_offset = new_offset;
        }
        else{
            parent_node->left_child_offset = new_offset;
        }
    }
    
    Node* insertedNode = (Node*)off2ptr(new_offset);
    insertedNode->actual_data_size = size;
    insertedNode->key = key;
   
    insertedNode->left_child_offset = 0;
    insertedNode->right_child_offset = 0;

    char *shared_data = (char*)off2ptr(new_offset + sizeof(Node));
    memcpy(shared_data, buf, size);
    
    writer_unlock();
    return (PST_SUCCESS);
}


int pst_update(int td, long key, char *buf, int size) {
    writer_lock();  // Ensure exclusive write access during the update

    if (td != tree_descriptor) {
        pst_errorcode = PST_ERR_PST_UPDATE;
        pst_printerror();
        writer_unlock(); // Release lock before returning
        return PST_ERROR;
    }

    size_t curr = header->tree_head_offset;
    Node* currnode;
    while (curr != 0) {
        currnode = off2ptr(curr);
        if (currnode->key == key) {
            // Key found, update data
            char* shared_data = (char*)off2ptr(currnode->data_offset);
            memcpy(shared_data, buf, size);  
            currnode->actual_data_size = size; 
            writer_unlock();  
            return PST_SUCCESS;  
        }
        if (key < currnode->key) {
            curr = currnode->left_child_offset;  // Go to left child
        } else {
            curr = currnode->right_child_offset;  // Go to right child
        }
    }

    // Key not found
    pst_errorcode = PST_ERR_PST_UPDATE;
    pst_printerror();
    writer_unlock();  // Release the lock
    return PST_ERROR;  
}


int pst_delete(int td, long key)
{
    if (td != tree_descriptor){
        pst_errorcode = PST_ERR_PST_DELETE;
        pst_printerror();
        return PST_ERROR;
    }

    writer_lock(); 

    size_t curr_off   = header->tree_head_offset;
    size_t parent_off = 0;
    Node  *curr       = NULL;
    Node  *parent     = NULL;

    while (curr_off != 0) {
        curr = (Node*)off2ptr(curr_off);
        if (key < curr->key) {
            parent_off = curr_off;
            parent     = curr;
            curr_off   = curr->left_child_offset;
        }
        else if (key > curr->key) {
            parent_off = curr_off;
            parent     = curr;
            curr_off   = curr->right_child_offset;
        }
        else {
            break;  
        }
    }
    if (curr_off == 0){// key not found
        pst_errorcode = PST_ERR_PST_DELETE;
        pst_printerror();
        writer_unlock();
        return PST_ERROR;
    }

    if (curr->left_child_offset && curr->right_child_offset) {
        size_t succ_off        = curr->right_child_offset;
        size_t succ_parent_off = curr_off;
        Node  *succ_parent     = curr;
        Node  *succ            = (Node*)off2ptr(succ_off);

        while (succ->left_child_offset != 0) {
            succ_parent_off = succ_off;
            succ_parent     = succ;
            succ_off        = succ->left_child_offset;
            succ            = (Node*)off2ptr(succ_off);
        }

        long tmp_key    = curr->key;
        curr->key       = succ->key;
        succ->key       = tmp_key;

        size_t tmp_sz               = curr->actual_data_size;
        curr->actual_data_size      = succ->actual_data_size;
        succ->actual_data_size      = tmp_sz;

        size_t tmp_off              = curr->data_offset;
        curr->data_offset           = succ->data_offset;
        succ->data_offset           = tmp_off;

        parent     = succ_parent;
        parent_off = succ_parent_off;
        curr       = succ;
        curr_off   = succ_off;
    }

    size_t child_off = curr->left_child_offset
                     ? curr->left_child_offset
                     : curr->right_child_offset;

    if (parent == NULL) {
        header->tree_head_offset = child_off;
    }
    else if (parent->left_child_offset == curr_off) {
        parent->left_child_offset = child_off;
    }
    else {
        parent->right_child_offset = child_off;
    }

    free_node(header, curr->key, curr_off);
    writer_unlock();
    return PST_SUCCESS;
}


int pst_get(int td, long key, char *buf)
{
    if (td != tree_descriptor){
        pst_errorcode = PST_ERR_PST_GET;
        pst_printerror();
        return PST_ERROR;
    }
    
    reader_lock();
    size_t curr = header->tree_head_offset;
    Node* currnode;
    while(curr != 0){
        currnode = off2ptr(curr);
        if(currnode->key == key){
            char* our_data = (char *)off2ptr(currnode->data_offset);
            memcpy(buf, our_data, currnode->actual_data_size);
            reader_unlock();
            return currnode->actual_data_size;
        }
        if(key < currnode->key){
            curr = currnode->left_child_offset;
        }
        else{
            curr = currnode->right_child_offset;
        }
    }
    //couldnt find so error
    pst_errorcode = PST_ERR_PST_GET;
    pst_printerror();
    reader_unlock();
    return (PST_ERROR);
}

int pst_findkeys(int td, long key1, long key2, int N, long keys[])
{
    if (td != tree_descriptor){
        pst_errorcode = PST_ERR_FINDKEYS;
        pst_printerror();
        return PST_ERROR;
    }

    reader_lock();
    
    int count = 0;
    helper_findkeys(header->tree_head_offset,key1, key2, &count, N, keys);

    reader_unlock();
    return count;
}

// recursive helper
void helper_findkeys(size_t node_offset, long key1, long key2, int *count, int N, long keys[]) {
    
    Node* node = off2ptr(node_offset);
    if (node_offset == 0 || *count == N) 
        return;
    if (node->key > key1)
        helper_findkeys(node->left_child_offset, key1, key2, count, N, keys);

    if (*count < N && node->key >= key1 && node->key <= key2)
        keys[(*count)++] = node->key;

    if (*count < N && node->key < key2)
        helper_findkeys(node->right_child_offset, key1, key2, count, N, keys);
}

int pst_printerror()
{
    const char *msg;
    switch (pst_errorcode) {
      case PST_ERR_NONE:
        msg = "pstree: no error\n"; break;
      case PST_ERR_PST_OPEN:
        msg = "pstree: error in pst_open\n"; break;
      case PST_ERR_PST_CLOSE:
        msg = "pstree: error in pst_close\n"; break;
      case PST_ERR_PST_CREATE:
        msg = "pstree: error in pst_create\n"; break;
      case PST_ERR_PST_DESTROY:
        msg = "pstree: error in pst_destroy\n"; break;
      case PST_ERR_PST_GETMAXDATASIZE:
        msg = "pstree: error in pst_getmaxdatasize\n"; break;
      case PST_ERR_GETNODECOUNT:
        msg = "pstree: error in pst_getnodecount\n"; break;
      case PST_ERR_PST_UPDATE:
      msg = "pstree: error in pst_update\n"; break;
      case PST_ERR_PST_DELETE:
      msg = "pstree: error in pst_delete\n"; break;
      case PST_ERR_PST_GET:
      msg = "pstree: error in pst_get\n"; break;
      case PST_ERR_FINDKEYS:
      msg = "pstree: error in pst_findkeys\n"; break;
      default:
        msg = "pstree: unknown error\n"; break;
    }
    fputs(msg, stderr);
    return PST_ERROR;  /* so callers can simply do: return pst_printerror(); */
}

/*
    we included traversal functions just to debug the code
*/
void helper_traversal(size_t head){
    if(head == 0){
        return;
    }

    size_t curr = head;
    Node* currnode = off2ptr(curr);
    size_t left = currnode->left_child_offset;
    size_t right = currnode->right_child_offset;

    helper_traversal(left);
    char* data = off2ptr(currnode->data_offset);
    printf("%s\n",data);
    helper_traversal(right);
}

void inorder_traversal(){
    size_t curr = header->tree_head_offset;
    helper_traversal(curr);
}
