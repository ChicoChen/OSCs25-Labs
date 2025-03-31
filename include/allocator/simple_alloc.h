#ifndef SIMPLE_ALLOC_H
#define SIMPLE_ALLOC_H

extern char _heap_start;
extern char _heap_end;

#define HEAP_START (char*)(&_heap_start)
#define HEAP_END (char*)(&_heap_end)

static char *memory_ptr = HEAP_START;
void *simple_alloc(unsigned int size);
int memalloc(void *args);

#endif