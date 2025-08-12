@ ARM Cortex-M context switch - PendSV handler
.syntax unified
.cpu cortex-m4
.thumb

.section .text
.global context_switch
.type context_switch, @function

context_switch:
    push {r0, lr}           # save regs
    
    ldr r1, [r0]            # *old_sp
    str sp, [r1]            # store current sp
    
    mov sp, r1              # load new sp
    
    pop {r0, pc}            # return
.size context_switch, . - context_switch
