#ifndef DTB_H
#define DTB_H
#include "../basic_type.h"

#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP        0x4
#define FDT_END        0x9

#define DTB_BLOCK_ALIGN 4 // 32-bit aligned

typedef struct {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} fdt_header;

typedef void(*callback_func)(unsigned int type, char *name, void *data, size_t len);

int dtb_parser(callback_func func,  addr_t dtb_addr);

// callback functions
void print_dts(unsigned int type, char *name, void *data, size_t len);

static unsigned int depth = 0;

uint32_t *add_padding(addr_t addr, size_t padding);

#endif