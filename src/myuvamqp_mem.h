#include <stdlib.h>


void *myuvamqp_mem_alloc(size_t size);
void *myuvamqp_mem_realloc(void *ptr, size_t size);
void myuvamqp_mem_free(void *ptr);
