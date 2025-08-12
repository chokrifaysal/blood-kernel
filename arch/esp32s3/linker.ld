/* ESP32-S3 RISC-V linker script */
ENTRY(_start)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x40370000, LENGTH = 2M
    RAM   (rwx): ORIGIN = 0x3FC88000, LENGTH = 320K
    RTC_SLOW (rwx): ORIGIN = 0x50000000, LENGTH = 8K
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
