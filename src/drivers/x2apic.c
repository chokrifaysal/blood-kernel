/*
 * x2apic.c – x86 x2APIC advanced interrupt controller
 */

#include "kernel/types.h"

/* x2APIC MSRs */
#define MSR_X2APIC_APICID       0x802
#define MSR_X2APIC_VERSION      0x803
#define MSR_X2APIC_TPR          0x808
#define MSR_X2APIC_APR          0x809
#define MSR_X2APIC_PPR          0x80A
#define MSR_X2APIC_EOI          0x80B
#define MSR_X2APIC_RRD          0x80C
#define MSR_X2APIC_LDR          0x80D
#define MSR_X2APIC_DFR          0x80E
#define MSR_X2APIC_SIVR         0x80F
#define MSR_X2APIC_ISR0         0x810
#define MSR_X2APIC_ISR1         0x811
#define MSR_X2APIC_ISR2         0x812
#define MSR_X2APIC_ISR3         0x813
#define MSR_X2APIC_ISR4         0x814
#define MSR_X2APIC_ISR5         0x815
#define MSR_X2APIC_ISR6         0x816
#define MSR_X2APIC_ISR7         0x817
#define MSR_X2APIC_TMR0         0x818
#define MSR_X2APIC_TMR1         0x819
#define MSR_X2APIC_TMR2         0x81A
#define MSR_X2APIC_TMR3         0x81B
#define MSR_X2APIC_TMR4         0x81C
#define MSR_X2APIC_TMR5         0x81D
#define MSR_X2APIC_TMR6         0x81E
#define MSR_X2APIC_TMR7         0x81F
#define MSR_X2APIC_IRR0         0x820
#define MSR_X2APIC_IRR1         0x821
#define MSR_X2APIC_IRR2         0x822
#define MSR_X2APIC_IRR3         0x823
#define MSR_X2APIC_IRR4         0x824
#define MSR_X2APIC_IRR5         0x825
#define MSR_X2APIC_IRR6         0x826
#define MSR_X2APIC_IRR7         0x827
#define MSR_X2APIC_ESR          0x828
#define MSR_X2APIC_LVT_CMCI     0x82F
#define MSR_X2APIC_ICR          0x830
#define MSR_X2APIC_LVT_TIMER    0x832
#define MSR_X2APIC_LVT_THERMAL  0x833
#define MSR_X2APIC_LVT_PMI      0x834
#define MSR_X2APIC_LVT_LINT0    0x835
#define MSR_X2APIC_LVT_LINT1    0x836
#define MSR_X2APIC_LVT_ERROR    0x837
#define MSR_X2APIC_INIT_COUNT   0x838
#define MSR_X2APIC_CUR_COUNT    0x839
#define MSR_X2APIC_DIV_CONF     0x83E
#define MSR_X2APIC_SELF_IPI     0x83F

/* APIC base MSR */
#define MSR_APIC_BASE           0x1B
#define APIC_BASE_BSP           0x100
#define APIC_BASE_X2APIC        0x400
#define APIC_BASE_GLOBAL_ENABLE 0x800

/* Spurious interrupt vector register bits */
#define SIVR_APIC_ENABLE        0x100
#define SIVR_FOCUS_DISABLE      0x200

/* LVT bits */
#define LVT_MASKED              0x10000
#define LVT_TRIGGER_LEVEL       0x8000
#define LVT_REMOTE_IRR          0x4000
#define LVT_PIN_POLARITY        0x2000
#define LVT_DELIVERY_STATUS     0x1000
#define LVT_DELIVERY_MODE_MASK  0x700
#define LVT_DELIVERY_FIXED      0x000
#define LVT_DELIVERY_SMI        0x200
#define LVT_DELIVERY_NMI        0x400
#define LVT_DELIVERY_INIT       0x500
#define LVT_DELIVERY_EXTINT     0x700

/* ICR bits */
#define ICR_DELIVERY_MODE_MASK  0x700
#define ICR_DELIVERY_FIXED      0x000
#define ICR_DELIVERY_LOWEST     0x100
#define ICR_DELIVERY_SMI        0x200
#define ICR_DELIVERY_NMI        0x400
#define ICR_DELIVERY_INIT       0x500
#define ICR_DELIVERY_STARTUP    0x600
#define ICR_DEST_MODE_PHYSICAL  0x000
#define ICR_DEST_MODE_LOGICAL   0x800
#define ICR_DELIVERY_STATUS     0x1000
#define ICR_LEVEL_ASSERT        0x4000
#define ICR_TRIGGER_LEVEL       0x8000
#define ICR_DEST_SHORTHAND_MASK 0xC0000
#define ICR_DEST_NO_SHORTHAND   0x00000
#define ICR_DEST_SELF           0x40000
#define ICR_DEST_ALL_INCLUDING  0x80000
#define ICR_DEST_ALL_EXCLUDING  0xC0000

