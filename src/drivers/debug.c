/*
 * debug.c – x86 advanced debugging (LBR, BTS, PMU)
 */

#include "kernel/types.h"

/* Last Branch Record MSRs */
#define MSR_LBR_SELECT          0x1C8
#define MSR_LBR_TOS             0x1C9
#define MSR_LBR_FROM_0          0x680
#define MSR_LBR_TO_0            0x6C0
#define MSR_LBR_INFO_0          0xDC0

/* Branch Trace Store MSRs */
#define MSR_BTS_BUFFER_BASE     0x2A0
#define MSR_BTS_INDEX           0x2A1
#define MSR_BTS_ABSOLUTE_MAX    0x2A2
#define MSR_BTS_INTERRUPT_THRESH 0x2A3

/* Debug control MSRs */
#define MSR_DEBUGCTL            0x1D9
#define MSR_LASTBRANCHFROMIP    0x1DB
#define MSR_LASTBRANCHTOIP      0x1DC
#define MSR_LASTINTFROMIP       0x1DD
#define MSR_LASTINTTOIP         0x1DE

/* DEBUGCTL bits */
#define DEBUGCTL_LBR            0x01    /* Last Branch Record */
#define DEBUGCTL_BTF            0x02    /* Single Step on Branches */
#define DEBUGCTL_TR             0x40    /* Trace Messages Enable */
#define DEBUGCTL_BTS            0x80    /* Branch Trace Store */
#define DEBUGCTL_BTINT          0x100   /* Branch Trace Interrupt */
#define DEBUGCTL_BTS_OFF_OS     0x200   /* BTS off if CPL = 0 */
#define DEBUGCTL_BTS_OFF_USR    0x400   /* BTS off if CPL > 0 */
#define DEBUGCTL_FREEZE_LBRS_ON_PMI 0x800 /* Freeze LBRs on PMI */

/* LBR_SELECT bits */
#define LBR_SELECT_CPL_EQ_0     0x01    /* CPL = 0 */
#define LBR_SELECT_CPL_NEQ_0    0x02    /* CPL != 0 */
#define LBR_SELECT_JCC          0x04    /* Conditional branches */
#define LBR_SELECT_NEAR_REL_CALL 0x08   /* Near relative calls */
#define LBR_SELECT_NEAR_IND_CALL 0x10   /* Near indirect calls */
#define LBR_SELECT_NEAR_RET     0x20    /* Near returns */
#define LBR_SELECT_NEAR_IND_JMP 0x40    /* Near indirect jumps */
#define LBR_SELECT_NEAR_REL_JMP 0x80    /* Near relative jumps */
#define LBR_SELECT_FAR_BRANCH   0x100   /* Far branches */

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 info;
} lbr_entry_t;

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 flags;
} bts_entry_t;

typedef struct {
    u8 lbr_supported;
    u8 bts_supported;
    u8 lbr_enabled;
    u8 bts_enabled;
    u8 lbr_depth;
    u8 lbr_format;
    u32 lbr_select_mask;
    lbr_entry_t lbr_stack[32];
    bts_entry_t* bts_buffer;
    u32 bts_buffer_size;
    u32 bts_index;
    u64 debugctl_value;
} debug_info_t;

static debug_info_t debug_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 debug_check_lbr_support(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check CPUID for LBR support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    if (!(edx & (1 << 21))) return 0; /* No debug store */
    
    /* Get LBR capabilities */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xA));
    
    if ((eax & 0xFF) < 1) return 0; /* No architectural PMU */
    
    debug_info.lbr_depth = (eax >> 16) & 0xFF;
    debug_info.lbr_format = (eax >> 24) & 0xFF;
    
    return debug_info.lbr_depth > 0;
}

static u8 debug_check_bts_support(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check CPUID for BTS support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (edx & (1 << 21)) != 0; /* Debug store supported */
}

static void debug_setup_lbr(void) {
    if (!debug_info.lbr_supported) return;
    
    /* Configure LBR select register */
    debug_info.lbr_select_mask = LBR_SELECT_CPL_EQ_0 | LBR_SELECT_CPL_NEQ_0 |
                                LBR_SELECT_JCC | LBR_SELECT_NEAR_REL_CALL |
                                LBR_SELECT_NEAR_IND_CALL | LBR_SELECT_NEAR_RET |
                                LBR_SELECT_NEAR_IND_JMP;
    
    msr_write(MSR_LBR_SELECT, debug_info.lbr_select_mask);
}

