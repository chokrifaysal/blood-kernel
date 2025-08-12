/*
 * boot.S – PIC32MZ2048EFH reset vector (MIPS32)
 * Entry @ 0xBFC00000 (KSEG1)
 */

.section .text.boot
.set noreorder
.set nomips16

.global _start
.type _start, @function

_start:
    la   $sp, _stack_top
    jal  kernel_main
    nop

hang:
    wait
    j    hang
