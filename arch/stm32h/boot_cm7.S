/*
 * boot_cm7.S – STM32H745 Cortex-M7 primary core
 * Runs @ 480 MHz, powers on CM4 via RCC
 */

.syntax unified
.cpu cortex-m7
.thumb

.section .vector_table, "a"
vectors:
    .word _estack_m7
    .word reset_handler_m7
    .word nmi_handler
    .word hardfault_handler
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word systick_handler

.section .text
.type reset_handler_m7, @function
reset_handler_m7:
    ldr   r0, =_estack_m7
    mov   sp, r0
    bl    enable_cm4
    bl    clock_init
    bl    kernel_main
hang: b hang

enable_cm4:
    /* RCC->CR |= D2CKEN | D1CKEN */
    ldr r1, =0x58024400
    ldr r2, [r1]
    orr r2, r2, (1<<16) | (1<<23)
    str r2, [r1]
    bx  lr
