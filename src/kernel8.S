.section ".text.boot"
.global _kernel8
_kernel8:
  ldr x1, =_dtb_addr
  str x0, [x1]
  bl _el2_to_el1

  MRS x0, mpidr_el1
  AND x0, x0, #0xFF

  CBZ x0, _init

_halt:
  wfe
  B _halt

_init:
  ADR x0, bss_begin
  ADR x1, bss_end

_clear_bss:
  CMP x0, x1
  B.GE _bss_done
  STR xzr, [x0], #8
  B _clear_bss
  
_bss_done:
  ADR x1, _stack_top
  mov sp, x1
  bl kernel_entry
  wfe

_el2_to_el1:
  mov x0, (1 << 31) // EL1 uses aarch64
  msr hcr_el2, x0
  mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
  msr spsr_el2, x0
  msr elr_el2, lr
  eret // return to EL1

.section ".data"
.global _dtb_addr
_dtb_addr: .dc.a 0x08