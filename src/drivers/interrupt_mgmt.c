/*
 * interrupt_mgmt.c – x86 advanced interrupt handling (NMI/SMI/MCE)
 */

#include "kernel/types.h"

/* NMI control ports */
#define NMI_ENABLE_PORT         0x70
#define NMI_DISABLE_BIT         0x80

/* SMI control MSRs */
#define MSR_SMI_COUNT           0x34
#define MSR_SMM_BASE            0x9E
#define MSR_SMM_MASK            0x9F

/* Machine Check MSRs */
#define MSR_MCG_CAP             0x179
#define MSR_MCG_STATUS          0x17A
#define MSR_MCG_CTL             0x17B
#define MSR_MC0_CTL             0x400
#define MSR_MC0_STATUS          0x401
#define MSR_MC0_ADDR            0x402
#define MSR_MC0_MISC            0x403

/* Interrupt types */
#define INT_TYPE_EXTERNAL       0
#define INT_TYPE_NMI            1
#define INT_TYPE_SMI            2
#define INT_TYPE_MCE            3
#define INT_TYPE_THERMAL        4

/* MCE status bits */
#define MCE_STATUS_VAL          0x8000000000000000ULL
#define MCE_STATUS_OVER         0x4000000000000000ULL
#define MCE_STATUS_UC           0x2000000000000000ULL
#define MCE_STATUS_EN           0x1000000000000000ULL
#define MCE_STATUS_MISCV        0x0800000000000000ULL
#define MCE_STATUS_ADDRV        0x0400000000000000ULL

typedef struct {
    u64 status;
    u64 addr;
    u64 misc;
    u32 bank;
    u8 valid;
} mce_record_t;

typedef struct {
    u8 nmi_supported;
    u8 smi_supported;
    u8 mce_supported;
    u8 thermal_interrupt_supported;
    u8 nmi_enabled;
    u8 smi_enabled;
    u8 mce_enabled;
    u32 mce_bank_count;
    u32 nmi_count;
    u32 smi_count;
    u32 mce_count;
    mce_record_t mce_records[32];
    u8 interrupt_nesting_level;
    u32 last_interrupt_vector;
} interrupt_mgmt_info_t;

static interrupt_mgmt_info_t interrupt_mgmt_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern void outb(u16 port, u8 value);
extern u8 inb(u16 port);

static void interrupt_mgmt_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for machine check support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    interrupt_mgmt_info.mce_supported = (edx & (1 << 14)) != 0;
    interrupt_mgmt_info.thermal_interrupt_supported = (edx & (1 << 22)) != 0;
    
    /* NMI is always supported on x86 */
    interrupt_mgmt_info.nmi_supported = 1;
    
    /* SMI support detection (simplified) */
    interrupt_mgmt_info.smi_supported = 1;
    
    if (interrupt_mgmt_info.mce_supported && msr_is_supported()) {
        /* Read MCE capabilities */
        u64 mcg_cap = msr_read(MSR_MCG_CAP);
        interrupt_mgmt_info.mce_bank_count = mcg_cap & 0xFF;
        
        if (interrupt_mgmt_info.mce_bank_count > 32) {
            interrupt_mgmt_info.mce_bank_count = 32;
        }
    }
}

static void interrupt_mgmt_setup_mce(void) {
    if (!interrupt_mgmt_info.mce_supported || !msr_is_supported()) return;
    
    /* Enable machine check exceptions */
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 6); /* Set MCE bit */
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    /* Initialize MCE banks */
    for (u32 i = 0; i < interrupt_mgmt_info.mce_bank_count; i++) {
        /* Enable all error reporting */
        msr_write(MSR_MC0_CTL + i * 4, 0xFFFFFFFFFFFFFFFFULL);
        
        /* Clear status registers */
        msr_write(MSR_MC0_STATUS + i * 4, 0);
    }
    
    /* Enable global MCE */
    msr_write(MSR_MCG_CTL, 0xFFFFFFFFFFFFFFFFULL);
    
    interrupt_mgmt_info.mce_enabled = 1;
}

static void interrupt_mgmt_setup_nmi(void) {
    /* Enable NMI by clearing the disable bit */
    u8 value = inb(NMI_ENABLE_PORT);
    value &= ~NMI_DISABLE_BIT;
    outb(NMI_ENABLE_PORT, value);
    
    interrupt_mgmt_info.nmi_enabled = 1;
}

