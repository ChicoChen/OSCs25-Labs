ENTRY(_kernel8)

STACK_SIZE = 0x40000; /* 256kB */
HEAP_SIZE = 0x40000; /* 256kB */

SECTIONS
{
    /*raspi load image to 0x80000 */
    . = 0x80000;
    .text.boot : {KEEP(*(.text.boot))}
    .text : {*(.text)}

    .rodata : {*(.rodata*)}
    .data : {*(.data)}

    . = ALIGN(0x8);
    bss_begin = .;
    .bss(NOLOAD) : {*(.bss*)}

    . = ALIGN(0x10);
    bss_end = .;
    _heap_start = .;

    . = _heap_start + HEAP_SIZE;
    _heap_end = .;

    /* _stack_top = _heap_start + HEAP_SIZE + STACK_SIZE */
    _stack_top = . + STACK_SIZE;
    . = _stack_top;
}