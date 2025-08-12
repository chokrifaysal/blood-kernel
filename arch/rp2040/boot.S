/*
 * boot.S - RP2040 reset vector, Cortex-M0+
 */

.syntax unified
.cpu cortex-m0plus
.thumb

.section .vector_table, "a"
vectors:
    .word _estack
    .word reset_handler
    .word nmi_handler
    .word hardfault_handler
    .word 0, 0, 0, 0
    .word 0, 0, 0
    .word svcall_handler
    .word 0, 0
    .word pendsv_handler
    .word systick_handler

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

.type svcall_handler, @function
svcall_handler: b .

.type pendsv_handler, @function
pendsv_handler: b .

.type systick_handler, @function
systick_handler: b .
