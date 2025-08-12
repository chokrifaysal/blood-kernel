# x86 context switch - pure asm, no libc
.code32
.section .text
.global context_switch
.type context_switch, @function

context_switch:
    pusha                   # save caller-saved regs
    
    mov 4(%esp), %eax       # old_sp ptr
    mov %esp, (%eax)        # store current sp
    
    mov 8(%esp), %eax       # new_sp
    mov %eax, %esp          # load new sp
    
    popa                    # restore regs
    
    ret
.size context_switch, . - context_switch
