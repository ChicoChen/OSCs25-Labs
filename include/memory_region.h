#ifndef MEM_REGION_H
#define MEM_REGION_H

#define MEM_SIZE 0x3C000000 // 240k * 4KB = 960 MB

// kernel code region, defined in kernel8.ld
extern void *_kernel_start; 
extern void *_stack_top;
#define KERNEL_START _kernel_start;
#define KERNEL_END _stack_top;

// reserved memory region for user program
#define USER_PROG_START 0x4000000 
#define USER_STACK_TOP 0x4020000 

// hard-coded spin table address for rasberry_pi_3b
#define SPIN_TABLE_START 0x0
#define SPIN_TABLE_SIZE 0x1000

// dtb address, defined in kernel8.S
extern void *_dtb_addr;

// reserved region for .cpio, defined file_sys/initramfs.c
extern void *initramfs_addr;

#endif