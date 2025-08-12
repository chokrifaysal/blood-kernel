/*
 * boot.S - Raspberry Pi 4 64-bit bring-up
 * Entry at 0x80000 (EL2 -> EL1)
 */

.section .text.boot
.global _start
.type _start, @function

_start:
    // Hang in EL3/EL2 until we drop to EL1
    mrs x0, currentel
    and x0, x0, #0xC
    cmp x0, #0x8        // EL2?
    b.eq 1f
    cmp x0, #0xC        // EL3?
    b.eq 1f
    b .

1:  // EL2 -> EL1
    mov x0, #0x33c      // EL1 SP0 | EL1 SPx | EL0
    msr spsr_el2, x0
    adr x0, 2f
    msr elr_el2, x0
    eret

2:  // EL1 entry
    ldr x0, =_stack_top
    mov sp, x0
    bl kernel_main

hang:
    wfe
    b hang