void interrupt_mgmt_init(void) {
    interrupt_mgmt_info.nmi_supported = 0;
    interrupt_mgmt_info.smi_supported = 0;
    interrupt_mgmt_info.mce_supported = 0;
    interrupt_mgmt_info.thermal_interrupt_supported = 0;
    interrupt_mgmt_info.nmi_enabled = 0;
    interrupt_mgmt_info.smi_enabled = 0;
    interrupt_mgmt_info.mce_enabled = 0;
    interrupt_mgmt_info.mce_bank_count = 0;
    interrupt_mgmt_info.nmi_count = 0;
    interrupt_mgmt_info.smi_count = 0;
    interrupt_mgmt_info.mce_count = 0;
    interrupt_mgmt_info.interrupt_nesting_level = 0;
    interrupt_mgmt_info.last_interrupt_vector = 0;
    
    interrupt_mgmt_detect_capabilities();
    interrupt_mgmt_setup_mce();
    interrupt_mgmt_setup_nmi();
}

u8 interrupt_mgmt_is_nmi_supported(void) {
    return interrupt_mgmt_info.nmi_supported;
}

u8 interrupt_mgmt_is_smi_supported(void) {
    return interrupt_mgmt_info.smi_supported;
}

u8 interrupt_mgmt_is_mce_supported(void) {
    return interrupt_mgmt_info.mce_supported;
}

u8 interrupt_mgmt_is_thermal_interrupt_supported(void) {
    return interrupt_mgmt_info.thermal_interrupt_supported;
}

void interrupt_mgmt_enable_nmi(void) {
    if (!interrupt_mgmt_info.nmi_supported) return;
    
    u8 value = inb(NMI_ENABLE_PORT);
    value &= ~NMI_DISABLE_BIT;
    outb(NMI_ENABLE_PORT, value);
    
    interrupt_mgmt_info.nmi_enabled = 1;
}

void interrupt_mgmt_disable_nmi(void) {
    if (!interrupt_mgmt_info.nmi_supported) return;
    
    u8 value = inb(NMI_ENABLE_PORT);
    value |= NMI_DISABLE_BIT;
    outb(NMI_ENABLE_PORT, value);
    
    interrupt_mgmt_info.nmi_enabled = 0;
}

u8 interrupt_mgmt_is_nmi_enabled(void) {
    return interrupt_mgmt_info.nmi_enabled;
}

void interrupt_mgmt_trigger_nmi(void) {
    if (!interrupt_mgmt_info.nmi_supported) return;
    
    /* Trigger NMI via APIC */
    extern void apic_send_nmi(u8 dest);
    apic_send_nmi(0xFF); /* Broadcast NMI */
}

u32 interrupt_mgmt_get_nmi_count(void) {
    return interrupt_mgmt_info.nmi_count;
}

u32 interrupt_mgmt_get_smi_count(void) {
    if (!interrupt_mgmt_info.smi_supported || !msr_is_supported()) return 0;
    
    return (u32)msr_read(MSR_SMI_COUNT);
}

u32 interrupt_mgmt_get_mce_count(void) {
    return interrupt_mgmt_info.mce_count;
}

u32 interrupt_mgmt_get_mce_bank_count(void) {
    return interrupt_mgmt_info.mce_bank_count;
}

void interrupt_mgmt_handle_nmi(void) {
    interrupt_mgmt_info.nmi_count++;
    interrupt_mgmt_info.last_interrupt_vector = 2; /* NMI vector */
    
    /* Check for machine check */
    if (interrupt_mgmt_info.mce_supported && msr_is_supported()) {
        u64 mcg_status = msr_read(MSR_MCG_STATUS);
        if (mcg_status & (1ULL << 2)) { /* MCIP bit */
            interrupt_mgmt_handle_mce();
            return;
        }
    }
    
    /* Handle other NMI sources */
    /* Memory parity error, I/O channel check, etc. */
}

void interrupt_mgmt_handle_mce(void) {
    if (!interrupt_mgmt_info.mce_enabled || !msr_is_supported()) return;
    
    interrupt_mgmt_info.mce_count++;
    
    /* Read global MCE status */
    u64 mcg_status = msr_read(MSR_MCG_STATUS);
    
    /* Scan all MCE banks */
    for (u32 i = 0; i < interrupt_mgmt_info.mce_bank_count; i++) {
        u64 status = msr_read(MSR_MC0_STATUS + i * 4);
        
        if (status & MCE_STATUS_VAL) {
            /* Record MCE information */
            if (interrupt_mgmt_info.mce_count <= 32) {
                mce_record_t* record = &interrupt_mgmt_info.mce_records[interrupt_mgmt_info.mce_count - 1];
                
                record->bank = i;
                record->status = status;
                record->valid = 1;
                
                if (status & MCE_STATUS_ADDRV) {
                    record->addr = msr_read(MSR_MC0_ADDR + i * 4);
                }
                
                if (status & MCE_STATUS_MISCV) {
                    record->misc = msr_read(MSR_MC0_MISC + i * 4);
                }
            }
            
            /* Clear the status register */
            msr_write(MSR_MC0_STATUS + i * 4, 0);
            
            /* Check if this is an uncorrectable error */
            if (status & MCE_STATUS_UC) {
                /* Uncorrectable error - system may need to halt */
                break;
            }
        }
    }
    
    /* Clear global MCE status */
    msr_write(MSR_MCG_STATUS, 0);
}

