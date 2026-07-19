#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

typedef struct block_header {
    size_t size;
    int free;
    struct block_header *next;
    struct block_header *prev;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

static block_header_t *free_list_head = NULL;
static block_header_t *heap_start = NULL;

/* Request more memory from the OS using sbrk() */
static block_header_t *request_space(size_t size) {
    block_header_t *block;
    void *request = sbrk(size + HEADER_SIZE);

    if (request == (void *)-1) {
        return NULL; /* sbrk failed */
    }

    block = (block_header_t *)request;
    block->size = size;
    block->free = 0;
    block->next = NULL;
    block->prev = NULL;

    return block;
}

/* First-fit search through the free list */
static block_header_t *find_free_block(size_t size) {
    block_header_t *current = free_list_head;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Split a block if it's much bigger than needed */
static void split_block(block_header_t *block, size_t size) {
    if (block->size >= size + HEADER_SIZE + ALIGNMENT) {
        block_header_t *new_block = (block_header_t *)((char *)block + HEADER_SIZE + size);
        new_block->size = block->size - size - HEADER_SIZE;
        new_block->free = 1;
        new_block->next = block->next;
        new_block->prev = block;

        if (block->next) {
            block->next->prev = new_block;
        }

        block->next = new_block;
        block->size = size;
    }
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;

    size = ALIGN(size);
    block_header_t *block;

    if (!heap_start) {
        /* First allocation ever */
        block = request_space(size);
        if (!block) return NULL;
        heap_start = block;
        free_list_head = block;
    } else {
        block = find_free_block(size);
        if (block) {
            split_block(block, size);
            block->free = 0;
        } else {
            /* No suitable free block found, request new space */
            block = request_space(size);
            if (!block) return NULL;

            /* Append to the end of the list */
            block_header_t *current = free_list_head;
            while (current->next) {
                current = current->next;
            }
            current->next = block;
            block->prev = current;
        }
    }

    return (void *)(block + 1); /* memory right after the header */
}

/* Merge adjacent free blocks to reduce fragmentation */
static void coalesce(block_header_t *block) {
    if (block->next && block->next->free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    if (block->prev && block->prev->free) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void my_free(void *ptr) {
    if (!ptr) return;

    block_header_t *block = (block_header_t *)ptr - 1;
    block->free = 1;

    coalesce(block);
}

/* --- Simple test driver --- */

void print_free_list(void) {
    block_header_t *current = free_list_head;
    printf("---- Free list ----\n");
    while (current) {
        printf("Block @ %p | size: %zu | free: %d\n",
               (void *)current, current->size, current->free);
        current = current->next;
    }
    printf("--------------------\n");
}

int main(void) {
    printf("Allocating a (100 bytes), b (200 bytes), c (50 bytes)\n");
    char *a = my_malloc(100);
    char *b = my_malloc(200);
    char *c = my_malloc(50);

    strcpy(a, "Hello");
    strcpy(b, "World");
    strcpy(c, "!");

    printf("a: %s, b: %s, c: %s\n", a, b, c);
    print_free_list();

    printf("\nFreeing b...\n");
    my_free(b);
    print_free_list();

    printf("\nFreeing a (should coalesce with b's freed space)...\n");
    my_free(a);
    print_free_list();

    printf("\nAllocating d (150 bytes) - should reuse coalesced space\n");
    char *d = my_malloc(150);
    strcpy(d, "Reused!");
    printf("d: %s\n", d);
    print_free_list();

    my_free(c);
    my_free(d);

    return 0;
}
