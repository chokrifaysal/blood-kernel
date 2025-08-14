/*
 * apic.c – x86 Advanced Programmable Interrupt Controller
 */

#include "kernel/types.h"

/* Local APIC registers */
#define LAPIC_ID          0x020
#define LAPIC_VERSION     0x030
#define LAPIC_TPR         0x080  /* Task Priority Register */
#define LAPIC_APR         0x090  /* Arbitration Priority Register */
#define LAPIC_PPR         0x0A0  /* Processor Priority Register */
#define LAPIC_EOI         0x0B0  /* End of Interrupt */
#define LAPIC_RRD         0x0C0  /* Remote Read Register */
#define LAPIC_LDR         0x0D0  /* Logical Destination Register */
#define LAPIC_DFR         0x0E0  /* Destination Format Register */
#define LAPIC_SVR         0x0F0  /* Spurious Interrupt Vector Register */
#define LAPIC_ISR         0x100  /* In-Service Register */
#define LAPIC_TMR         0x180  /* Trigger Mode Register */
#define LAPIC_IRR         0x200  /* Interrupt Request Register */
#define LAPIC_ESR         0x280  /* Error Status Register */
#define LAPIC_CMCI        0x2F0  /* Corrected Machine Check Interrupt */
#define LAPIC_ICR_LOW     0x300  /* Interrupt Command Register */
#define LAPIC_ICR_HIGH    0x310
#define LAPIC_LVT_TIMER   0x320  /* Local Vector Table Timer */
#define LAPIC_LVT_THERMAL 0x330  /* Thermal Sensor */
#define LAPIC_LVT_PERF    0x340  /* Performance Counter */
#define LAPIC_LVT_LINT0   0x350  /* Local Interrupt 0 */
#define LAPIC_LVT_LINT1   0x360  /* Local Interrupt 1 */
#define LAPIC_LVT_ERROR   0x370  /* Error */
#define LAPIC_TIMER_ICR   0x380  /* Timer Initial Count Register */
#define LAPIC_TIMER_CCR   0x390  /* Timer Current Count Register */
#define LAPIC_TIMER_DCR   0x3E0  /* Timer Divide Configuration Register */

/* I/O APIC registers */
#define IOAPIC_REGSEL     0x00
#define IOAPIC_IOWIN      0x10

#define IOAPIC_REG_ID     0x00
#define IOAPIC_REG_VER    0x01
#define IOAPIC_REG_ARB    0x02
#define IOAPIC_REG_REDTBL 0x10

/* ICR delivery modes */
#define ICR_FIXED         0x00000000
#define ICR_LOWEST        0x00000100
#define ICR_SMI           0x00000200
#define ICR_NMI           0x00000400
#define ICR_INIT          0x00000500
#define ICR_STARTUP       0x00000600

/* ICR destination modes */
#define ICR_PHYSICAL      0x00000000
#define ICR_LOGICAL       0x00000800

/* ICR delivery status */
#define ICR_IDLE          0x00000000
#define ICR_SEND_PENDING  0x00001000

/* ICR level */
#define ICR_DEASSERT      0x00000000
#define ICR_ASSERT        0x00004000

/* ICR trigger mode */
#define ICR_EDGE          0x00000000
#define ICR_LEVEL         0x00008000

/* ICR destination shorthand */
#define ICR_NO_SHORTHAND  0x00000000
#define ICR_SELF          0x00040000
#define ICR_ALL_INCLUDING 0x00080000
#define ICR_ALL_EXCLUDING 0x000C0000

static u32 lapic_base = 0;
static u32 ioapic_base = 0;
static u8 apic_enabled = 0;

static inline u32 lapic_read(u32 reg) {
    return *(volatile u32*)(lapic_base + reg);
}

static inline void lapic_write(u32 reg, u32 value) {
    *(volatile u32*)(lapic_base + reg) = value;
}

static inline u32 ioapic_read(u32 reg) {
    *(volatile u32*)(ioapic_base + IOAPIC_REGSEL) = reg;
    return *(volatile u32*)(ioapic_base + IOAPIC_IOWIN);
}

static inline void ioapic_write(u32 reg, u32 value) {
    *(volatile u32*)(ioapic_base + IOAPIC_REGSEL) = reg;
    *(volatile u32*)(ioapic_base + IOAPIC_IOWIN) = value;
}

void apic_init(u32 lapic_addr, u32 ioapic_addr) {
    lapic_base = lapic_addr;
    ioapic_base = ioapic_addr;
    
    if (!lapic_base) {
        /* Try to get from MSR */
        u32 eax, edx;
        __asm__ volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(0x1B));
        lapic_base = eax & 0xFFFFF000;
    }
    
    if (!lapic_base) {
        return;
    }
    
    /* Enable Local APIC */
    u32 eax, edx;
    __asm__ volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(0x1B));
    eax |= (1 << 11); /* Global enable */
    __asm__ volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(0x1B));
    
    /* Software enable Local APIC */
    lapic_write(LAPIC_SVR, lapic_read(LAPIC_SVR) | 0x100);
    
    /* Set task priority to accept all interrupts */
    lapic_write(LAPIC_TPR, 0);
    
    /* Configure LVT entries */
    lapic_write(LAPIC_LVT_TIMER, 0x10000);   /* Masked */
    lapic_write(LAPIC_LVT_LINT0, 0x8700);    /* ExtINT */
    lapic_write(LAPIC_LVT_LINT1, 0x400);     /* NMI */
    lapic_write(LAPIC_LVT_ERROR, 0x10000);   /* Masked */
    
    /* Clear error status */
    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0);
    
    /* Send EOI to clear any pending interrupts */
    lapic_write(LAPIC_EOI, 0);
    
    apic_enabled = 1;
}

