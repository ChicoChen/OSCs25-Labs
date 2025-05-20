#ifndef MEM_REGION_H
#define MEM_REGION_H

#include "basic_type.h"

#define MEM_SIZE 0x3C000000 // 240k * 4KB = 960 MB

// hard-coded spin table address for rasberry_pi_3b
#define SPIN_TABLE_START 0x0
#define SPIN_TABLE_SIZE 0x1000

// page array region
#define PAGE_ARRAY_START 0x10000000
extern size_t page_array_size;
#define PAGE_ARRAY_SIZE page_array_size

// kernel code region, defined in kernel8.ld
extern void *_kernel_start; 
extern void *_stack_top;
#define KERNEL_START (addr_t)&_kernel_start
#define KERNEL_END (addr_t)&_stack_top

// memory region for user program
#define USER_STACK_SIZE 0x040000 // 256KB space
#define KERNEL_STACK_SIZE 0x010000 // 64KB space

// dtb address, defined in kernel8.S
extern void *_dtb_addr;
extern size_t dtb_size;

// reserved region for .cpio, defined file_sys/initramfs.c
extern void *initramfs_addr;
extern size_t initramfs_size;

#endif