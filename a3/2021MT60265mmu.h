#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZE 4*1024 // 4KB
#define MAGIC 314159265

// Free Space will be managed by free lists with first-fit strategy
// Header of occupied block will be of size 8 bytes (Size of space allocated + Magic Number)

typedef struct{
    size_t size;
    int magic;
} header;

typedef struct{
    size_t size;
    struct node* next;
} node;

void* memStartPtr = NULL;
void* freeListHead = NULL;
int initialized = 0;

void merge(node* prev, node* curr){
    if(curr == NULL) return;
    if((void*)prev + prev->size + sizeof(node) == (void*)curr){
        prev->size += curr->size + sizeof(node);
        prev->next = curr->next;
    }
}

void initializeMemory(size_t size){
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    memStartPtr = ptr;
    initialized = 1;

    freeListHead = ptr;
    freeListHead->size = MAX_SIZE;
    freeListHead->next = NULL;
}


void* my_malloc(size_t size) {
    if(!initialized)
        initializeMemory(MAX_SIZE);

    if(size > MAX_SIZE || size <= 0){
        printf("Invalid size\n");
        return NULL;
    }

    node* temp = freeListHead;
    node* prev = NULL;

    while(temp != NULL){
        if(temp->size >= size + sizeof(header)){
            node* new_node = (node*)((void*)temp + size + sizeof(header));
            new_node->size = temp->size - size - sizeof(header);
            new_node->next = temp->next;

            header* new_header = (header*)temp;
            new_header->size = size;
            new_header->magic = MAGIC;

            if(prev == NULL){
                freeListHead = new_node;
            }
            else{
                prev->next = new_node;
            }
            return (void*)temp + sizeof(header);
        }
        prev = temp;
        temp = temp->next;
    }

    printf("No free space available\n");
    return NULL;
}

void* my_calloc(size_t nelem, size_t size) {
    if(!initialized)
        initializeMemory(MAX_SIZE);
    
    void* ptr = my_malloc(nelem*size);

    if(ptr!=NULL){
        memset(ptr, 0, nelem*size);
    }
    return ptr;
}

void my_free(void* ptr) {
    if(ptr == NULL){
        printf("Invalid pointer\n");
        return;
    }
    if(!initialized){
        printf("Memory not initialized\n");
        return;
    }

    header* hptr = (header*)(ptr - sizeof(header));
    if(hptr->magic != MAGIC){
        printf("Invalid pointer\n");
        return;
    }

    hptr->magic = 0;
    int sz = hptr->size;
    // Free List can not be empty
    // My implementation of free list is ordered (Easier Coalescing)
    // Pointer should be between two elements of free list (prev and curr), or first or last element of free list

    node* curr = freeListHead;
    node* prev = NULL;
    
    while(curr!=NULL){
        if((void*)curr > (void*)hptr){
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    if(prev == NULL){
        freeListHead = (node*)hptr;
        freeListHead->size = sz;
        freeListHead->next = curr;
        merge(freeListHead,curr);
    }
    else{
        node* new_node = (node*)hptr;
        new_node->size = sz;
        new_node->next = curr;
        prev->next = new_node;
        merge(new_node,curr);
        merge(prev,new_node);
    }
}