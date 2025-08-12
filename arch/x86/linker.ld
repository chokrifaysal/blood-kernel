/* x86 linker script - simple, no fancy shit */
ENTRY(_start)

SECTIONS
{
    . = 0x100000;  /* 1MB - standard kernel load addr */
    
    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text)
    }
    
    .rodata ALIGN(4K) : {
        *(.rodata)
    }
    
    .data ALIGN(4K) : {
        *(.data)
    }
    
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }
    
    . = 0x8000;    /* task stacks */
    .task_stacks : {
        *(.task_stacks)
    }
}
