/*
 * system_mgmt_mode.c – x86 System Management Mode (SMM/SMI) management
 */

#include "kernel/types.h"

/* SMM MSRs */
#define MSR_SMM_BASE                    0x9E
#define MSR_SMM_MASK                    0x9F
#define MSR_SMI_COUNT                   0x34

/* SMRAM state save area offsets */
#define SMRAM_STATE_EAX                 0xFFF8
#define SMRAM_STATE_ECX                 0xFFF4
#define SMRAM_STATE_EDX                 0xFFF0
#define SMRAM_STATE_EBX                 0xFFEC
#define SMRAM_STATE_ESP                 0xFFE8
#define SMRAM_STATE_EBP                 0xFFE4
#define SMRAM_STATE_ESI                 0xFFE0
#define SMRAM_STATE_EDI                 0xFFDC
#define SMRAM_STATE_EIP                 0xFFD8
#define SMRAM_STATE_EFLAGS              0xFFD4
#define SMRAM_STATE_CR0                 0xFFD0
#define SMRAM_STATE_CR3                 0xFFCC
#define SMRAM_STATE_CR4                 0xFFC8

/* SMI sources */
#define SMI_SOURCE_TIMER                0
#define SMI_SOURCE_GPIO                 1
#define SMI_SOURCE_LEGACY_USB           2
#define SMI_SOURCE_PERIODIC             3
#define SMI_SOURCE_TCO                  4
#define SMI_SOURCE_MCSMI                5
#define SMI_SOURCE_BIOS_RLS             6
#define SMI_SOURCE_BIOS_EN              7
#define SMI_SOURCE_EOS                  8
#define SMI_SOURCE_GBL_SMI_EN           9
#define SMI_SOURCE_LEGACY_USB2          10
#define SMI_SOURCE_INTEL_USB2           11
#define SMI_SOURCE_SWSMI_TMR            12
#define SMI_SOURCE_APMC                 13
#define SMI_SOURCE_SLP_SMI              14
#define SMI_SOURCE_PM1_STS              15

/* SMM revision identifiers */
#define SMM_REV_P6                      0x00020000
#define SMM_REV_PENTIUM                 0x00010000
#define SMM_REV_I386                    0x00000000

typedef struct {
    u32 eax, ecx, edx, ebx;
    u32 esp, ebp, esi, edi;
    u32 eip, eflags;
    u32 cr0, cr3, cr4;
    u32 es, cs, ss, ds, fs, gs;
    u32 ldtr, tr;
    u32 dr6, dr7;
    u64 smbase;
    u32 smm_revision;
    u32 auto_halt_restart;
    u64 io_restart_rip;
    u64 io_restart_rcx;
    u64 io_restart_rsi;
    u64 io_restart_rdi;
} smm_state_save_t;

typedef struct {
    u8 smi_source;
    u64 timestamp;
    u32 duration_cycles;
    u32 cpu_id;
    u64 instruction_pointer;
} smi_event_t;

typedef struct {
    u8 smm_supported;
    u8 smm_enabled;
    u8 smm_locked;
    u64 smram_base;
    u32 smram_size;
    u32 smm_revision;
    u32 total_smi_count;
    u32 smi_sources_enabled;
    smi_event_t smi_events[64];
    u32 smi_event_index;
    u32 num_smi_events;
    u64 total_smm_time;
    u64 last_smi_time;
    u32 smm_entries;
    u32 smm_exits;
    u32 smi_source_counts[16];
    u8 smm_handler_installed;
    u64 smm_handler_address;
} system_mgmt_mode_info_t;

static system_mgmt_mode_info_t smm_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);
extern u32 cpu_get_current_id(void);
extern u32 inl(u16 port);
extern void outl(u16 port, u32 value);

static void system_mgmt_mode_detect_smm(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for SMM support via CPUID */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    /* SMM is typically available on all x86 processors, but check for specific features */
    smm_info.smm_supported = 1;
    
    if (msr_is_supported()) {
        /* Try to read SMM base MSR */
        u64 smm_base = msr_read(MSR_SMM_BASE);
        if (smm_base != 0xFFFFFFFFFFFFFFFFULL) {
            smm_info.smram_base = smm_base & 0xFFFFF000ULL;
            smm_info.smram_size = 0x10000;  /* Default 64KB SMRAM */
        } else {
            /* Default SMRAM location */
            smm_info.smram_base = 0x30000;
            smm_info.smram_size = 0x10000;
        }
    } else {
        /* Default SMRAM for older processors */
        smm_info.smram_base = 0x30000;
        smm_info.smram_size = 0x10000;
    }
    
    /* Detect SMM revision */
    smm_info.smm_revision = SMM_REV_P6;  /* Assume modern processor */
}