static void debug_setup_bts(void) {
    if (!debug_info.bts_supported) return;
    
    extern void* paging_alloc_pages(u32 count);
    
    /* Allocate BTS buffer (4KB = 128 entries) */
    debug_info.bts_buffer_size = 128;
    debug_info.bts_buffer = (bts_entry_t*)paging_alloc_pages(1);
    
    if (!debug_info.bts_buffer) return;
    
    /* Clear buffer */
    for (u32 i = 0; i < debug_info.bts_buffer_size; i++) {
        debug_info.bts_buffer[i].from_ip = 0;
        debug_info.bts_buffer[i].to_ip = 0;
        debug_info.bts_buffer[i].flags = 0;
    }
    
    /* Configure BTS MSRs */
    msr_write(MSR_BTS_BUFFER_BASE, (u64)debug_info.bts_buffer);
    msr_write(MSR_BTS_INDEX, (u64)debug_info.bts_buffer);
    msr_write(MSR_BTS_ABSOLUTE_MAX, (u64)debug_info.bts_buffer + 
              (debug_info.bts_buffer_size * sizeof(bts_entry_t)));
    msr_write(MSR_BTS_INTERRUPT_THRESH, (u64)debug_info.bts_buffer + 
              ((debug_info.bts_buffer_size - 8) * sizeof(bts_entry_t)));
    
    debug_info.bts_index = 0;
}

void debug_init(void) {
    debug_info.lbr_supported = 0;
    debug_info.bts_supported = 0;
    debug_info.lbr_enabled = 0;
    debug_info.bts_enabled = 0;
    debug_info.lbr_depth = 0;
    debug_info.lbr_format = 0;
    debug_info.debugctl_value = 0;
    
    if (!msr_is_supported()) return;
    
    debug_info.lbr_supported = debug_check_lbr_support();
    debug_info.bts_supported = debug_check_bts_support();
    
    if (debug_info.lbr_supported) {
        debug_setup_lbr();
    }
    
    if (debug_info.bts_supported) {
        debug_setup_bts();
    }
}

u8 debug_is_lbr_supported(void) {
    return debug_info.lbr_supported;
}

u8 debug_is_bts_supported(void) {
    return debug_info.bts_supported;
}

u8 debug_is_lbr_enabled(void) {
    return debug_info.lbr_enabled;
}

u8 debug_is_bts_enabled(void) {
    return debug_info.bts_enabled;
}

void debug_enable_lbr(void) {
    if (!debug_info.lbr_supported) return;
    
    debug_info.debugctl_value |= DEBUGCTL_LBR;
    msr_write(MSR_DEBUGCTL, debug_info.debugctl_value);
    
    debug_info.lbr_enabled = 1;
}

void debug_disable_lbr(void) {
    if (!debug_info.lbr_supported) return;
    
    debug_info.debugctl_value &= ~DEBUGCTL_LBR;
    msr_write(MSR_DEBUGCTL, debug_info.debugctl_value);
    
    debug_info.lbr_enabled = 0;
}

void debug_enable_bts(void) {
    if (!debug_info.bts_supported || !debug_info.bts_buffer) return;
    
    debug_info.debugctl_value |= DEBUGCTL_BTS | DEBUGCTL_BTINT;
    msr_write(MSR_DEBUGCTL, debug_info.debugctl_value);
    
    debug_info.bts_enabled = 1;
}

void debug_disable_bts(void) {
    if (!debug_info.bts_supported) return;
    
    debug_info.debugctl_value &= ~(DEBUGCTL_BTS | DEBUGCTL_BTINT);
    msr_write(MSR_DEBUGCTL, debug_info.debugctl_value);
    
    debug_info.bts_enabled = 0;
}

u32 debug_read_lbr_stack(lbr_entry_t* buffer, u32 max_entries) {
    if (!debug_info.lbr_supported || !buffer) return 0;
    
    u32 tos = (u32)msr_read(MSR_LBR_TOS);
    u32 entries_read = 0;
    
    for (u32 i = 0; i < debug_info.lbr_depth && entries_read < max_entries; i++) {
        u32 index = (tos - i) % debug_info.lbr_depth;
        
        buffer[entries_read].from_ip = msr_read(MSR_LBR_FROM_0 + index);
        buffer[entries_read].to_ip = msr_read(MSR_LBR_TO_0 + index);
        
        if (debug_info.lbr_format >= 5) {
            buffer[entries_read].info = msr_read(MSR_LBR_INFO_0 + index);
        } else {
            buffer[entries_read].info = 0;
        }
        
        /* Stop if entry is empty */
        if (buffer[entries_read].from_ip == 0 && buffer[entries_read].to_ip == 0) {
            break;
        }
        
        entries_read++;
    }
    
    return entries_read;
}

