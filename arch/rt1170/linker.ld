/* RT1170 – 2 MB flash, 1 MB SRAM, 1 GHz */
ENTRY(reset_handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x30000000, LENGTH = 2M
    DTCM  (rwx): ORIGIN = 0x20000000, LENGTH = 512K
    SRAM  (rwx): ORIGIN = 0x20200000, LENGTH = 512K
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
    .vector_table : ALIGN(512) { KEEP(*(.vector_table)) } > FLASH
    .text         : ALIGN(4)   { *(.text*) } > FLASH
    .rodata       : ALIGN(4)   { *(.rodata*) } > FLASH
    .data : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) { _sidata = LOADADDR(.data); _sdata = .; *(.data*) _edata = .; } > SRAM
    .bss  : ALIGN(4) { _sbss = .; *(COMMON) *(.bss*) _ebss = .; } > SRAM
}
