/*
 * system_ctrl.c – x86 advanced system control (SMM/ACPI extensions)
 */

#include "kernel/types.h"

/* SMM MSRs */
#define MSR_SMM_BASE                0x9E
#define MSR_SMM_MASK                0x9F
#define MSR_SMRR_PHYSBASE           0x1F2
#define MSR_SMRR_PHYSMASK           0x1F3

/* ACPI PM1 Control Register */
#define ACPI_PM1_CNT_SLP_TYP_SHIFT  10
#define ACPI_PM1_CNT_SLP_EN         (1 << 13)

/* System Management Mode */
#define SMM_REVISION_ID_OFFSET      0xFEFC
#define SMM_STATE_SAVE_AREA_SIZE    0x200

/* Power states */
#define ACPI_S0_WORKING             0
#define ACPI_S1_SLEEPING            1
#define ACPI_S3_SUSPEND_TO_RAM      3
#define ACPI_S4_SUSPEND_TO_DISK     4
#define ACPI_S5_SOFT_OFF            5

/* System reset methods */
#define RESET_METHOD_KBC            0
#define RESET_METHOD_ACPI           1
#define RESET_METHOD_PCI            2
#define RESET_METHOD_CF9            3

typedef struct {
    u64 smbase;
    u32 smm_size;
    u8 smm_revision;
    u8 smm_enabled;
    u32 smi_count;
    u64 last_smi_time;
} smm_info_t;

typedef struct {
    u16 pm1a_cnt_port;
    u16 pm1b_cnt_port;
    u16 pm1a_evt_port;
    u16 pm1b_evt_port;
    u8 s3_supported;
    u8 s4_supported;
    u8 s5_supported;
    u8 current_state;
} acpi_power_info_t;

typedef struct {
    u8 system_ctrl_supported;
    u8 smm_supported;
    u8 acpi_supported;
    u8 reset_supported;
    smm_info_t smm;
    acpi_power_info_t acpi;
    u8 preferred_reset_method;
    u32 system_resets;
    u32 power_state_changes;
    u64 uptime_seconds;
    u64 last_reset_time;
} system_ctrl_info_t;

static system_ctrl_info_t system_ctrl_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern void outb(u16 port, u8 value);
extern u8 inb(u16 port);
extern u16 inw(u16 port);
extern void outw(u16 port, u16 value);
extern u64 timer_get_ticks(void);

static void system_ctrl_detect_smm(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for SMM support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    system_ctrl_info.smm_supported = (edx & (1 << 29)) != 0; /* SMX bit indicates SMM */
    
    if (system_ctrl_info.smm_supported && msr_is_supported()) {
        /* Read SMRR base and mask */
        u64 smrr_base = msr_read(MSR_SMRR_PHYSBASE);
        u64 smrr_mask = msr_read(MSR_SMRR_PHYSMASK);
        
        if (smrr_mask & (1ULL << 11)) { /* SMRR enabled */
            system_ctrl_info.smm.smbase = smrr_base & ~0xFFF;
            system_ctrl_info.smm.smm_size = (~(smrr_mask & ~0xFFF) + 1) & 0xFFFFFFFF;
            system_ctrl_info.smm.smm_enabled = 1;
        }
    }
}

static void system_ctrl_detect_acpi(void) {
    /* Basic ACPI detection - in real implementation would parse ACPI tables */
    system_ctrl_info.acpi_supported = 1;
    
    /* Common ACPI PM1 control ports */
    system_ctrl_info.acpi.pm1a_cnt_port = 0x404;
    system_ctrl_info.acpi.pm1b_cnt_port = 0x0;
    system_ctrl_info.acpi.pm1a_evt_port = 0x400;
    system_ctrl_info.acpi.pm1b_evt_port = 0x0;
    
    /* Assume S3, S4, S5 are supported */
    system_ctrl_info.acpi.s3_supported = 1;
    system_ctrl_info.acpi.s4_supported = 1;
    system_ctrl_info.acpi.s5_supported = 1;
    system_ctrl_info.acpi.current_state = ACPI_S0_WORKING;
}

