ENTRY(task_main)

SECTIONS
{
    . = 0x10010000;   /* user task load address */
    
    .text : {
        *(.text*)
    }
    
    .rodata : {
        *(.rodata*)
    }
    
    .data : {
        *(.data*)
    }
    
    .bss : {
        *(COMMON)
        *(.bss*)
    }
    
    .stack ORIGIN(RAM) + LENGTH(RAM) - 0x400 : {
        *(.stack)
    }
}
