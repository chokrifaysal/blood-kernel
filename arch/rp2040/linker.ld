ENTRY(reset_handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 2M
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 264K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .vector_table : { KEEP(*(.vector_table)) } > FLASH
    .text         : { *(.text*) } > FLASH
    .rodata       : { *(.rodata*) } > FLASH
    .data : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) {
        _sidata = LOADADDR(.data);
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM
    .bss : {
        _sbss = .;
        *(COMMON)
        *(.bss*)
        _ebss = .;
    } > RAM
}
