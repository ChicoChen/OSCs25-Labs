.section ".text.boot"
.global _boot
_boot:
  MRS x0, mpidr_el1
  AND x0, x0, #0xFF

  CBZ x0, _init

_halt:
  wfe
  B _halt

_init:
  ADR x0, bss_begin
  ADR x1, bss_end
  // pollute x0 on purpose to check correctness of _clear_bss
  // STR x1, [x0], #8 
  
_clear_bss:
  CMP x0, x1
  B.GE _bss_done
  STR xzr, [x0], #8
  B _clear_bss
  
_bss_done:
  ADR x1, _stack_top
  mov sp, x1
  //TODO
  bl sys_entry
  wfe