/* ARM Cortex-M linker script */
ENTRY(reset_handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : {
        KEEP(*(.isr_vector))
    } > FLASH
    
    .text : {
        *(.text)
        *(.text*)
    } > FLASH
    
    .rodata : {
        *(.rodata)
        *(.rodata*)
    } > FLASH
    
    .data : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) {
        _sidata = LOADADDR(.data);
        _sdata = .;
        *(.data)
        *(.data*)
        _edata = .;
    } > RAM
    
    .bss : {
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM
    
    .stack ORIGIN(RAM) + LENGTH(RAM) - 0x400 : {
        *(.stack)
    } > RAM
    
    .task_stacks : {
        *(.task_stacks)
    } > RAM
}
