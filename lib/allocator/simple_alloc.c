#include "allocator/simple_alloc.h"
#include "mini_uart.h"
#include "str_utils.h"

char* simple_alloc(unsigned int size){
    if(memory_ptr + size > HEAP_END) {
        send_line("!out of heap space");
        return 0;
    }

    char *allocated = memory_ptr;
    memory_ptr += size;
    return allocated;
}

int memalloc(void *args){

    unsigned int start = (unsigned int)HEAP_START;
    unsigned int end = (unsigned int)HEAP_END;

    char *string = simple_alloc(11);
    send_string("allocated memory: ");
    send_line(itoa((unsigned int)string, string, HEX));

    send_string("next alloc will happend at: ");
    send_line(itoa((unsigned int)memory_ptr, string, HEX));

}