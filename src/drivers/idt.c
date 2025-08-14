/*
 * idt.c – x86 Interrupt Descriptor Table and handlers
 */

#include "kernel/types.h"

#define IDT_ENTRIES 256

/* IDT gate types */
#define IDT_TASK_GATE    0x5
#define IDT_INT_GATE     0xE
#define IDT_TRAP_GATE    0xF

/* IDT flags */
#define IDT_PRESENT      0x80
#define IDT_DPL_0        0x00
#define IDT_DPL_3        0x60

typedef struct {
    u16 offset_low;
    u16 selector;
    u8 zero;
    u8 type_attr;
    u16 offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

/* Exception handlers */
extern void isr0(void);   /* Division by zero */
extern void isr1(void);   /* Debug */
extern void isr2(void);   /* NMI */
extern void isr3(void);   /* Breakpoint */
extern void isr4(void);   /* Overflow */
extern void isr5(void);   /* Bound range exceeded */
extern void isr6(void);   /* Invalid opcode */
extern void isr7(void);   /* Device not available */
extern void isr8(void);   /* Double fault */
extern void isr10(void);  /* Invalid TSS */
extern void isr11(void);  /* Segment not present */
extern void isr12(void);  /* Stack fault */
extern void isr13(void);  /* General protection fault */
extern void isr14(void);  /* Page fault */
extern void isr16(void);  /* x87 FPU error */
extern void isr17(void);  /* Alignment check */
extern void isr18(void);  /* Machine check */
extern void isr19(void);  /* SIMD FPU error */

/* IRQ handlers */
extern void irq0(void);   /* PIT timer */
extern void irq1(void);   /* Keyboard */
extern void irq2(void);   /* Cascade */
extern void irq3(void);   /* COM2 */
extern void irq4(void);   /* COM1 */
extern void irq5(void);   /* LPT2 */
extern void irq6(void);   /* Floppy */
extern void irq7(void);   /* LPT1 */
extern void irq8(void);   /* RTC */
extern void irq9(void);   /* Free */
extern void irq10(void);  /* Free */
extern void irq11(void);  /* Free */
extern void irq12(void);  /* PS/2 mouse */
extern void irq13(void);  /* FPU */
extern void irq14(void);  /* Primary ATA */
extern void irq15(void);  /* Secondary ATA */

static void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (u32)&idt;
    
    /* Clear IDT */
    for (u16 i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    /* Install exception handlers */
    idt_set_gate(0, (u32)isr0, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(1, (u32)isr1, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(2, (u32)isr2, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(3, (u32)isr3, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(4, (u32)isr4, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(5, (u32)isr5, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(6, (u32)isr6, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(7, (u32)isr7, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(8, (u32)isr8, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(10, (u32)isr10, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(11, (u32)isr11, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(12, (u32)isr12, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(13, (u32)isr13, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(14, (u32)isr14, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(16, (u32)isr16, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(17, (u32)isr17, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(18, (u32)isr18, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(19, (u32)isr19, 0x08, IDT_PRESENT | IDT_INT_GATE);
    
    /* Install IRQ handlers */
    idt_set_gate(32, (u32)irq0, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(33, (u32)irq1, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(34, (u32)irq2, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(35, (u32)irq3, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(36, (u32)irq4, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(37, (u32)irq5, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(38, (u32)irq6, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(39, (u32)irq7, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(40, (u32)irq8, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(41, (u32)irq9, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(42, (u32)irq10, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(43, (u32)irq11, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(44, (u32)irq12, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(45, (u32)irq13, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(46, (u32)irq14, 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_gate(47, (u32)irq15, 0x08, IDT_PRESENT | IDT_INT_GATE);
    
    /* Load IDT */
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

/* Exception names */
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point Exception"
};

void isr_handler(u32 int_no, u32 err_code, u32 eip, u32 cs, u32 eflags) {
    extern void vga_puts(const char* str);
    extern void vga_set_color(u8 fg, u8 bg);
    extern void vga_printf(const char* fmt, ...);
    
    vga_set_color(15, 4); /* White on red */
    vga_puts("\n*** EXCEPTION ***\n");
    
    if (int_no < 20) {
        vga_puts(exception_messages[int_no]);
    } else {
        vga_puts("Unknown Exception");
    }
    
    vga_printf("\nINT: %u  ERR: %u\n", int_no, err_code);
    vga_printf("EIP: %08X  CS: %04X\n", eip, cs);
    vga_printf("EFLAGS: %08X\n", eflags);
    
    if (int_no == 14) { /* Page fault */
        u32 faulting_addr;
        __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_addr));
        vga_printf("Page fault at: %08X\n", faulting_addr);
        
        if (err_code & 1) vga_puts("Protection violation\n");
        else vga_puts("Page not present\n");
        
        if (err_code & 2) vga_puts("Write operation\n");
        else vga_puts("Read operation\n");
        
        if (err_code & 4) vga_puts("User mode\n");
        else vga_puts("Kernel mode\n");
    }
    
    vga_puts("\nSystem halted.\n");
    
    /* Halt the system */
    __asm__ volatile("cli; hlt");
    while (1);
}

void irq_handler(u32 irq_no) {
    extern void pit_irq_handler(void);
    extern void ps2_kbd_irq_handler(void);
    
    /* Handle specific IRQs */
    switch (irq_no) {
        case 0: /* PIT timer */
            pit_irq_handler();
            break;
        case 1: /* Keyboard */
            ps2_kbd_irq_handler();
            break;
        case 14: /* Primary ATA */
        case 15: /* Secondary ATA */
            /* ATA interrupt handling */
            break;
        default:
            /* Unhandled IRQ */
            break;
    }
    
    /* Send EOI to PIC */
    if (irq_no >= 8) {
        /* Secondary PIC */
        __asm__ volatile("outb %0, $0xA0" : : "a"((u8)0x20));
    }
    /* Primary PIC */
    __asm__ volatile("outb %0, $0x20" : : "a"((u8)0x20));
}