const mce_record_t* interrupt_mgmt_get_mce_record(u32 index) {
    if (index >= interrupt_mgmt_info.mce_count || index >= 32) return 0;
    
    return &interrupt_mgmt_info.mce_records[index];
}

void interrupt_mgmt_clear_mce_records(void) {
    interrupt_mgmt_info.mce_count = 0;
    
    for (u32 i = 0; i < 32; i++) {
        interrupt_mgmt_info.mce_records[i].valid = 0;
    }
}

void interrupt_mgmt_enter_interrupt(u32 vector) {
    interrupt_mgmt_info.interrupt_nesting_level++;
    interrupt_mgmt_info.last_interrupt_vector = vector;
}

void interrupt_mgmt_exit_interrupt(void) {
    if (interrupt_mgmt_info.interrupt_nesting_level > 0) {
        interrupt_mgmt_info.interrupt_nesting_level--;
    }
}

u8 interrupt_mgmt_get_nesting_level(void) {
    return interrupt_mgmt_info.interrupt_nesting_level;
}

u32 interrupt_mgmt_get_last_vector(void) {
    return interrupt_mgmt_info.last_interrupt_vector;
}

u8 interrupt_mgmt_in_interrupt(void) {
    return interrupt_mgmt_info.interrupt_nesting_level > 0;
}

void interrupt_mgmt_mask_all_interrupts(void) {
    __asm__ volatile("cli");
}

void interrupt_mgmt_unmask_all_interrupts(void) {
    __asm__ volatile("sti");
}

u8 interrupt_mgmt_are_interrupts_enabled(void) {
    u32 flags;
    __asm__ volatile("pushf; pop %0" : "=r"(flags));
    return (flags & (1 << 9)) != 0; /* IF flag */
}

void interrupt_mgmt_save_and_disable_interrupts(u32* flags) {
    __asm__ volatile("pushf; pop %0; cli" : "=r"(*flags));
}

void interrupt_mgmt_restore_interrupts(u32 flags) {
    __asm__ volatile("push %0; popf" : : "r"(flags));
}

void interrupt_mgmt_setup_thermal_interrupt(void) {
    if (!interrupt_mgmt_info.thermal_interrupt_supported) return;
    
    extern void thermal_setup_interrupt(void);
    thermal_setup_interrupt();
}

void interrupt_mgmt_handle_thermal_interrupt(void) {
    if (!interrupt_mgmt_info.thermal_interrupt_supported) return;
    
    extern void thermal_handle_interrupt(void);
    thermal_handle_interrupt();
}

u8 interrupt_mgmt_is_mce_bank_valid(u32 bank) {
    return bank < interrupt_mgmt_info.mce_bank_count;
}

u64 interrupt_mgmt_read_mce_bank_status(u32 bank) {
    if (!interrupt_mgmt_is_mce_bank_valid(bank) || !msr_is_supported()) return 0;
    
    return msr_read(MSR_MC0_STATUS + bank * 4);
}

void interrupt_mgmt_clear_mce_bank_status(u32 bank) {
    if (!interrupt_mgmt_is_mce_bank_valid(bank) || !msr_is_supported()) return;
    
    msr_write(MSR_MC0_STATUS + bank * 4, 0);
}

void interrupt_mgmt_enable_mce_bank(u32 bank) {
    if (!interrupt_mgmt_is_mce_bank_valid(bank) || !msr_is_supported()) return;
    
    msr_write(MSR_MC0_CTL + bank * 4, 0xFFFFFFFFFFFFFFFFULL);
}

void interrupt_mgmt_disable_mce_bank(u32 bank) {
    if (!interrupt_mgmt_is_mce_bank_valid(bank) || !msr_is_supported()) return;
    
    msr_write(MSR_MC0_CTL + bank * 4, 0);
}
