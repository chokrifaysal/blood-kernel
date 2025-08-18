/*
 * cpu_debug_ext.c – x86 CPU debugging extensions (Intel PT/LBR/BTS)
 */

#include "kernel/types.h"

/* Intel Processor Trace MSRs */
#define MSR_IA32_RTIT_CTL               0x570
#define MSR_IA32_RTIT_STATUS            0x571
#define MSR_IA32_RTIT_ADDR0_A           0x580
#define MSR_IA32_RTIT_ADDR0_B           0x581
#define MSR_IA32_RTIT_ADDR1_A           0x582
#define MSR_IA32_RTIT_ADDR1_B           0x583
#define MSR_IA32_RTIT_ADDR2_A           0x584
#define MSR_IA32_RTIT_ADDR2_B           0x585
#define MSR_IA32_RTIT_ADDR3_A           0x586
#define MSR_IA32_RTIT_ADDR3_B           0x587
#define MSR_IA32_RTIT_CR3_MATCH         0x572
#define MSR_IA32_RTIT_OUTPUT_BASE       0x560
#define MSR_IA32_RTIT_OUTPUT_MASK_PTRS  0x561

/* Last Branch Record MSRs */
#define MSR_LBR_SELECT                  0x1C8
#define MSR_LBR_TOS                     0x1C9
#define MSR_LBR_FROM_0                  0x680
#define MSR_LBR_TO_0                    0x6C0
#define MSR_LBR_INFO_0                  0xDC0

/* Branch Trace Store MSRs */
#define MSR_BTS_BUFFER_BASE             0x2A0
#define MSR_BTS_INDEX                   0x2A1
#define MSR_BTS_ABSOLUTE_MAXIMUM        0x2A2
#define MSR_BTS_INTERRUPT_THRESHOLD     0x2A3

/* RTIT Control bits */
#define RTIT_CTL_TRACEEN                (1ULL << 0)
#define RTIT_CTL_CYCEN                  (1ULL << 1)
#define RTIT_CTL_OS                     (1ULL << 2)
#define RTIT_CTL_USER                   (1ULL << 3)
#define RTIT_CTL_PWREVT                 (1ULL << 4)
#define RTIT_CTL_FUPONPTW               (1ULL << 5)
#define RTIT_CTL_FABRICEN               (1ULL << 6)
#define RTIT_CTL_CR3FILTER              (1ULL << 7)
#define RTIT_CTL_TOPA                   (1ULL << 8)
#define RTIT_CTL_MTCEN                  (1ULL << 9)
#define RTIT_CTL_TSCEN                  (1ULL << 10)
#define RTIT_CTL_DISRETC                (1ULL << 11)
#define RTIT_CTL_PTWEN                  (1ULL << 12)
#define RTIT_CTL_BRANCHEN               (1ULL << 13)

/* LBR Select bits */
#define LBR_SELECT_CPL_EQ_0             (1ULL << 0)
#define LBR_SELECT_CPL_NEQ_0            (1ULL << 1)
#define LBR_SELECT_JCC                  (1ULL << 2)
#define LBR_SELECT_NEAR_REL_CALL        (1ULL << 3)
#define LBR_SELECT_NEAR_IND_CALL        (1ULL << 4)
#define LBR_SELECT_NEAR_RET             (1ULL << 5)
#define LBR_SELECT_NEAR_IND_JMP         (1ULL << 6)
#define LBR_SELECT_NEAR_REL_JMP         (1ULL << 7)
#define LBR_SELECT_FAR_BRANCH           (1ULL << 8)
#define LBR_SELECT_EN_CALLSTACK         (1ULL << 9)

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 info;
    u64 timestamp;
} lbr_entry_t;

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 flags;
} bts_entry_t;

typedef struct {
    u8 intel_pt_supported;
    u8 intel_pt_enabled;
    u64 pt_control;
    u64 pt_status;
    u64 pt_output_base;
    u64 pt_output_mask;
    u32 pt_packets_generated;
    u64 pt_trace_size;
} intel_pt_info_t;

typedef struct {
    u8 lbr_supported;
    u8 lbr_enabled;
    u8 lbr_depth;
    u64 lbr_select;
    u32 lbr_tos;
    lbr_entry_t lbr_stack[32];
    u32 lbr_records_captured;
} lbr_info_t;

typedef struct {
    u8 bts_supported;
    u8 bts_enabled;
    u64 bts_buffer_base;
    u64 bts_index;
    u64 bts_absolute_max;
    u64 bts_threshold;
    u32 bts_records_captured;
    u32 bts_buffer_overflows;
} bts_info_t;

typedef struct {
    u8 cpu_debug_ext_supported;
    intel_pt_info_t intel_pt;
    lbr_info_t lbr;
    bts_info_t bts;
    u32 total_trace_sessions;
    u64 total_trace_time;
    u64 last_trace_start;
    u64 last_trace_end;
    u32 debug_interrupts;
} cpu_debug_ext_info_t;

