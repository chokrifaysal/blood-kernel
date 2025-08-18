/*
 * apic_ext.c – x86 advanced interrupt handling (APIC extensions/IPI)
 */

#include "kernel/types.h"

/* APIC base MSR */
#define MSR_APIC_BASE                   0x1B
#define APIC_BASE_ENABLE                (1 << 11)
#define APIC_BASE_X2APIC                (1 << 10)

/* APIC registers */
#define APIC_ID                         0x20
#define APIC_VERSION                    0x30
#define APIC_TPR                        0x80
#define APIC_APR                        0x90
#define APIC_PPR                        0xA0
#define APIC_EOI                        0xB0
#define APIC_RRD                        0xC0
#define APIC_LDR                        0xD0
#define APIC_DFR                        0xE0
#define APIC_SPURIOUS                   0xF0
#define APIC_ISR                        0x100
#define APIC_TMR                        0x180
#define APIC_IRR                        0x200
#define APIC_ESR                        0x280
#define APIC_ICR_LOW                    0x300
#define APIC_ICR_HIGH                   0x310
#define APIC_LVT_TIMER                  0x320
#define APIC_LVT_THERMAL                0x330
#define APIC_LVT_PERFMON                0x340
#define APIC_LVT_LINT0                  0x350
#define APIC_LVT_LINT1                  0x360
#define APIC_LVT_ERROR                  0x370
#define APIC_TIMER_ICR                  0x380
#define APIC_TIMER_CCR                  0x390
#define APIC_TIMER_DCR                  0x3E0

/* ICR delivery modes */
#define ICR_DELIVERY_FIXED              0x0
#define ICR_DELIVERY_LOWEST             0x1
#define ICR_DELIVERY_SMI                0x2
#define ICR_DELIVERY_NMI                0x4
#define ICR_DELIVERY_INIT               0x5
#define ICR_DELIVERY_STARTUP            0x6

/* ICR destination modes */
#define ICR_DEST_PHYSICAL               0x0
#define ICR_DEST_LOGICAL                0x1

/* ICR destination shorthand */
#define ICR_DEST_NO_SHORTHAND           0x0
#define ICR_DEST_SELF                   0x1
#define ICR_DEST_ALL_INCLUDING_SELF     0x2
#define ICR_DEST_ALL_EXCLUDING_SELF     0x3

/* Timer modes */
#define APIC_TIMER_ONESHOT              0x0
#define APIC_TIMER_PERIODIC             0x1
#define APIC_TIMER_TSC_DEADLINE         0x2

typedef struct {
    u32 vector;
    u32 delivery_mode;
    u32 dest_mode;
    u32 dest_shorthand;
    u32 destination;
    u64 timestamp;
    u8 delivered;
} ipi_record_t;

typedef struct {
    u8 apic_ext_supported;
    u8 x2apic_supported;
    u8 x2apic_enabled;
    u8 apic_enabled;
    u32 apic_base_addr;
    u32 apic_id;
    u32 apic_version;
    u8 max_lvt_entries;
    u32 spurious_vector;
    u32 timer_frequency;
    u8 timer_mode;
    u32 timer_initial_count;
    u32 ipi_count;
    u32 broadcast_count;
    u32 nmi_count;
    u32 smi_count;
    u32 init_count;
    u32 startup_count;
    ipi_record_t ipi_history[64];
    u32 ipi_history_index;
    u32 error_count;
    u32 last_error;
    u64 last_ipi_time;
} apic_ext_info_t;

static apic_ext_info_t apic_ext_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u32 inl(u16 port);
extern void outl(u16 port, u32 value);
extern u64 timer_get_ticks(void);

static u32 apic_ext_read_reg(u32 reg) {
    if (apic_ext_info.x2apic_enabled) {
        /* x2APIC uses MSRs */
        return (u32)msr_read(0x800 + (reg >> 4));
    } else {
        /* xAPIC uses memory-mapped I/O */
        volatile u32* apic_base = (volatile u32*)apic_ext_info.apic_base_addr;
        return apic_base[reg >> 2];
    }
}

static void apic_ext_write_reg(u32 reg, u32 value) {
    if (apic_ext_info.x2apic_enabled) {
        /* x2APIC uses MSRs */
        msr_write(0x800 + (reg >> 4), value);
    } else {
        /* xAPIC uses memory-mapped I/O */
        volatile u32* apic_base = (volatile u32*)apic_ext_info.apic_base_addr;
        apic_base[reg >> 2] = value;
    }
}

