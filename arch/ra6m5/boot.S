/*
 * boot.S – RA6M5 Cortex-M33 cold-boot
 * Jumps from BootROM → _start @ 0x00000000
 */

.syntax unified
.cpu cortex-m33
.thumb

.section .vector_table, "a"
vectors:
    .word _estack
    .word reset_handler
    .word nmi_handler
    .word hardfault_handler
    .word 0,0,0,0
    .word 0,0,0,0,0,0,0
    .word systick_handler
    .word 0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0

.section .text
.type reset_handler, @function
reset_handler:
    ldr   r0, =_estack
    mov   sp, r0
    bl    kernel_main
hang:
    b     hang

.type nmi_handler, @function
nmi_handler: b .

.type hardfault_handler, @function
hardfault_handler: b .

.type systick_handler, @function
systick_handler: b .
