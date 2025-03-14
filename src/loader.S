.section ".text.relocate"
.global _relocate_entry
_relocate_entry:
  adr x0, .
  ldr x1,= _loader_start
  
  cmp x0, x1
  ldr x2,= _codesize
  b.eq _loader_entry

_relocation:
  ldr x3, [x0], #8
  str x3, [x1], #8
  sub x2, x2, #8
  cbnz x2, _relocation
  ldr x4,= _loader_entry
  br x4

.section ".text.boot"
.global _loader_entry
_loader_entry:
  mrs x0, mpidr_el1
  and x0, x0, #0xFF
  cbz x0, _init

_halt:
  wfe
  b _halt 

_init:
  adr x0, bss_begin
  adr x1, bss_end

_clear_bss:
  cmp x0, x1
  b.ge _exec
  str xzr, [x0], #8
  b _clear_bss

_exec:
  adr x1, _stack_top
  mov sp, x1
  bl loader_entry
  ldr x1,= _kernel_start
  br x1