void apic_send_eoi(void) {
    if (apic_enabled) {
        lapic_write(LAPIC_EOI, 0);
    }
}

void apic_send_ipi(u8 dest_apic_id, u8 vector) {
    if (!apic_enabled) return;
    
    /* Wait for previous IPI to complete */
    while (lapic_read(LAPIC_ICR_LOW) & ICR_SEND_PENDING);
    
    /* Set destination */
    lapic_write(LAPIC_ICR_HIGH, (u32)dest_apic_id << 24);
    
    /* Send IPI */
    lapic_write(LAPIC_ICR_LOW, ICR_FIXED | vector);
}

void apic_send_init_ipi(u8 dest_apic_id) {
    if (!apic_enabled) return;
    
    while (lapic_read(LAPIC_ICR_LOW) & ICR_SEND_PENDING);
    
    lapic_write(LAPIC_ICR_HIGH, (u32)dest_apic_id << 24);
    lapic_write(LAPIC_ICR_LOW, ICR_INIT | ICR_PHYSICAL | ICR_ASSERT);
    
    /* Wait 10ms */
    extern void timer_delay(u32 ms);
    timer_delay(10);
    
    /* Deassert */
    while (lapic_read(LAPIC_ICR_LOW) & ICR_SEND_PENDING);
    lapic_write(LAPIC_ICR_LOW, ICR_INIT | ICR_PHYSICAL | ICR_DEASSERT);
}

void apic_send_startup_ipi(u8 dest_apic_id, u8 vector) {
    if (!apic_enabled) return;
    
    while (lapic_read(LAPIC_ICR_LOW) & ICR_SEND_PENDING);
    
    lapic_write(LAPIC_ICR_HIGH, (u32)dest_apic_id << 24);
    lapic_write(LAPIC_ICR_LOW, ICR_STARTUP | ICR_PHYSICAL | vector);
}

void apic_broadcast_ipi(u8 vector) {
    if (!apic_enabled) return;
    
    while (lapic_read(LAPIC_ICR_LOW) & ICR_SEND_PENDING);
    
    lapic_write(LAPIC_ICR_LOW, ICR_FIXED | ICR_ALL_EXCLUDING | vector);
}

u8 apic_get_id(void) {
    if (!apic_enabled) return 0;
    return (lapic_read(LAPIC_ID) >> 24) & 0xFF;
}

void apic_timer_init(u32 frequency) {
    if (!apic_enabled) return;
    
    /* Set divide value to 16 */
    lapic_write(LAPIC_TIMER_DCR, 0x3);
    
    /* Set LVT timer entry */
    lapic_write(LAPIC_LVT_TIMER, 32 | 0x20000); /* Vector 32, periodic */
    
    /* Calculate initial count for desired frequency */
    u32 initial_count = 0xFFFFFFFF;
    lapic_write(LAPIC_TIMER_ICR, initial_count);
    
    /* Wait 100ms to calibrate */
    extern void timer_delay(u32 ms);
    timer_delay(100);
    
    /* Read current count */
    u32 current_count = lapic_read(LAPIC_TIMER_CCR);
    u32 ticks_per_100ms = initial_count - current_count;
    
    /* Calculate ticks per second */
    u32 ticks_per_second = ticks_per_100ms * 10;
    
    /* Set initial count for desired frequency */
    initial_count = ticks_per_second / frequency;
    lapic_write(LAPIC_TIMER_ICR, initial_count);
}

void apic_timer_stop(void) {
    if (!apic_enabled) return;
    lapic_write(LAPIC_LVT_TIMER, 0x10000); /* Masked */
}

void ioapic_init(void) {
    if (!ioapic_base) return;
    
    /* Get number of redirection entries */
    u32 version = ioapic_read(IOAPIC_REG_VER);
    u8 max_redirections = ((version >> 16) & 0xFF) + 1;
    
    /* Mask all interrupts */
    for (u8 i = 0; i < max_redirections; i++) {
        ioapic_write(IOAPIC_REG_REDTBL + i * 2, 0x10000); /* Masked */
        ioapic_write(IOAPIC_REG_REDTBL + i * 2 + 1, 0);
    }
}

void ioapic_set_irq(u8 irq, u8 vector, u8 dest_apic_id) {
    if (!ioapic_base) return;
    
    u32 low = vector;
    u32 high = (u32)dest_apic_id << 24;
    
    ioapic_write(IOAPIC_REG_REDTBL + irq * 2, low);
    ioapic_write(IOAPIC_REG_REDTBL + irq * 2 + 1, high);
}

void ioapic_mask_irq(u8 irq) {
    if (!ioapic_base) return;
    
    u32 low = ioapic_read(IOAPIC_REG_REDTBL + irq * 2);
    low |= 0x10000; /* Set mask bit */
    ioapic_write(IOAPIC_REG_REDTBL + irq * 2, low);
}

void ioapic_unmask_irq(u8 irq) {
    if (!ioapic_base) return;
    
    u32 low = ioapic_read(IOAPIC_REG_REDTBL + irq * 2);
    low &= ~0x10000; /* Clear mask bit */
    ioapic_write(IOAPIC_REG_REDTBL + irq * 2, low);
}

u8 apic_is_enabled(void) {
    return apic_enabled;
}

u32 apic_get_error_status(void) {
    if (!apic_enabled) return 0;
    
    lapic_write(LAPIC_ESR, 0);
    return lapic_read(LAPIC_ESR);
}

void apic_clear_errors(void) {
    if (!apic_enabled) return;
    
    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0);
}

u32 apic_get_version(void) {
    if (!apic_enabled) return 0;
    return lapic_read(LAPIC_VERSION);
}