typedef struct {
    u8 x2apic_supported;
    u8 x2apic_enabled;
    u32 apic_id;
    u32 version;
    u32 max_lvt_entries;
    u8 eoi_broadcast_suppression;
    u32 timer_frequency;
    u8 timer_running;
} x2apic_info_t;

static x2apic_info_t x2apic_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 x2apic_check_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (ecx & (1 << 21)) != 0;
}

static void x2apic_enable(void) {
    u64 apic_base = msr_read(MSR_APIC_BASE);
    
    /* Enable x2APIC mode */
    apic_base |= APIC_BASE_X2APIC | APIC_BASE_GLOBAL_ENABLE;
    msr_write(MSR_APIC_BASE, apic_base);
    
    x2apic_info.x2apic_enabled = 1;
}

static void x2apic_read_info(void) {
    /* Read APIC ID */
    x2apic_info.apic_id = (u32)msr_read(MSR_X2APIC_APICID);
    
    /* Read version information */
    u32 version = (u32)msr_read(MSR_X2APIC_VERSION);
    x2apic_info.version = version & 0xFF;
    x2apic_info.max_lvt_entries = ((version >> 16) & 0xFF) + 1;
    x2apic_info.eoi_broadcast_suppression = (version & (1 << 24)) != 0;
}

void x2apic_init(void) {
    if (!msr_is_supported()) return;
    
    x2apic_info.x2apic_supported = x2apic_check_support();
    if (!x2apic_info.x2apic_supported) return;
    
    x2apic_enable();
    x2apic_read_info();
    
    /* Configure spurious interrupt vector */
    msr_write(MSR_X2APIC_SIVR, 0xFF | SIVR_APIC_ENABLE);
    
    /* Mask all LVT entries initially */
    msr_write(MSR_X2APIC_LVT_TIMER, LVT_MASKED);
    msr_write(MSR_X2APIC_LVT_THERMAL, LVT_MASKED);
    msr_write(MSR_X2APIC_LVT_PMI, LVT_MASKED);
    msr_write(MSR_X2APIC_LVT_LINT0, LVT_MASKED);
    msr_write(MSR_X2APIC_LVT_LINT1, LVT_MASKED);
    msr_write(MSR_X2APIC_LVT_ERROR, LVT_MASKED);
    
    if (x2apic_info.max_lvt_entries > 6) {
        msr_write(MSR_X2APIC_LVT_CMCI, LVT_MASKED);
    }
    
    /* Clear error status */
    msr_write(MSR_X2APIC_ESR, 0);
    msr_write(MSR_X2APIC_ESR, 0);
    
    x2apic_info.timer_frequency = 0;
    x2apic_info.timer_running = 0;
}

u8 x2apic_is_supported(void) {
    return x2apic_info.x2apic_supported;
}

u8 x2apic_is_enabled(void) {
    return x2apic_info.x2apic_enabled;
}

u32 x2apic_get_id(void) {
    if (!x2apic_info.x2apic_enabled) return 0;
    
    return x2apic_info.apic_id;
}

u32 x2apic_get_version(void) {
    return x2apic_info.version;
}

u32 x2apic_get_max_lvt_entries(void) {
    return x2apic_info.max_lvt_entries;
}

void x2apic_send_eoi(void) {
    if (!x2apic_info.x2apic_enabled) return;
    
    msr_write(MSR_X2APIC_EOI, 0);
}

void x2apic_send_ipi(u32 dest_apic_id, u32 vector, u32 delivery_mode) {
    if (!x2apic_info.x2apic_enabled) return;
    
    u64 icr = vector | delivery_mode | ICR_DEST_MODE_PHYSICAL;
    icr |= ((u64)dest_apic_id << 32);
    
    msr_write(MSR_X2APIC_ICR, icr);
}

void x2apic_send_ipi_all_excluding_self(u32 vector, u32 delivery_mode) {
    if (!x2apic_info.x2apic_enabled) return;
    
    u64 icr = vector | delivery_mode | ICR_DEST_ALL_EXCLUDING;
    
    msr_write(MSR_X2APIC_ICR, icr);
}

void x2apic_send_ipi_self(u32 vector) {
    if (!x2apic_info.x2apic_enabled) return;
    
    msr_write(MSR_X2APIC_SELF_IPI, vector);
}