void system_ctrl_init(void) {
    system_ctrl_info.system_ctrl_supported = 0;
    system_ctrl_info.smm_supported = 0;
    system_ctrl_info.acpi_supported = 0;
    system_ctrl_info.reset_supported = 1;
    system_ctrl_info.preferred_reset_method = RESET_METHOD_CF9;
    system_ctrl_info.system_resets = 0;
    system_ctrl_info.power_state_changes = 0;
    system_ctrl_info.uptime_seconds = 0;
    system_ctrl_info.last_reset_time = 0;
    
    /* Initialize SMM info */
    system_ctrl_info.smm.smbase = 0x30000;
    system_ctrl_info.smm.smm_size = 0x10000;
    system_ctrl_info.smm.smm_revision = 0;
    system_ctrl_info.smm.smm_enabled = 0;
    system_ctrl_info.smm.smi_count = 0;
    system_ctrl_info.smm.last_smi_time = 0;
    
    system_ctrl_detect_smm();
    system_ctrl_detect_acpi();
    
    if (system_ctrl_info.smm_supported || system_ctrl_info.acpi_supported) {
        system_ctrl_info.system_ctrl_supported = 1;
    }
}

u8 system_ctrl_is_supported(void) {
    return system_ctrl_info.system_ctrl_supported;
}

u8 system_ctrl_is_smm_supported(void) {
    return system_ctrl_info.smm_supported;
}

u8 system_ctrl_is_acpi_supported(void) {
    return system_ctrl_info.acpi_supported;
}

void system_ctrl_reset(u8 method) {
    system_ctrl_info.system_resets++;
    system_ctrl_info.last_reset_time = timer_get_ticks();
    
    switch (method) {
        case RESET_METHOD_KBC:
            /* Keyboard controller reset */
            outb(0x64, 0xFE);
            break;
            
        case RESET_METHOD_CF9:
            /* PCI Configuration Space reset */
            outb(0xCF9, 0x02);
            outb(0xCF9, 0x06);
            break;
            
        case RESET_METHOD_ACPI:
            if (system_ctrl_info.acpi_supported) {
                /* ACPI reset register */
                outb(0xCF9, 0x0E);
            }
            break;
            
        case RESET_METHOD_PCI:
            /* PCI reset through configuration space */
            outw(0xCF8, 0x8000F8AC);
            outb(0xCFC, 0x00);
            break;
            
        default:
            /* Default to CF9 method */
            outb(0xCF9, 0x06);
            break;
    }
    
    /* If we reach here, reset failed */
    while (1) {
        __asm__ volatile("hlt");
    }
}

void system_ctrl_shutdown(void) {
    if (!system_ctrl_info.acpi_supported) return;
    
    system_ctrl_info.power_state_changes++;
    
    /* ACPI S5 (soft off) */
    u16 pm1a_cnt = system_ctrl_info.acpi.pm1a_cnt_port;
    if (pm1a_cnt != 0) {
        u16 value = (ACPI_S5_SOFT_OFF << ACPI_PM1_CNT_SLP_TYP_SHIFT) | ACPI_PM1_CNT_SLP_EN;
        outw(pm1a_cnt, value);
    }
    
    /* Alternative shutdown methods */
    outw(0x604, 0x2000);  /* QEMU */
    outw(0xB004, 0x2000); /* Bochs */
    outw(0x4004, 0x3400); /* VirtualBox */
    
    /* If ACPI shutdown fails, try APM */
    __asm__ volatile(
        "movw $0x5307, %%ax\n\t"
        "movw $0x0001, %%bx\n\t"
        "movw $0x0003, %%cx\n\t"
        "int $0x15"
        : : : "ax", "bx", "cx"
    );
}

void system_ctrl_suspend_to_ram(void) {
    if (!system_ctrl_info.acpi_supported || !system_ctrl_info.acpi.s3_supported) return;
    
    system_ctrl_info.power_state_changes++;
    system_ctrl_info.acpi.current_state = ACPI_S3_SUSPEND_TO_RAM;
    
    /* ACPI S3 (suspend to RAM) */
    u16 pm1a_cnt = system_ctrl_info.acpi.pm1a_cnt_port;
    if (pm1a_cnt != 0) {
        u16 value = (ACPI_S3_SUSPEND_TO_RAM << ACPI_PM1_CNT_SLP_TYP_SHIFT) | ACPI_PM1_CNT_SLP_EN;
        outw(pm1a_cnt, value);
    }
}

void system_ctrl_suspend_to_disk(void) {
    if (!system_ctrl_info.acpi_supported || !system_ctrl_info.acpi.s4_supported) return;
    
    system_ctrl_info.power_state_changes++;
    system_ctrl_info.acpi.current_state = ACPI_S4_SUSPEND_TO_DISK;
    
    /* ACPI S4 (suspend to disk) */
    u16 pm1a_cnt = system_ctrl_info.acpi.pm1a_cnt_port;
    if (pm1a_cnt != 0) {
        u16 value = (ACPI_S4_SUSPEND_TO_DISK << ACPI_PM1_CNT_SLP_TYP_SHIFT) | ACPI_PM1_CNT_SLP_EN;
        outw(pm1a_cnt, value);
    }
}

