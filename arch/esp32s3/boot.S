/*
 * boot.S – ESP32-S3 RISC-V entry
 * Jumps from ROM → _start @ 0x4037_0000
 */

.section .text.boot
.global _start
.type _start, @function

_start:
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    la sp, _stack_top
    call kernel_main
hang:
    wfi
    j hang