void x2apic_setup_timer(u32 vector, u32 initial_count, u8 periodic) {
    if (!x2apic_info.x2apic_enabled) return;
    
    /* Stop timer */
    msr_write(MSR_X2APIC_LVT_TIMER, LVT_MASKED);
    
    /* Set divide configuration (divide by 1) */
    msr_write(MSR_X2APIC_DIV_CONF, 0x0B);
    
    /* Set initial count */
    msr_write(MSR_X2APIC_INIT_COUNT, initial_count);
    
    /* Configure timer LVT */
    u32 lvt_timer = vector;
    if (periodic) {
        lvt_timer |= 0x20000; /* Periodic mode */
    }
    
    msr_write(MSR_X2APIC_LVT_TIMER, lvt_timer);
    
    x2apic_info.timer_running = 1;
}

void x2apic_stop_timer(void) {
    if (!x2apic_info.x2apic_enabled) return;
    
    msr_write(MSR_X2APIC_LVT_TIMER, LVT_MASKED);
    msr_write(MSR_X2APIC_INIT_COUNT, 0);
    
    x2apic_info.timer_running = 0;
}

u32 x2apic_get_timer_count(void) {
    if (!x2apic_info.x2apic_enabled) return 0;
    
    return (u32)msr_read(MSR_X2APIC_CUR_COUNT);
}

void x2apic_calibrate_timer(void) {
    if (!x2apic_info.x2apic_enabled) return;
    
    /* Use PIT to calibrate x2APIC timer */
    extern void timer_delay(u32 ms);
    
    /* Setup timer for calibration */
    msr_write(MSR_X2APIC_LVT_TIMER, LVT_MASKED);
    msr_write(MSR_X2APIC_DIV_CONF, 0x0B); /* Divide by 1 */
    msr_write(MSR_X2APIC_INIT_COUNT, 0xFFFFFFFF);
    
    /* Wait 10ms */
    timer_delay(10);
    
    /* Read current count */
    u32 current_count = (u32)msr_read(MSR_X2APIC_CUR_COUNT);
    u32 ticks_per_10ms = 0xFFFFFFFF - current_count;
    
    /* Calculate frequency */
    x2apic_info.timer_frequency = ticks_per_10ms * 100;
    
    /* Stop timer */
    msr_write(MSR_X2APIC_INIT_COUNT, 0);
}

u32 x2apic_get_timer_frequency(void) {
    return x2apic_info.timer_frequency;
}

void x2apic_setup_lint(u8 lint_num, u32 vector, u32 delivery_mode, u8 trigger_mode, u8 polarity) {
    if (!x2apic_info.x2apic_enabled || lint_num > 1) return;
    
    u32 lvt = vector | delivery_mode;
    
    if (trigger_mode) {
        lvt |= LVT_TRIGGER_LEVEL;
    }
    
    if (polarity) {
        lvt |= LVT_PIN_POLARITY;
    }
    
    u32 msr_addr = (lint_num == 0) ? MSR_X2APIC_LVT_LINT0 : MSR_X2APIC_LVT_LINT1;
    msr_write(msr_addr, lvt);
}

void x2apic_mask_lint(u8 lint_num) {
    if (!x2apic_info.x2apic_enabled || lint_num > 1) return;
    
    u32 msr_addr = (lint_num == 0) ? MSR_X2APIC_LVT_LINT0 : MSR_X2APIC_LVT_LINT1;
    u32 lvt = (u32)msr_read(msr_addr);
    lvt |= LVT_MASKED;
    msr_write(msr_addr, lvt);
}

void x2apic_unmask_lint(u8 lint_num) {
    if (!x2apic_info.x2apic_enabled || lint_num > 1) return;
    
    u32 msr_addr = (lint_num == 0) ? MSR_X2APIC_LVT_LINT0 : MSR_X2APIC_LVT_LINT1;
    u32 lvt = (u32)msr_read(msr_addr);
    lvt &= ~LVT_MASKED;
    msr_write(msr_addr, lvt);
}

u32 x2apic_get_error_status(void) {
    if (!x2apic_info.x2apic_enabled) return 0;
    
    return (u32)msr_read(MSR_X2APIC_ESR);
}

void x2apic_clear_error_status(void) {
    if (!x2apic_info.x2apic_enabled) return;
    
    msr_write(MSR_X2APIC_ESR, 0);
    msr_write(MSR_X2APIC_ESR, 0);
}

u8 x2apic_is_timer_running(void) {
    return x2apic_info.timer_running;
}

u8 x2apic_supports_eoi_broadcast_suppression(void) {
    return x2apic_info.eoi_broadcast_suppression;
}