void system_ctrl_handle_smi(void) {
    if (!system_ctrl_info.smm_supported) return;
    
    system_ctrl_info.smm.smi_count++;
    system_ctrl_info.smm.last_smi_time = timer_get_ticks();
    
    /* SMI handling would be done in SMM mode */
    /* This is just for statistics tracking */
}

u32 system_ctrl_get_smi_count(void) {
    return system_ctrl_info.smm.smi_count;
}

u64 system_ctrl_get_last_smi_time(void) {
    return system_ctrl_info.smm.last_smi_time;
}

u64 system_ctrl_get_smm_base(void) {
    return system_ctrl_info.smm.smbase;
}

u32 system_ctrl_get_smm_size(void) {
    return system_ctrl_info.smm.smm_size;
}

u8 system_ctrl_is_smm_enabled(void) {
    return system_ctrl_info.smm.smm_enabled;
}

u8 system_ctrl_get_acpi_state(void) {
    return system_ctrl_info.acpi.current_state;
}

u8 system_ctrl_is_s3_supported(void) {
    return system_ctrl_info.acpi.s3_supported;
}

u8 system_ctrl_is_s4_supported(void) {
    return system_ctrl_info.acpi.s4_supported;
}

u8 system_ctrl_is_s5_supported(void) {
    return system_ctrl_info.acpi.s5_supported;
}

u32 system_ctrl_get_reset_count(void) {
    return system_ctrl_info.system_resets;
}

u32 system_ctrl_get_power_state_changes(void) {
    return system_ctrl_info.power_state_changes;
}

u64 system_ctrl_get_uptime(void) {
    return timer_get_ticks() / 1000; /* Convert to seconds */
}

u64 system_ctrl_get_last_reset_time(void) {
    return system_ctrl_info.last_reset_time;
}

void system_ctrl_set_preferred_reset_method(u8 method) {
    if (method <= RESET_METHOD_CF9) {
        system_ctrl_info.preferred_reset_method = method;
    }
}

u8 system_ctrl_get_preferred_reset_method(void) {
    return system_ctrl_info.preferred_reset_method;
}

void system_ctrl_emergency_reset(void) {
    /* Try multiple reset methods in sequence */
    outb(0xCF9, 0x06);  /* CF9 reset */
    
    /* Wait a bit */
    for (volatile u32 i = 0; i < 1000000; i++);
    
    outb(0x64, 0xFE);   /* Keyboard controller reset */
    
    /* Wait a bit more */
    for (volatile u32 i = 0; i < 1000000; i++);
    
    /* Triple fault */
    __asm__ volatile("lidt %0" : : "m"((u16){0, 0}));
    __asm__ volatile("int $0x03");
}

void system_ctrl_warm_reset(void) {
    /* Warm reset preserves memory content */
    outb(0xCF9, 0x02);
    outb(0xCF9, 0x06);
}

void system_ctrl_cold_reset(void) {
    /* Cold reset clears all system state */
    outb(0xCF9, 0x0E);
}

u8 system_ctrl_detect_reset_cause(void) {
    /* Read reset status from various sources */
    u8 cf9_status = inb(0xCF9);
    
    /* Check for power-on reset */
    if (cf9_status & 0x01) return 1; /* Power-on reset */
    if (cf9_status & 0x02) return 2; /* Warm reset */
    if (cf9_status & 0x04) return 3; /* Cold reset */
    
    return 0; /* Unknown */
}

void system_ctrl_enable_wake_events(u32 events) {
    if (!system_ctrl_info.acpi_supported) return;
    
    /* Enable wake events in ACPI PM1 enable register */
    u16 pm1a_evt = system_ctrl_info.acpi.pm1a_evt_port;
    if (pm1a_evt != 0) {
        u16 current = inw(pm1a_evt + 2); /* PM1_EN register */
        outw(pm1a_evt + 2, current | (events & 0xFFFF));
    }
}

void system_ctrl_disable_wake_events(u32 events) {
    if (!system_ctrl_info.acpi_supported) return;
    
    /* Disable wake events in ACPI PM1 enable register */
    u16 pm1a_evt = system_ctrl_info.acpi.pm1a_evt_port;
    if (pm1a_evt != 0) {
        u16 current = inw(pm1a_evt + 2); /* PM1_EN register */
        outw(pm1a_evt + 2, current & ~(events & 0xFFFF));
    }
}
