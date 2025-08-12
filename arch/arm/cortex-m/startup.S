@ ARM Cortex-M startup - straight from reset vector
.syntax unified
.cpu cortex-m4
.thumb

.section .isr_vector, "a"
vectors:
    .word _estack
    .word reset_handler
    .word nmi_handler
    .word hardfault_handler
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved
    .word 0                 # reserved

.section .text
.type reset_handler, %function
reset_handler:
    ldr r0, =_estack
    mov sp, r0
    
    bl kernel_main
    
hang:
    b hang
.size reset_handler, . - reset_handler

.type nmi_handler, %function
nmi_handler:
    b .
.size nmi_handler, . - nmi_handler

.type hardfault_handler, %function
hardfault_handler:
    b .
.size hardfault_handler, . - hardfault_handler