static void apic_ext_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for APIC support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    apic_ext_info.apic_ext_supported = (edx & (1 << 9)) != 0;
    
    /* Check for x2APIC support */
    apic_ext_info.x2apic_supported = (ecx & (1 << 21)) != 0;
    
    if (apic_ext_info.apic_ext_supported && msr_is_supported()) {
        /* Read APIC base */
        u64 apic_base_msr = msr_read(MSR_APIC_BASE);
        apic_ext_info.apic_base_addr = apic_base_msr & 0xFFFFF000;
        apic_ext_info.apic_enabled = (apic_base_msr & APIC_BASE_ENABLE) != 0;
        apic_ext_info.x2apic_enabled = (apic_base_msr & APIC_BASE_X2APIC) != 0;
    }
}

void apic_ext_init(void) {
    apic_ext_info.apic_ext_supported = 0;
    apic_ext_info.x2apic_supported = 0;
    apic_ext_info.x2apic_enabled = 0;
    apic_ext_info.apic_enabled = 0;
    apic_ext_info.apic_base_addr = 0xFEE00000;
    apic_ext_info.apic_id = 0;
    apic_ext_info.apic_version = 0;
    apic_ext_info.max_lvt_entries = 0;
    apic_ext_info.spurious_vector = 0xFF;
    apic_ext_info.timer_frequency = 0;
    apic_ext_info.timer_mode = APIC_TIMER_ONESHOT;
    apic_ext_info.timer_initial_count = 0;
    apic_ext_info.ipi_count = 0;
    apic_ext_info.broadcast_count = 0;
    apic_ext_info.nmi_count = 0;
    apic_ext_info.smi_count = 0;
    apic_ext_info.init_count = 0;
    apic_ext_info.startup_count = 0;
    apic_ext_info.ipi_history_index = 0;
    apic_ext_info.error_count = 0;
    apic_ext_info.last_error = 0;
    apic_ext_info.last_ipi_time = 0;
    
    for (u32 i = 0; i < 64; i++) {
        apic_ext_info.ipi_history[i].vector = 0;
        apic_ext_info.ipi_history[i].delivery_mode = 0;
        apic_ext_info.ipi_history[i].dest_mode = 0;
        apic_ext_info.ipi_history[i].dest_shorthand = 0;
        apic_ext_info.ipi_history[i].destination = 0;
        apic_ext_info.ipi_history[i].timestamp = 0;
        apic_ext_info.ipi_history[i].delivered = 0;
    }
    
    apic_ext_detect_capabilities();
    
    if (apic_ext_info.apic_ext_supported) {
        /* Read APIC ID and version */
        apic_ext_info.apic_id = apic_ext_read_reg(APIC_ID);
        if (!apic_ext_info.x2apic_enabled) {
            apic_ext_info.apic_id >>= 24;
        }
        
        u32 version = apic_ext_read_reg(APIC_VERSION);
        apic_ext_info.apic_version = version & 0xFF;
        apic_ext_info.max_lvt_entries = ((version >> 16) & 0xFF) + 1;
        
        /* Enable APIC if not already enabled */
        if (!apic_ext_info.apic_enabled) {
            u64 apic_base_msr = msr_read(MSR_APIC_BASE);
            apic_base_msr |= APIC_BASE_ENABLE;
            msr_write(MSR_APIC_BASE, apic_base_msr);
            apic_ext_info.apic_enabled = 1;
        }
        
        /* Set spurious interrupt vector */
        u32 spurious = apic_ext_read_reg(APIC_SPURIOUS);
        spurious |= (1 << 8); /* Enable APIC */
        spurious |= apic_ext_info.spurious_vector;
        apic_ext_write_reg(APIC_SPURIOUS, spurious);
    }
}

u8 apic_ext_is_supported(void) {
    return apic_ext_info.apic_ext_supported;
}

u8 apic_ext_is_x2apic_supported(void) {
    return apic_ext_info.x2apic_supported;
}

u8 apic_ext_is_x2apic_enabled(void) {
    return apic_ext_info.x2apic_enabled;
}

