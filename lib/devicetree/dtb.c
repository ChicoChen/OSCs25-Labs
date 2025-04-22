#include "devicetree/dtb.h"
#include "memory_region.h"
#include "mini_uart.h"
#include "str_utils.h"
#include "utils.h"

size_t dtb_size = 0;

void init_dtb(){
    if(!_dtb_addr) return;
    char temp[16];
    dtb_size = ((fdt_header *)_dtb_addr)->totalsize;
    dtb_size = to_le_u32(dtb_size);

    send_string("dtb addr: ");
    send_line(itoa((uint32_t)_dtb_addr, temp, HEX));
    send_string("dtb size: ");
    send_line(itoa((uint32_t)dtb_size, temp, HEX));
}

int dtb_parser(callback_func func, addr_t dtb_addr){
    if(dtb_addr < 10) {
        send_line("dtb address invalid");
        char address[11];
        send_line(itoa(dtb_addr, address, HEX));
        return 1;
    }

    fdt_header *header = (fdt_header *)dtb_addr;
    char *string_block = (char*)(dtb_addr + to_le_u32(header->off_dt_strings));
    
    uint32_t *struct_block = (uint32_t *)(dtb_addr + to_le_u32(header->off_dt_struct));
    uint32_t *read_head = struct_block;
    
    uint32_t token = to_le_u32(*read_head++);
    while(token == FDT_NOP) token = to_le_u32(*read_head++);
    
    while(token != FDT_END){
        char *name;
        switch (token)
        {
         case FDT_BEGIN_NODE:
            name = (char *)read_head;
            read_head = add_padding((addr_t)(name + get_size(name)), DTB_BLOCK_ALIGN);
            break;
            
         case FDT_PROP:
            uint32_t len = to_le_u32(*(read_head++));
            uint32_t nameoff = to_le_u32(*(read_head++));
            name = string_block + nameoff;
            func(token, name, (void *) read_head, len);
            read_head = add_padding((addr_t)(read_head) + len, DTB_BLOCK_ALIGN);
            break;
         case FDT_END_NODE:
         case FDT_NOP:
            break;
         default:
            send_line("error: unknowen dtb structure node");
        }   
        token = to_le_u32(*read_head++);
    }
    return 0;
}

void print_dts(unsigned int type, char *name, void *data, size_t len){
    char buffer[32];
    switch(type){
    case FDT_BEGIN_NODE:
        make_str(buffer, ' ' , depth * 2);
        _send_string_(buffer, sync_send_data);
        _send_string_(name, sync_send_data);
        send_line(":{");
        depth++;
        break;
        
    case FDT_END_NODE:
        depth--;
        make_str(buffer, ' ', depth * 2);
        _send_string_(buffer, sync_send_data);
        _send_line_("}", sync_send_data);
        break;
    case FDT_PROP:
        _send_string_(buffer, sync_send_data);
        _send_string_(name, sync_send_data);
        if(len == 0) break;
        _send_string_(": ", sync_send_data);
        for(int i = 0; i < len; i++){
            async_send_data(((char *)data)[i]);
        }
        _send_line_("", sync_send_data);
        break;
    }
}
    
uint32_t *add_padding(addr_t addr, size_t padding){
    unsigned long long offset = addr % padding;
    if(offset != 0) addr += padding - offset;
    return (uint32_t *)addr;
}
