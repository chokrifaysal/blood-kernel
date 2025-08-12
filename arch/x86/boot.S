# x86 boot code - gets us from multiboot to C
.code32
.section .multiboot, "a"
    .long 0x1BADB002          # magic
    .long 0x00000003          # flags
    .long -(0x1BADB002 + 0x00000003)  # checksum

.section .bss
    .align 16
stack_bottom:
    .space 16384              # 16k stack
stack_top:

.section .text
.global _start
.type _start, @function

_start:
    movl $stack_top, %esp     # setup stack
    
    pushl %ebx               # multiboot magic
    pushl %eax               # multiboot info
    
    call kernel_main
    
halt:
    hlt
    jmp halt

.size _start, . - _start