u8 apic_ext_enable_x2apic(void) {
    if (!apic_ext_info.x2apic_supported || !msr_is_supported()) return 0;
    
    u64 apic_base_msr = msr_read(MSR_APIC_BASE);
    apic_base_msr |= APIC_BASE_X2APIC | APIC_BASE_ENABLE;
    msr_write(MSR_APIC_BASE, apic_base_msr);
    
    apic_ext_info.x2apic_enabled = 1;
    apic_ext_info.apic_enabled = 1;
    
    /* Re-read APIC ID in x2APIC mode */
    apic_ext_info.apic_id = apic_ext_read_reg(APIC_ID);
    
    return 1;
}

u32 apic_ext_get_id(void) {
    return apic_ext_info.apic_id;
}

u32 apic_ext_get_version(void) {
    return apic_ext_info.apic_version;
}

u8 apic_ext_get_max_lvt_entries(void) {
    return apic_ext_info.max_lvt_entries;
}

void apic_ext_send_ipi(u32 dest_apic_id, u32 vector, u32 delivery_mode) {
    if (!apic_ext_info.apic_enabled) return;
    
    /* Record IPI in history */
    ipi_record_t* record = &apic_ext_info.ipi_history[apic_ext_info.ipi_history_index];
    record->vector = vector;
    record->delivery_mode = delivery_mode;
    record->dest_mode = ICR_DEST_PHYSICAL;
    record->dest_shorthand = ICR_DEST_NO_SHORTHAND;
    record->destination = dest_apic_id;
    record->timestamp = timer_get_ticks();
    record->delivered = 0;
    
    apic_ext_info.ipi_history_index = (apic_ext_info.ipi_history_index + 1) % 64;
    apic_ext_info.ipi_count++;
    apic_ext_info.last_ipi_time = record->timestamp;
    
    if (apic_ext_info.x2apic_enabled) {
        /* x2APIC mode - single MSR write */
        u64 icr = ((u64)dest_apic_id << 32) | (delivery_mode << 8) | vector;
        msr_write(0x830, icr); /* ICR MSR in x2APIC */
    } else {
        /* xAPIC mode - two register writes */
        apic_ext_write_reg(APIC_ICR_HIGH, dest_apic_id << 24);
        apic_ext_write_reg(APIC_ICR_LOW, (delivery_mode << 8) | vector);
    }
    
    record->delivered = 1;
}

void apic_ext_send_ipi_broadcast(u32 vector, u32 delivery_mode, u8 include_self) {
    if (!apic_ext_info.apic_enabled) return;
    
    u32 dest_shorthand = include_self ? ICR_DEST_ALL_INCLUDING_SELF : ICR_DEST_ALL_EXCLUDING_SELF;
    
    /* Record broadcast IPI */
    ipi_record_t* record = &apic_ext_info.ipi_history[apic_ext_info.ipi_history_index];
    record->vector = vector;
    record->delivery_mode = delivery_mode;
    record->dest_mode = ICR_DEST_PHYSICAL;
    record->dest_shorthand = dest_shorthand;
    record->destination = 0xFFFFFFFF;
    record->timestamp = timer_get_ticks();
    record->delivered = 0;
    
    apic_ext_info.ipi_history_index = (apic_ext_info.ipi_history_index + 1) % 64;
    apic_ext_info.broadcast_count++;
    apic_ext_info.last_ipi_time = record->timestamp;
    
    if (apic_ext_info.x2apic_enabled) {
        u64 icr = ((u64)dest_shorthand << 18) | (delivery_mode << 8) | vector;
        msr_write(0x830, icr);
    } else {
        apic_ext_write_reg(APIC_ICR_HIGH, 0);
        apic_ext_write_reg(APIC_ICR_LOW, (dest_shorthand << 18) | (delivery_mode << 8) | vector);
    }
    
    record->delivered = 1;
}

void apic_ext_send_nmi(u32 dest_apic_id) {
    apic_ext_send_ipi(dest_apic_id, 0, ICR_DELIVERY_NMI);
    apic_ext_info.nmi_count++;
}

void apic_ext_send_init(u32 dest_apic_id) {
    apic_ext_send_ipi(dest_apic_id, 0, ICR_DELIVERY_INIT);
    apic_ext_info.init_count++;
}

