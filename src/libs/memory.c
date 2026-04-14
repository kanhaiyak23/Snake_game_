/*
 * memory.c — Virtual RAM Pool Allocator
 *
 * Strategy: Best-fit free-list allocator.
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
 * is_adjacent: true when block b starts immediately after a.
 * --------------------------------------------------------------- */
static int is_adjacent(const BlockHeader *a, const BlockHeader *b) {
    return ((const char *)a + HEADER_SIZE + a->size) == (const char *)b;
}

/* ---------------------------------------------------------------
 * coalesce_with_next: repeatedly merge blk with next free block if
 * they are physically adjacent in VRAM.
 * --------------------------------------------------------------- */
static void coalesce_with_next(BlockHeader *blk) {
    while (blk->next != (void*)0 && !blk->next->used && is_adjacent(blk, blk->next)) {
        BlockHeader *nxt = blk->next;
        blk->size += HEADER_SIZE + nxt->size;
        blk->next  = nxt->next;
    }
}

/* ---------------------------------------------------------------
 * find_prev_by_address: scan from pool_head to find the block
 * whose ->next is `blk` (i.e., the predecessor in list order,
 * which always matches address order because alloc keeps the list
 * sorted by address).
 * Returns NULL if blk is the first block.
 * --------------------------------------------------------------- */
static BlockHeader *find_prev_by_address(BlockHeader *blk) {
    BlockHeader *cur = pool_head;
    BlockHeader *prev = (void*)0;
    while (cur != (void*)0 && cur != blk) {
        prev = cur;
        cur  = cur->next;
    }
    return prev;
}

/* ---------------------------------------------------------------
 * alloc: Best-fit allocator.
 * Scans all free blocks and picks the smallest block that can fit
 * `size` (after 8-byte alignment). Splits when remainder is useful.
 * Returns NULL if no block fits.
 * --------------------------------------------------------------- */
void *alloc(int size) {
    if (size <= 0) return (void*)0;

    /* Align to 8 bytes */
    int rem = my_mod(size, 8);
    if (rem != 0) size += (8 - rem);

    BlockHeader *cur = pool_head;
    BlockHeader *best = (void*)0;
    while (cur != (void*)0) {
        if (!cur->used && cur->size >= size) {
            if (best == (void*)0 || cur->size < best->size) {
                best = cur;
                if (best->size == size) break; /* exact fit, can't do better */
            }
        }
        cur = cur->next;
    }

    if (best != (void*)0) {
        /* Can we split? Need space for a new header + at least 8 bytes. */
        int leftover = best->size - size;
        if (leftover > HEADER_SIZE + 8) {
            /* Create a new free block after this one */
            BlockHeader *newblk = (BlockHeader *)((char *)best + HEADER_SIZE + size);
            newblk->size = leftover - HEADER_SIZE;
            newblk->used = 0;
            newblk->next = best->next;
            best->size = size;
            best->next = newblk;
        }
        best->used = 1;
        return (void *)((char *)best + HEADER_SIZE);
    }
    return (void*)0; /* out of memory */
}

/* ---------------------------------------------------------------
 * dealloc: Mark a block as free and coalesce with both neighbors
 * when possible (previous and next).
 * --------------------------------------------------------------- */
void dealloc(void *ptr) {
    if (ptr == (void*)0) return;

    /* Safety: ensure ptr is within the pool */
    if ((char *)ptr < VRAM || (char *)ptr >= VRAM + POOL_SIZE) return;

    BlockHeader *blk = (BlockHeader *)((char *)ptr - HEADER_SIZE);

    /* Double-free guard */
    if (!blk->used) return;

    blk->used = 0;

    /* Find previous block using address-order traversal */
    BlockHeader *prev = find_prev_by_address(blk);

    /* Coalesce right side first (blk absorbs its free successor) */
    coalesce_with_next(blk);

    /* Coalesce with previous if it is free and adjacent */
    if (prev != (void*)0 && !prev->used && is_adjacent(prev, blk)) {
        prev->size += HEADER_SIZE + blk->size;
        prev->next  = blk->next;
        /* Continue coalescing newly enlarged prev with its successor */
        coalesce_with_next(prev);
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
