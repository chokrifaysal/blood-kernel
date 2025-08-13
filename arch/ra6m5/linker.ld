/* RA6M5 – 2 MB flash, 640 kB SRAM, 200 MHz */
ENTRY(reset_handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 2M
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 640K
    TZ_RAM (rwx): ORIGIN = 0x20080000, LENGTH = 32K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .vector_table : ALIGN(512) {
        KEEP(*(.vector_table))
    } > FLASH

    .text : ALIGN(4) {
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

    .tz_bss (NOLOAD) : {
        *(.tz_bss*)
    } > TZ_RAM
}
