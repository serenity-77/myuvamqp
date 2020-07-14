#include "myuvamqp_mem.h"


void *myuvamqp_mem_alloc(size_t size)
{
    return malloc(size);
}

void *myuvamqp_mem_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void myuvamqp_mem_free(void *ptr)
{
    if(ptr) free(ptr);
}
