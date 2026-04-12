#ifndef MEMORY_H
#define MEMORY_H

/* memory.h — Custom Virtual RAM Pool
 * Implements a free-list allocator on top of a global byte array.
 * No malloc/free from <stdlib.h> is used for game data.
 */

#define POOL_SIZE (1024 * 1024)   /* 1 MB virtual RAM */

void  mem_init(void);             /* must be called once at startup */
void *alloc(int size);            /* allocate `size` bytes from pool */
void  dealloc(void *ptr);         /* release previously alloc'd block */
void  mem_dump_stats(void);       /* print used/free bytes for debugging */
int   mem_used(void);             /* return bytes currently in use */

#endif /* MEMORY_H */