void debug_clear_lbr_stack(void) {
    if (!debug_info.lbr_supported) return;
    
    for (u32 i = 0; i < debug_info.lbr_depth; i++) {
        msr_write(MSR_LBR_FROM_0 + i, 0);
        msr_write(MSR_LBR_TO_0 + i, 0);
        if (debug_info.lbr_format >= 5) {
            msr_write(MSR_LBR_INFO_0 + i, 0);
        }
    }
    
    msr_write(MSR_LBR_TOS, 0);
}

u32 debug_read_bts_buffer(bts_entry_t* buffer, u32 max_entries) {
    if (!debug_info.bts_supported || !debug_info.bts_buffer || !buffer) return 0;
    
    u64 current_index = msr_read(MSR_BTS_INDEX);
    u32 entries_in_buffer = ((current_index - (u64)debug_info.bts_buffer) / sizeof(bts_entry_t));
    
    if (entries_in_buffer > debug_info.bts_buffer_size) {
        entries_in_buffer = debug_info.bts_buffer_size;
    }
    
    u32 entries_to_copy = (entries_in_buffer < max_entries) ? entries_in_buffer : max_entries;
    
    for (u32 i = 0; i < entries_to_copy; i++) {
        buffer[i] = debug_info.bts_buffer[i];
    }
    
    return entries_to_copy;
}

void debug_clear_bts_buffer(void) {
    if (!debug_info.bts_supported || !debug_info.bts_buffer) return;
    
    /* Reset BTS index to buffer base */
    msr_write(MSR_BTS_INDEX, (u64)debug_info.bts_buffer);
    
    /* Clear buffer */
    for (u32 i = 0; i < debug_info.bts_buffer_size; i++) {
        debug_info.bts_buffer[i].from_ip = 0;
        debug_info.bts_buffer[i].to_ip = 0;
        debug_info.bts_buffer[i].flags = 0;
    }
    
    debug_info.bts_index = 0;
}

void debug_set_lbr_filter(u32 filter_mask) {
    if (!debug_info.lbr_supported) return;
    
    debug_info.lbr_select_mask = filter_mask;
    msr_write(MSR_LBR_SELECT, filter_mask);
}

u32 debug_get_lbr_filter(void) {
    return debug_info.lbr_select_mask;
}

u8 debug_get_lbr_depth(void) {
    return debug_info.lbr_depth;
}

u8 debug_get_lbr_format(void) {
    return debug_info.lbr_format;
}

u32 debug_get_bts_buffer_size(void) {
    return debug_info.bts_buffer_size;
}

void debug_freeze_on_pmi(u8 enable) {
    if (!debug_info.lbr_supported) return;
    
    if (enable) {
        debug_info.debugctl_value |= DEBUGCTL_FREEZE_LBRS_ON_PMI;
    } else {
        debug_info.debugctl_value &= ~DEBUGCTL_FREEZE_LBRS_ON_PMI;
    }
    
    msr_write(MSR_DEBUGCTL, debug_info.debugctl_value);
}

void debug_single_step_branches(u8 enable) {
    if (!debug_info.lbr_supported) return;
    
    if (enable) {
        debug_info.debugctl_value |= DEBUGCTL_BTF;
    } else {
        debug_info.debugctl_value &= ~DEBUGCTL_BTF;
    }
    
    msr_write(MSR_DEBUGCTL, debug_info.debugctl_value);
}

u64 debug_get_last_branch_from(void) {
    if (!debug_info.lbr_supported) return 0;
    
    return msr_read(MSR_LASTBRANCHFROMIP);
}

u64 debug_get_last_branch_to(void) {
    if (!debug_info.lbr_supported) return 0;
    
    return msr_read(MSR_LASTBRANCHTOIP);
}

void debug_bts_interrupt_handler(void) {
    if (!debug_info.bts_enabled) return;
    
    /* BTS buffer is nearly full, handle overflow */
    debug_info.bts_index = debug_info.bts_buffer_size;
    
    /* Reset buffer or notify user */
    debug_clear_bts_buffer();
}

u64 debug_get_debugctl(void) {
    return debug_info.debugctl_value;
}
