/*
 * memory.c — Virtual RAM Pool Allocator
 *
 * Strategy: First-fit free-list allocator.
 *
 * Layout inside VRAM[]:
 *   [BlockHeader][user data][BlockHeader][user data]...
 *
 * BlockHeader contains: size (bytes of user data) and used flag.
 * Freed blocks are merged with adjacent free blocks (coalescing).
 *
 * The global VRAM array is the ONLY place where <stdlib.h>'s memory
 * is used — it is declared as a static global, fulfilling the
 * "allocate one giant block of virtual RAM" requirement.
 */
#include "memory.h"
#include "math.h"
#include <stdio.h>   /* allowed: printf for debug dump only */

/* ----- Internal block header ----- */
typedef struct BlockHeader {
    int  size;              /* bytes of user payload (not including header) */
    int  used;              /* 1 = allocated, 0 = free                       */
    struct BlockHeader *next; /* pointer to next block header                */
} BlockHeader;

#define HEADER_SIZE ((int)sizeof(BlockHeader))

/* The one and only virtual RAM region */
static char VRAM[POOL_SIZE];

/* Pointer to the first block in the free list */
static BlockHeader *pool_head = (void*)0;

/* ---------------------------------------------------------------
 * mem_init: Must be called once before any alloc/dealloc.
 * Sets up the entire VRAM as one large free block.
 * --------------------------------------------------------------- */
void mem_init(void) {
    pool_head = (BlockHeader *)VRAM;
    pool_head->size = POOL_SIZE - HEADER_SIZE;
    pool_head->used = 0;
    pool_head->next = (void*)0;
}

/* ---------------------------------------------------------------
 * alloc: First-fit allocator.
 * Finds the first free block >= size. Splits the block if it is
 * large enough to leave a useful remainder (> HEADER_SIZE + 8).
 * Returns NULL if no block fits.
 * --------------------------------------------------------------- */
void *alloc(int size) {
    if (size <= 0) return (void*)0;

    /* Align to 8 bytes */
    int rem = my_mod(size, 8);
    if (rem != 0) size += (8 - rem);

    BlockHeader *cur = pool_head;
    while (cur != (void*)0) {
        if (!cur->used && cur->size >= size) {
            /* Can we split? Need space for a new header + at least 8 bytes. */
            int leftover = cur->size - size;
            if (leftover > HEADER_SIZE + 8) {
                /* Create a new free block after this one */
                BlockHeader *newblk = (BlockHeader *)((char *)cur + HEADER_SIZE + size);
                newblk->size = leftover - HEADER_SIZE;
                newblk->used = 0;
                newblk->next = cur->next;
                cur->size = size;
                cur->next = newblk;
            }
            cur->used = 1;
            return (void *)((char *)cur + HEADER_SIZE);
        }
        cur = cur->next;
    }
    return (void*)0; /* out of memory */
}

/* ---------------------------------------------------------------
 * dealloc: Mark a block as free and coalesce with the next block
 * if it is also free. (Forward coalescing only for simplicity.)
 * --------------------------------------------------------------- */
void dealloc(void *ptr) {
    if (ptr == (void*)0) return;

    BlockHeader *blk = (BlockHeader *)((char *)ptr - HEADER_SIZE);
    blk->used = 0;

    /* Forward coalescing: merge with next block if it is also free */
    while (blk->next != (void*)0 && !blk->next->used) {
        BlockHeader *nxt = blk->next;
        blk->size += HEADER_SIZE + nxt->size;
        blk->next  = nxt->next;
    }
}

/* ---------------------------------------------------------------
 * mem_used: Returns total bytes currently allocated (user payload).
 * --------------------------------------------------------------- */
int mem_used(void) {
    int total = 0;
    BlockHeader *cur = pool_head;
    while (cur != (void*)0) {
        if (cur->used) total += cur->size;
        cur = cur->next;
    }
    return total;
}

/* ---------------------------------------------------------------
 * mem_dump_stats: Print a one-line summary (uses <stdio.h> printf).
 * --------------------------------------------------------------- */
void mem_dump_stats(void) {
    int used = 0, free_bytes = 0, blocks = 0;
    BlockHeader *cur = pool_head;
    while (cur != (void*)0) {
        blocks++;
        if (cur->used) used += cur->size;
        else           free_bytes += cur->size;
        cur = cur->next;
    }
    printf("[MEM] blocks=%d  used=%d  free=%d\n", blocks, used, free_bytes);
}
