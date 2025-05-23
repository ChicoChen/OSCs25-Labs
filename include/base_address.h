#ifndef ADDRESS_H
#define ADDRESS_H

// ----- peripherals -----
#define MMIO_BASE   0x3F000000

#define GPIO_BASE       MMIO_BASE + 0x200000
#define MAILBOX_BASE    MMIO_BASE + 0xB880
// ----- interrupts -----
#define ARM_INTERRUPT_BASE  MMIO_BASE + 0xB000
#define CORE_INTERRUPT_BASE 0x40000000

#endif