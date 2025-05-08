// save general registers to stack
.macro save_all
    sub sp, sp, 34 * 8 // 31 + 3 register
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    mrs x0, spsr_el1
    mrs x1, elr_el1
    mrs x2, esr_el1
    stp x30, x0,  [sp, 16 * 15]
    stp x1,  x2,  [sp, 16 * 16]
.endm

// load general registers from stack
.macro load_all
    ldp x1,  x2, [sp ,16 * 16]
    ldp x30, x0, [sp ,16 * 15]
    msr spsr_el1, x0
    msr elr_el1, x1
    msr esr_el1, x2

    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    add sp, sp, 34 * 8
.endm

.section ".text"
.global _el1_to_el0
.type _el1_to_el0 %function
_el1_to_el0: // TODO: inline assembly version?
  msr DAIFSet, 0xf
  msr elr_el1, x0
  msr sp_el0, x1
  mov x0, 0x3c0
  msr spsr_el1, x0
  eret

.align 11 // aligned to 0x800
.global _exception_vector_table
_exception_vector_table:
  b _default_handler // default handler
  .align 7 // aligned to 0x80, will pad 0
  b _default_handler
  .align 7
  b _default_handler
  .align 7
  b _default_handler
  .align 7

  b _default_handler
  .align 7
  b _curr_irq_entry // irq interrupt form the same EL_x
  .align 7
  b _default_handler
  .align 7
  b _default_handler
  .align 7

  b _lower_sync_entry
  .align 7
  b _default_handler
  .align 7
  b _default_handler
  .align 7
  b _default_handler
  .align 7

  b _default_handler
  .align 7
  b _default_handler
  .align 7
  b _default_handler
  .align 7
  b _default_handler
  .align 7

_default_handler:
  msr DAIFSet, 0xf
  save_all
  mrs x0, spsr_el1
  mrs x1, elr_el1
  mrs x2, esr_el1
  bl print_el_message
  load_all
  // msr DAIFClr, 0xf // don't need to enable DAIF again since eret restore PSTATE
  eret

_curr_irq_entry:
  msr DAIFSet, 0xf
  save_all
  bl curr_irq_handler
  load_all
  eret

_lower_sync_entry:
  msr DAIFSet, 0xf
  save_all
  mov x0, sp
  bl lower_sync_handler
  load_all
  eret