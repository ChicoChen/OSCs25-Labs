.section ".text.relocate"
.global _relocate_entry
_relocate_entry:
  adr x10, .
  ldr x11,= _loader_start
  mov x20, x0
  
  cmp x10, x11
  ldr x2,= _codesize
  b.eq _loader_entry

_relocation:
  ldr x3, [x10], #8
  str x3, [x11], #8
  sub x2, x2, #8
  cbnz x2, _relocation
  ldr x4,= _loader_entry
  br x4

.section ".text.boot"
.global _loader_entry
_loader_entry:
  mrs x10, mpidr_el1
  and x10, x10, #0xFF
  cbz x10, _init

_halt:
  wfe
  b _halt 

_init:
  adr x10, bss_begin
  adr x11, bss_end

_clear_bss:
  cmp x10, x11
  b.ge _exec
  str xzr, [x10], #8
  b _clear_bss

_exec:
  adr x11, _stack_top
  mov sp, x11
  bl loader_entry
  mov x0, x20
  ldr x11,= _kernel_start
  br x11

