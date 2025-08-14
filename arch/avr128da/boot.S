; boot.S – AVR128DA48 reset vector + TCA0 ISR
; GCC-AVR expects .init0 → .init9 → main
    .section .init0,"ax"
    .global __init
__init:
    rjmp reset_handler

    .section .vectors,"ax"
    .org 0x40
    rjmp tca0_ovf_isr

    .section .init9,"ax"
reset_handler:
    ldi r16, low(RAMEND)
    out SPL, r16
    ldi r16, high(RAMEND)
    out SPH, r16
    rcall kernel_main
loop:
    rjmp loop

    .section .text
    .global tca0_ovf_isr
tca0_ovf_isr:
    push r16
    in r16, SREG
    push r16
    rcall tca0_tick_handler
    pop r16
    out SREG, r16
    pop r16
    reti
