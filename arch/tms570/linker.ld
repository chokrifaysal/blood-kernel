ENTRY(reset_handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 3M
    RAM   (rwx): ORIGIN = 0x08000000, LENGTH = 256K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text       : { *(.text*) } > FLASH
    .rodata     : { *(.rodata*) } > FLASH
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
