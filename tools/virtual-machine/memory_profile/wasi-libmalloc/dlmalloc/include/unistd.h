/* Stub include file to support dlmalloc. */

#include <__macro_PAGESIZE.h>
#include <stdint.h>

#define sysconf(name) PAGESIZE
#define _SC_PAGESIZE

/* Declare sbrk. */
void *sbrk(intptr_t increment) __attribute__((__warn_unused_result__));