static cpu_debug_ext_info_t debug_ext_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);
extern void* kmalloc(u32 size);
extern u64 virt_to_phys(void* virt);

static void cpu_debug_ext_detect_intel_pt(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for Intel PT support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    debug_ext_info.intel_pt.intel_pt_supported = (ebx & (1 << 25)) != 0;
    
    if (debug_ext_info.intel_pt.intel_pt_supported) {
        /* Get Intel PT capabilities */
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x14), "c"(0));
        
        /* Additional PT feature detection would go here */
    }
}

static void cpu_debug_ext_detect_lbr(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for LBR support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    debug_ext_info.lbr.lbr_supported = (edx & (1 << 21)) != 0;
    
    if (debug_ext_info.lbr.lbr_supported) {
        /* Determine LBR stack depth */
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xA));
        debug_ext_info.lbr.lbr_depth = (eax >> 16) & 0xFF;
        
        if (debug_ext_info.lbr.lbr_depth == 0) {
            debug_ext_info.lbr.lbr_depth = 16;  /* Default depth */
        }
        if (debug_ext_info.lbr.lbr_depth > 32) {
            debug_ext_info.lbr.lbr_depth = 32;  /* Limit to our array size */
        }
    }
}

static void cpu_debug_ext_detect_bts(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for BTS support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    debug_ext_info.bts.bts_supported = (edx & (1 << 21)) != 0;
}

void cpu_debug_ext_init(void) {
    debug_ext_info.cpu_debug_ext_supported = 0;
    debug_ext_info.total_trace_sessions = 0;
    debug_ext_info.total_trace_time = 0;
    debug_ext_info.last_trace_start = 0;
    debug_ext_info.last_trace_end = 0;
    debug_ext_info.debug_interrupts = 0;
    
    /* Initialize Intel PT */
    debug_ext_info.intel_pt.intel_pt_supported = 0;
    debug_ext_info.intel_pt.intel_pt_enabled = 0;
    debug_ext_info.intel_pt.pt_control = 0;
    debug_ext_info.intel_pt.pt_status = 0;
    debug_ext_info.intel_pt.pt_output_base = 0;
    debug_ext_info.intel_pt.pt_output_mask = 0;
    debug_ext_info.intel_pt.pt_packets_generated = 0;
    debug_ext_info.intel_pt.pt_trace_size = 0;
    
    /* Initialize LBR */
    debug_ext_info.lbr.lbr_supported = 0;
    debug_ext_info.lbr.lbr_enabled = 0;
    debug_ext_info.lbr.lbr_depth = 0;
    debug_ext_info.lbr.lbr_select = 0;
    debug_ext_info.lbr.lbr_tos = 0;
    debug_ext_info.lbr.lbr_records_captured = 0;
    
    for (u8 i = 0; i < 32; i++) {
        debug_ext_info.lbr.lbr_stack[i].from_ip = 0;
        debug_ext_info.lbr.lbr_stack[i].to_ip = 0;
        debug_ext_info.lbr.lbr_stack[i].info = 0;
        debug_ext_info.lbr.lbr_stack[i].timestamp = 0;
    }
    
    /* Initialize BTS */
    debug_ext_info.bts.bts_supported = 0;
    debug_ext_info.bts.bts_enabled = 0;
    debug_ext_info.bts.bts_buffer_base = 0;
    debug_ext_info.bts.bts_index = 0;
    debug_ext_info.bts.bts_absolute_max = 0;
    debug_ext_info.bts.bts_threshold = 0;
    debug_ext_info.bts.bts_records_captured = 0;
    debug_ext_info.bts.bts_buffer_overflows = 0;
    
    cpu_debug_ext_detect_intel_pt();
    cpu_debug_ext_detect_lbr();
    cpu_debug_ext_detect_bts();
    
    if (debug_ext_info.intel_pt.intel_pt_supported || 
        debug_ext_info.lbr.lbr_supported || 
        debug_ext_info.bts.bts_supported) {
        debug_ext_info.cpu_debug_ext_supported = 1;
    }
}

u8 cpu_debug_ext_is_supported(void) {
    return debug_ext_info.cpu_debug_ext_supported;
}