void apic_ext_send_startup(u32 dest_apic_id, u32 start_page) {
    apic_ext_send_ipi(dest_apic_id, start_page, ICR_DELIVERY_STARTUP);
    apic_ext_info.startup_count++;
}

void apic_ext_eoi(void) {
    if (!apic_ext_info.apic_enabled) return;
    
    apic_ext_write_reg(APIC_EOI, 0);
}

u32 apic_ext_get_isr(u8 reg) {
    if (!apic_ext_info.apic_enabled || reg >= 8) return 0;
    
    return apic_ext_read_reg(APIC_ISR + (reg * 0x10));
}

u32 apic_ext_get_irr(u8 reg) {
    if (!apic_ext_info.apic_enabled || reg >= 8) return 0;
    
    return apic_ext_read_reg(APIC_IRR + (reg * 0x10));
}

u32 apic_ext_get_tmr(u8 reg) {
    if (!apic_ext_info.apic_enabled || reg >= 8) return 0;
    
    return apic_ext_read_reg(APIC_TMR + (reg * 0x10));
}

void apic_ext_setup_timer(u32 initial_count, u8 mode, u8 vector) {
    if (!apic_ext_info.apic_enabled) return;
    
    apic_ext_info.timer_initial_count = initial_count;
    apic_ext_info.timer_mode = mode;
    
    /* Set timer divide configuration */
    apic_ext_write_reg(APIC_TIMER_DCR, 0x3); /* Divide by 16 */
    
    /* Set LVT timer entry */
    u32 lvt_timer = vector | (mode << 17);
    apic_ext_write_reg(APIC_LVT_TIMER, lvt_timer);
    
    /* Set initial count */
    apic_ext_write_reg(APIC_TIMER_ICR, initial_count);
}

u32 apic_ext_get_timer_current_count(void) {
    if (!apic_ext_info.apic_enabled) return 0;
    
    return apic_ext_read_reg(APIC_TIMER_CCR);
}

void apic_ext_set_task_priority(u8 priority) {
    if (!apic_ext_info.apic_enabled) return;
    
    apic_ext_write_reg(APIC_TPR, priority << 4);
}

u8 apic_ext_get_task_priority(void) {
    if (!apic_ext_info.apic_enabled) return 0;
    
    return apic_ext_read_reg(APIC_TPR) >> 4;
}

u32 apic_ext_get_error_status(void) {
    if (!apic_ext_info.apic_enabled) return 0;
    
    /* Write to ESR to update it */
    apic_ext_write_reg(APIC_ESR, 0);
    u32 error = apic_ext_read_reg(APIC_ESR);
    
    if (error != 0) {
        apic_ext_info.error_count++;
        apic_ext_info.last_error = error;
    }
    
    return error;
}

u32 apic_ext_get_ipi_count(void) {
    return apic_ext_info.ipi_count;
}

u32 apic_ext_get_broadcast_count(void) {
    return apic_ext_info.broadcast_count;
}

u32 apic_ext_get_nmi_count(void) {
    return apic_ext_info.nmi_count;
}

u32 apic_ext_get_init_count(void) {
    return apic_ext_info.init_count;
}

u32 apic_ext_get_startup_count(void) {
    return apic_ext_info.startup_count;
}

const ipi_record_t* apic_ext_get_ipi_history(u32 index) {
    if (index >= 64) return 0;
    return &apic_ext_info.ipi_history[index];
}

u32 apic_ext_get_error_count(void) {
    return apic_ext_info.error_count;
}

u32 apic_ext_get_last_error(void) {
    return apic_ext_info.last_error;
}

u64 apic_ext_get_last_ipi_time(void) {
    return apic_ext_info.last_ipi_time;
}

void apic_ext_clear_statistics(void) {
    apic_ext_info.ipi_count = 0;
    apic_ext_info.broadcast_count = 0;
    apic_ext_info.nmi_count = 0;
    apic_ext_info.init_count = 0;
    apic_ext_info.startup_count = 0;
    apic_ext_info.error_count = 0;
    apic_ext_info.last_error = 0;
    apic_ext_info.ipi_history_index = 0;
    
    for (u32 i = 0; i < 64; i++) {
        apic_ext_info.ipi_history[i].vector = 0;
        apic_ext_info.ipi_history[i].delivered = 0;
    }
}
