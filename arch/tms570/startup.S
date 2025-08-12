@ TMS570LS3137 startup - TI Hercules
.syntax unified
.cpu cortex-r4f

.section .isr_vector, "a"
vectors:
    .word _estack
    .word reset_handler
    .word undef_handler
    .word swi_handler
    .word pabort_handler
    .word dabort_handler
    .word 0                 // reserved
    .word irq_handler

.section .text
.type reset_handler, @function
reset_handler:
    ldr r0, =_estack
    mov sp, r0
    
    bl kernel_main
    
hang:
    b hang
.size reset_handler, . - reset_handler

.type irq_handler, @function
irq_handler:
    save {r0-r12, lr}
    bl can_irq_handler
    restore {r0-r12, pc}^