void system_mgmt_mode_init(void) {
    smm_info.smm_supported = 0;
    smm_info.smm_enabled = 0;
    smm_info.smm_locked = 0;
    smm_info.smram_base = 0;
    smm_info.smram_size = 0;
    smm_info.smm_revision = 0;
    smm_info.total_smi_count = 0;
    smm_info.smi_sources_enabled = 0;
    smm_info.smi_event_index = 0;
    smm_info.num_smi_events = 0;
    smm_info.total_smm_time = 0;
    smm_info.last_smi_time = 0;
    smm_info.smm_entries = 0;
    smm_info.smm_exits = 0;
    smm_info.smm_handler_installed = 0;
    smm_info.smm_handler_address = 0;
    
    for (u32 i = 0; i < 64; i++) {
        smm_info.smi_events[i].smi_source = 0;
        smm_info.smi_events[i].timestamp = 0;
        smm_info.smi_events[i].duration_cycles = 0;
        smm_info.smi_events[i].cpu_id = 0;
        smm_info.smi_events[i].instruction_pointer = 0;
    }
    
    for (u8 i = 0; i < 16; i++) {
        smm_info.smi_source_counts[i] = 0;
    }
    
    system_mgmt_mode_detect_smm();
}

u8 system_mgmt_mode_is_supported(void) {
    return smm_info.smm_supported;
}

u8 system_mgmt_mode_enable_smi_source(u8 smi_source) {
    if (!smm_info.smm_supported || smi_source >= 16) return 0;
    
    /* Enable specific SMI source (implementation depends on chipset) */
    smm_info.smi_sources_enabled |= (1 << smi_source);
    
    return 1;
}

u8 system_mgmt_mode_disable_smi_source(u8 smi_source) {
    if (!smm_info.smm_supported || smi_source >= 16) return 0;
    
    /* Disable specific SMI source */
    smm_info.smi_sources_enabled &= ~(1 << smi_source);
    
    return 1;
}

void system_mgmt_mode_trigger_smi(u8 smi_source) {
    if (!smm_info.smm_supported) return;
    
    /* Trigger SMI via APMC port (0xB2) for software SMI */
    if (smi_source == SMI_SOURCE_APMC) {
        outl(0xB2, 0x00);  /* Software SMI */
    }
    
    /* Record SMI trigger */
    smm_info.smi_source_counts[smi_source]++;
}

void system_mgmt_mode_handle_smi_entry(u8 smi_source, u64 instruction_pointer) {
    u64 entry_time = timer_get_ticks();
    
    /* Record SMI event */
    smi_event_t* event = &smm_info.smi_events[smm_info.smi_event_index];
    event->smi_source = smi_source;
    event->timestamp = entry_time;
    event->cpu_id = cpu_get_current_id();
    event->instruction_pointer = instruction_pointer;
    
    smm_info.smm_entries++;
    smm_info.total_smi_count++;
    smm_info.last_smi_time = entry_time;
    smm_info.smi_source_counts[smi_source]++;
    
    smm_info.smi_event_index = (smm_info.smi_event_index + 1) % 64;
    if (smm_info.num_smi_events < 64) {
        smm_info.num_smi_events++;
    }
}

void system_mgmt_mode_handle_smi_exit(u32 duration_cycles) {
    if (smm_info.num_smi_events > 0) {
        u32 last_index = (smm_info.smi_event_index + 63) % 64;
        smm_info.smi_events[last_index].duration_cycles = duration_cycles;
        smm_info.total_smm_time += duration_cycles;
    }
    
    smm_info.smm_exits++;
}

u8 system_mgmt_mode_install_handler(void* handler_code, u32 handler_size) {
    if (!smm_info.smm_supported || handler_size > smm_info.smram_size) return 0;
    
    /* Install SMM handler in SMRAM */
    volatile u8* smram = (volatile u8*)smm_info.smram_base;
    u8* code = (u8*)handler_code;
    
    for (u32 i = 0; i < handler_size; i++) {
        smram[i] = code[i];
    }
    
    smm_info.smm_handler_installed = 1;
    smm_info.smm_handler_address = smm_info.smram_base;
    
    return 1;
}

void system_mgmt_mode_save_state(smm_state_save_t* state) {
    if (!smm_info.smm_supported) return;
    
    /* Save current processor state to SMM state save area */
    volatile u32* smram = (volatile u32*)smm_info.smram_base;
    
    smram[SMRAM_STATE_EAX / 4] = state->eax;
    smram[SMRAM_STATE_ECX / 4] = state->ecx;
    smram[SMRAM_STATE_EDX / 4] = state->edx;
    smram[SMRAM_STATE_EBX / 4] = state->ebx;
    smram[SMRAM_STATE_ESP / 4] = state->esp;
    smram[SMRAM_STATE_EBP / 4] = state->ebp;
    smram[SMRAM_STATE_ESI / 4] = state->esi;
    smram[SMRAM_STATE_EDI / 4] = state->edi;
    smram[SMRAM_STATE_EIP / 4] = state->eip;
    smram[SMRAM_STATE_EFLAGS / 4] = state->eflags;
    smram[SMRAM_STATE_CR0 / 4] = state->cr0;
    smram[SMRAM_STATE_CR3 / 4] = state->cr3;
    smram[SMRAM_STATE_CR4 / 4] = state->cr4;
}

