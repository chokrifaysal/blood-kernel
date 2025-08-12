/* BCM2711 linker script – 64-bit */
ENTRY(_start)

SECTIONS
{
    . = 0x80000;
    
    .text : ALIGN(4K) {
        *(.text.boot)
        *(.text*)
    }
    
    .rodata : ALIGN(4K) {
        *(.rodata*)
    }
    
    .data : ALIGN(4K) {
        *(.data*)
    }
    
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss*)
    }
    
    _stack_top = . + 0x10000;
}
