/* PIC32MZ2048EFH linker script – 200 MHz, 2 MB flash, 512 kB RAM */
ENTRY(_start)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x9D000000, LENGTH = 2M
    RAM   (rwx): ORIGIN = 0x80000000, LENGTH = 512K
    RAM_KS0 (rwx): ORIGIN = 0xA0000000, LENGTH = 32K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .text : ALIGN(4) {
        *(.text.boot)
        *(.text*)
    } > FLASH

    .rodata : ALIGN(4) {
        *(.rodata*)
    } > FLASH

    .data : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) {
        _sidata = LOADADDR(.data);
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM

    .bss : ALIGN(4) {
        _sbss = .;
        *(COMMON)
        *(.bss*)
        _ebss = .;
    } > RAM
}
