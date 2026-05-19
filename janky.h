#ifndef JANKY_H
#define JANKY_H

#include <stddef.h>

/* janky is a tiny freestanding allocator
 *
 * shadow the C library's malloc/free/realloc so the
 * allocator can be linked into a program in place of libc's.
 */
void *malloc(size_t size);
void  free(void *ptr);
void *realloc(void *ptr, size_t new_size);

/* debug helper: pretty-print the current heap layout to stdout */
void  janky_print_heap(void);

#ifdef JANKY_TEST
/* reset allocator state, making all existing heap pointers invalid */
void  janky_reset(void);
#endif

#endif /* JANKY_H */
