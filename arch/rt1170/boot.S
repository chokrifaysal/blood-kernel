/* boot.S – RT1170 M7 cold-boot from ROM */
.syntax unified
.cpu cortex-m7
.thumb
.section .vector_table, "a"
vectors:
    .word _estack
    .word reset_handler
    .word nmi_handler
    .word hardfault_handler
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word systick_handler
.section .text
.type reset_handler, @function
reset_handler:
    ldr r0, =_estack
    mov sp, r0
    bl  kernel_main
    hang: b hang