u8 cpu_debug_ext_enable_intel_pt(u8 user_mode, u8 kernel_mode) {
    if (!debug_ext_info.intel_pt.intel_pt_supported || !msr_is_supported()) return 0;
    
    /* Allocate trace buffer */
    void* trace_buffer = kmalloc(1024 * 1024);  /* 1MB buffer */
    if (!trace_buffer) return 0;
    
    debug_ext_info.intel_pt.pt_output_base = virt_to_phys(trace_buffer);
    debug_ext_info.intel_pt.pt_output_mask = (1024 * 1024) - 1;
    
    /* Configure PT output */
    msr_write(MSR_IA32_RTIT_OUTPUT_BASE, debug_ext_info.intel_pt.pt_output_base);
    msr_write(MSR_IA32_RTIT_OUTPUT_MASK_PTRS, debug_ext_info.intel_pt.pt_output_mask);
    
    /* Configure PT control */
    u64 pt_ctl = RTIT_CTL_TRACEEN | RTIT_CTL_TSCEN | RTIT_CTL_BRANCHEN;
    if (user_mode) pt_ctl |= RTIT_CTL_USER;
    if (kernel_mode) pt_ctl |= RTIT_CTL_OS;
    
    msr_write(MSR_IA32_RTIT_CTL, pt_ctl);
    
    debug_ext_info.intel_pt.intel_pt_enabled = 1;
    debug_ext_info.intel_pt.pt_control = pt_ctl;
    debug_ext_info.last_trace_start = timer_get_ticks();
    debug_ext_info.total_trace_sessions++;
    
    return 1;
}

u8 cpu_debug_ext_disable_intel_pt(void) {
    if (!debug_ext_info.intel_pt.intel_pt_enabled || !msr_is_supported()) return 0;
    
    /* Disable PT */
    msr_write(MSR_IA32_RTIT_CTL, 0);
    
    /* Read final status */
    debug_ext_info.intel_pt.pt_status = msr_read(MSR_IA32_RTIT_STATUS);
    
    debug_ext_info.intel_pt.intel_pt_enabled = 0;
    debug_ext_info.last_trace_end = timer_get_ticks();
    
    if (debug_ext_info.last_trace_start > 0) {
        debug_ext_info.total_trace_time += (debug_ext_info.last_trace_end - debug_ext_info.last_trace_start);
    }
    
    return 1;
}

u8 cpu_debug_ext_enable_lbr(u64 select_mask) {
    if (!debug_ext_info.lbr.lbr_supported || !msr_is_supported()) return 0;
    
    /* Configure LBR selection */
    msr_write(MSR_LBR_SELECT, select_mask);
    debug_ext_info.lbr.lbr_select = select_mask;
    
    /* Enable LBR in DebugCtl MSR */
    u64 debugctl = msr_read(0x1D9);  /* MSR_IA32_DEBUGCTL */
    debugctl |= (1ULL << 0);  /* LBR */
    msr_write(0x1D9, debugctl);
    
    debug_ext_info.lbr.lbr_enabled = 1;
    debug_ext_info.last_trace_start = timer_get_ticks();
    debug_ext_info.total_trace_sessions++;
    
    return 1;
}

u8 cpu_debug_ext_disable_lbr(void) {
    if (!debug_ext_info.lbr.lbr_enabled || !msr_is_supported()) return 0;
    
    /* Disable LBR in DebugCtl MSR */
    u64 debugctl = msr_read(0x1D9);
    debugctl &= ~(1ULL << 0);
    msr_write(0x1D9, debugctl);
    
    debug_ext_info.lbr.lbr_enabled = 0;
    debug_ext_info.last_trace_end = timer_get_ticks();
    
    if (debug_ext_info.last_trace_start > 0) {
        debug_ext_info.total_trace_time += (debug_ext_info.last_trace_end - debug_ext_info.last_trace_start);
    }
    
    return 1;
}

void cpu_debug_ext_read_lbr_stack(void) {
    if (!debug_ext_info.lbr.lbr_enabled || !msr_is_supported()) return;
    
    /* Read TOS (Top of Stack) */
    debug_ext_info.lbr.lbr_tos = msr_read(MSR_LBR_TOS) & 0x1F;
    
    /* Read LBR stack */
    for (u8 i = 0; i < debug_ext_info.lbr.lbr_depth; i++) {
        debug_ext_info.lbr.lbr_stack[i].from_ip = msr_read(MSR_LBR_FROM_0 + i);
        debug_ext_info.lbr.lbr_stack[i].to_ip = msr_read(MSR_LBR_TO_0 + i);
        debug_ext_info.lbr.lbr_stack[i].info = msr_read(MSR_LBR_INFO_0 + i);
        debug_ext_info.lbr.lbr_stack[i].timestamp = timer_get_ticks();
    }
    
    debug_ext_info.lbr.lbr_records_captured++;
}

