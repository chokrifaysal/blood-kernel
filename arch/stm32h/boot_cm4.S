/*
 * boot_cm4.S – STM32H745 Cortex-M4 secondary core
 * Starts via mailbox from CM7
 */

.syntax unified
.cpu cortex-m4
.thumb

.section .vector_table, "a"
vectors:
    .word _estack_m4
    .word reset_handler_m4
    .word nmi_handler
    .word hardfault_handler
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word systick_handler

.section .text
.type reset_handler_m4, @function
reset_handler_m4:
    ldr   r0, =_estack_m4
    mov   sp, r0
    bl    kernel_main_m4
hang: b hang
