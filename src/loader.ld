ENTRY(_relocate_entry)

STACK_SIZE = 0x8000; /* 32kB */
HEAP_SIZE = 0x8000; /* 32kB */

_loader_start = 0x60000;
_kernel_start = 0x80000;

SECTIONS
{
    /*raspi load image to 0x80000 */
    . = _loader_start;
    .text.relocate : {KEEP(*(.text.relocate))}
    .text.boot : {KEEP(*(.text.boot))}
    .text : {*(.text)}

    .rodata : {*(.rodata*)}
    .data : {*(.data)}

    . = ALIGN(0x8);
    bss_begin = .;
    .bss(NOLOAD) : {*(.bss*)}

    . = ALIGN(0x10);
    bss_end = .;

    . = bss_end + HEAP_SIZE;

    _stack_top = . + STACK_SIZE;
    . = _stack_top;
    PROVIDE(_codesize = . - _loader_start);
}