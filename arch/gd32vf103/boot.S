# boot.S – GD32VF103 RISC-V reset vector
.section .text.boot
.global _start
_start:
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    la sp, _stack_top
    call kernel_main
hang: j hang