void system_mgmt_mode_restore_state(smm_state_save_t* state) {
    if (!smm_info.smm_supported) return;
    
    /* Restore processor state from SMM state save area */
    volatile u32* smram = (volatile u32*)smm_info.smram_base;
    
    state->eax = smram[SMRAM_STATE_EAX / 4];
    state->ecx = smram[SMRAM_STATE_ECX / 4];
    state->edx = smram[SMRAM_STATE_EDX / 4];
    state->ebx = smram[SMRAM_STATE_EBX / 4];
    state->esp = smram[SMRAM_STATE_ESP / 4];
    state->ebp = smram[SMRAM_STATE_EBP / 4];
    state->esi = smram[SMRAM_STATE_ESI / 4];
    state->edi = smram[SMRAM_STATE_EDI / 4];
    state->eip = smram[SMRAM_STATE_EIP / 4];
    state->eflags = smram[SMRAM_STATE_EFLAGS / 4];
    state->cr0 = smram[SMRAM_STATE_CR0 / 4];
    state->cr3 = smram[SMRAM_STATE_CR3 / 4];
    state->cr4 = smram[SMRAM_STATE_CR4 / 4];
}

u8 system_mgmt_mode_is_enabled(void) {
    return smm_info.smm_enabled;
}

u8 system_mgmt_mode_is_locked(void) {
    return smm_info.smm_locked;
}

u64 system_mgmt_mode_get_smram_base(void) {
    return smm_info.smram_base;
}

u32 system_mgmt_mode_get_smram_size(void) {
    return smm_info.smram_size;
}

u32 system_mgmt_mode_get_smm_revision(void) {
    return smm_info.smm_revision;
}

u32 system_mgmt_mode_get_total_smi_count(void) {
    return smm_info.total_smi_count;
}

u32 system_mgmt_mode_get_smi_sources_enabled(void) {
    return smm_info.smi_sources_enabled;
}

u32 system_mgmt_mode_get_num_smi_events(void) {
    return smm_info.num_smi_events;
}

const smi_event_t* system_mgmt_mode_get_smi_event(u32 index) {
    if (index >= smm_info.num_smi_events) return 0;
    return &smm_info.smi_events[index];
}

u64 system_mgmt_mode_get_total_smm_time(void) {
    return smm_info.total_smm_time;
}

u64 system_mgmt_mode_get_last_smi_time(void) {
    return smm_info.last_smi_time;
}

u32 system_mgmt_mode_get_smm_entries(void) {
    return smm_info.smm_entries;
}

u32 system_mgmt_mode_get_smm_exits(void) {
    return smm_info.smm_exits;
}

u32 system_mgmt_mode_get_smi_source_count(u8 smi_source) {
    if (smi_source >= 16) return 0;
    return smm_info.smi_source_counts[smi_source];
}

u8 system_mgmt_mode_is_handler_installed(void) {
    return smm_info.smm_handler_installed;
}

u64 system_mgmt_mode_get_handler_address(void) {
    return smm_info.smm_handler_address;
}

const char* system_mgmt_mode_get_smi_source_name(u8 smi_source) {
    switch (smi_source) {
        case SMI_SOURCE_TIMER: return "Timer";
        case SMI_SOURCE_GPIO: return "GPIO";
        case SMI_SOURCE_LEGACY_USB: return "Legacy USB";
        case SMI_SOURCE_PERIODIC: return "Periodic";
        case SMI_SOURCE_TCO: return "TCO";
        case SMI_SOURCE_MCSMI: return "MCSMI";
        case SMI_SOURCE_BIOS_RLS: return "BIOS RLS";
        case SMI_SOURCE_BIOS_EN: return "BIOS EN";
        case SMI_SOURCE_EOS: return "EOS";
        case SMI_SOURCE_GBL_SMI_EN: return "Global SMI EN";
        case SMI_SOURCE_LEGACY_USB2: return "Legacy USB2";
        case SMI_SOURCE_INTEL_USB2: return "Intel USB2";
        case SMI_SOURCE_SWSMI_TMR: return "SW SMI Timer";
        case SMI_SOURCE_APMC: return "APMC";
        case SMI_SOURCE_SLP_SMI: return "Sleep SMI";
        case SMI_SOURCE_PM1_STS: return "PM1 Status";
        default: return "Unknown";
    }
}

void system_mgmt_mode_clear_statistics(void) {
    smm_info.total_smi_count = 0;
    smm_info.num_smi_events = 0;
    smm_info.smi_event_index = 0;
    smm_info.total_smm_time = 0;
    smm_info.smm_entries = 0;
    smm_info.smm_exits = 0;
    
    for (u8 i = 0; i < 16; i++) {
        smm_info.smi_source_counts[i] = 0;
    }
}