u8 cpu_debug_ext_enable_bts(void* buffer, u32 buffer_size) {
    if (!debug_ext_info.bts.bts_supported || !msr_is_supported()) return 0;
    
    u64 buffer_phys = virt_to_phys(buffer);
    
    /* Configure BTS buffer */
    msr_write(MSR_BTS_BUFFER_BASE, buffer_phys);
    msr_write(MSR_BTS_ABSOLUTE_MAXIMUM, buffer_phys + buffer_size);
    msr_write(MSR_BTS_INTERRUPT_THRESHOLD, buffer_phys + (buffer_size * 3 / 4));
    msr_write(MSR_BTS_INDEX, buffer_phys);
    
    /* Enable BTS in DebugCtl MSR */
    u64 debugctl = msr_read(0x1D9);
    debugctl |= (1ULL << 1) | (1ULL << 2);  /* BTS | BTINT */
    msr_write(0x1D9, debugctl);
    
    debug_ext_info.bts.bts_enabled = 1;
    debug_ext_info.bts.bts_buffer_base = buffer_phys;
    debug_ext_info.bts.bts_absolute_max = buffer_phys + buffer_size;
    debug_ext_info.bts.bts_threshold = buffer_phys + (buffer_size * 3 / 4);
    debug_ext_info.last_trace_start = timer_get_ticks();
    debug_ext_info.total_trace_sessions++;
    
    return 1;
}

u8 cpu_debug_ext_disable_bts(void) {
    if (!debug_ext_info.bts.bts_enabled || !msr_is_supported()) return 0;
    
    /* Disable BTS in DebugCtl MSR */
    u64 debugctl = msr_read(0x1D9);
    debugctl &= ~((1ULL << 1) | (1ULL << 2));
    msr_write(0x1D9, debugctl);
    
    /* Read final index */
    debug_ext_info.bts.bts_index = msr_read(MSR_BTS_INDEX);
    
    debug_ext_info.bts.bts_enabled = 0;
    debug_ext_info.last_trace_end = timer_get_ticks();
    
    if (debug_ext_info.last_trace_start > 0) {
        debug_ext_info.total_trace_time += (debug_ext_info.last_trace_end - debug_ext_info.last_trace_start);
    }
    
    return 1;
}

void cpu_debug_ext_handle_debug_interrupt(void) {
    debug_ext_info.debug_interrupts++;
    
    if (debug_ext_info.bts.bts_enabled) {
        debug_ext_info.bts.bts_buffer_overflows++;
    }
}

u8 cpu_debug_ext_is_intel_pt_supported(void) {
    return debug_ext_info.intel_pt.intel_pt_supported;
}

u8 cpu_debug_ext_is_intel_pt_enabled(void) {
    return debug_ext_info.intel_pt.intel_pt_enabled;
}

u8 cpu_debug_ext_is_lbr_supported(void) {
    return debug_ext_info.lbr.lbr_supported;
}

u8 cpu_debug_ext_is_lbr_enabled(void) {
    return debug_ext_info.lbr.lbr_enabled;
}

u8 cpu_debug_ext_is_bts_supported(void) {
    return debug_ext_info.bts.bts_supported;
}

u8 cpu_debug_ext_is_bts_enabled(void) {
    return debug_ext_info.bts.bts_enabled;
}

u8 cpu_debug_ext_get_lbr_depth(void) {
    return debug_ext_info.lbr.lbr_depth;
}

const lbr_entry_t* cpu_debug_ext_get_lbr_entry(u8 index) {
    if (index >= debug_ext_info.lbr.lbr_depth) return 0;
    return &debug_ext_info.lbr.lbr_stack[index];
}

u32 cpu_debug_ext_get_lbr_tos(void) {
    return debug_ext_info.lbr.lbr_tos;
}

u64 cpu_debug_ext_get_pt_output_base(void) {
    return debug_ext_info.intel_pt.pt_output_base;
}

u64 cpu_debug_ext_get_pt_status(void) {
    return debug_ext_info.intel_pt.pt_status;
}

u32 cpu_debug_ext_get_total_trace_sessions(void) {
    return debug_ext_info.total_trace_sessions;
}

u64 cpu_debug_ext_get_total_trace_time(void) {
    return debug_ext_info.total_trace_time;
}

u32 cpu_debug_ext_get_debug_interrupts(void) {
    return debug_ext_info.debug_interrupts;
}

u32 cpu_debug_ext_get_lbr_records_captured(void) {
    return debug_ext_info.lbr.lbr_records_captured;
}

u32 cpu_debug_ext_get_bts_records_captured(void) {
    return debug_ext_info.bts.bts_records_captured;
}

u32 cpu_debug_ext_get_bts_buffer_overflows(void) {
    return debug_ext_info.bts.bts_buffer_overflows;
}

void cpu_debug_ext_clear_statistics(void) {
    debug_ext_info.total_trace_sessions = 0;
    debug_ext_info.total_trace_time = 0;
    debug_ext_info.debug_interrupts = 0;
    debug_ext_info.lbr.lbr_records_captured = 0;
    debug_ext_info.bts.bts_records_captured = 0;
    debug_ext_info.bts.bts_buffer_overflows = 0;
    debug_ext_info.intel_pt.pt_packets_generated = 0;
}
