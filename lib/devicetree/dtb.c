#include "devicetree/dtb.h"
#include "mini_uart.h"
#include "str_utils.h"
#include "utils.h"
#include "file_sys/initramfs.h"

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

void find_initramfs(unsigned int type, char *name, void *data, size_t len){
    if(type == FDT_PROP && strcmp("linux,initrd-start", name)){
        unsigned int cpio_addr = to_le_u32(*(unsigned int*)data);
        send_string("initramfs address found: ");
        char addr[11];
        send_line(itoa(cpio_addr, addr, HEX));
        set_initramfs_addr(cpio_addr);
    }
}

void print_dts(unsigned int type, char *name, void *data, size_t len){
    char buffer[32];
    switch(type){
    case FDT_BEGIN_NODE:
        make_str(buffer, ' ' , depth * 2);
        send_string(buffer);
        send_string(name);
        send_line(":{");
        depth++;
        break;
        
    case FDT_END_NODE:
        depth--;
        make_str(buffer, ' ', depth * 2);
        send_string(buffer);
        send_line("}");
        break;
    case FDT_PROP:
        send_string(buffer);
        send_string(name);
        if(len == 0) break;
        send_string(": ");
        for(int i = 0; i < len; i++){
            send_data(((char *)data)[i]);
        }
        send_line("");
        break;
    }
}
    
uint32_t *add_padding(addr_t addr, size_t padding){
    unsigned long long offset = addr % padding;
    if(offset != 0) addr += padding - offset;
    return (uint32_t *)addr;
}
