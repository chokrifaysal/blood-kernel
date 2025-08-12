; boot.S – AVR128DA48 reset vector
; GCC-AVR expects .init0 → .init9 → main
    .section .init0,"ax"
    .global __init
__init:
    rjmp reset_handler

    .section .init9,"ax"
reset_handler:
    ldi r16, low(RAMEND)
    out SPL, r16
    ldi r16, high(RAMEND)
    out SPH, r16
    rcall kernel_main
loop:
    rjmp loop
