/* AVR128DA48 – 128 kB flash, 16 kB SRAM, 20 MHz */
ENTRY(reset_handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x0000, LENGTH = 128K
    RAM   (rwx): ORIGIN = 0x800100, LENGTH = 16K
}

SECTIONS
{
    .text : ALIGN(2) {
        KEEP(*(.init0))
        KEEP(*(.init1))
        KEEP(*(.init2))
        KEEP(*(.init3))
        KEEP(*(.init4))
        KEEP(*(.init5))
        KEEP(*(.init6))
        KEEP(*(.init7))
        KEEP(*(.init8))
        KEEP(*(.init9))
        *(.text*)
    } > FLASH

    .rodata : ALIGN(2) {
        *(.rodata*)
    } > FLASH

    .data : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) {
        _sidata = LOADADDR(.data);
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM

    .bss : ALIGN(2) {
        _sbss = .;
        *(COMMON)
        *(.bss*)
        _ebss = .;
    } > RAM
}
