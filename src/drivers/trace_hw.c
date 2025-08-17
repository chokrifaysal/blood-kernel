/*
 * trace_hw.c – x86 hardware instruction tracing (Intel PT/LBR extensions)
 */

#include "kernel/types.h"

/* Intel PT MSRs */
#define MSR_IA32_RTIT_CTL           0x570
#define MSR_IA32_RTIT_STATUS        0x571
#define MSR_IA32_RTIT_ADDR0_A       0x580
#define MSR_IA32_RTIT_ADDR0_B       0x581
#define MSR_IA32_RTIT_ADDR1_A       0x582
#define MSR_IA32_RTIT_ADDR1_B       0x583
#define MSR_IA32_RTIT_ADDR2_A       0x584
#define MSR_IA32_RTIT_ADDR2_B       0x585
#define MSR_IA32_RTIT_ADDR3_A       0x586
#define MSR_IA32_RTIT_ADDR3_B       0x587
#define MSR_IA32_RTIT_CR3_MATCH     0x572
#define MSR_IA32_RTIT_OUTPUT_BASE   0x560
#define MSR_IA32_RTIT_OUTPUT_MASK_PTRS 0x561

/* RTIT_CTL bits */
#define RTIT_CTL_TRACEEN            (1 << 0)
#define RTIT_CTL_CYCEN              (1 << 1)
#define RTIT_CTL_OS                 (1 << 2)
#define RTIT_CTL_USER               (1 << 3)
#define RTIT_CTL_PWREVT             (1 << 4)
#define RTIT_CTL_FUPONPTW           (1 << 5)
#define RTIT_CTL_FABRICEN           (1 << 6)
#define RTIT_CTL_CR3FILTER          (1 << 7)
#define RTIT_CTL_TOPA               (1 << 8)
#define RTIT_CTL_MTCEN              (1 << 9)
#define RTIT_CTL_TSCEN              (1 << 10)
#define RTIT_CTL_DISRETC            (1 << 11)
#define RTIT_CTL_PTWEN              (1 << 12)
#define RTIT_CTL_BRANCHEN           (1 << 13)

/* RTIT_STATUS bits */
#define RTIT_STATUS_FILTEREN        (1 << 0)
#define RTIT_STATUS_CONTEXTEN       (1 << 1)
#define RTIT_STATUS_TRIGGEREN       (1 << 2)
#define RTIT_STATUS_ERROR           (1 << 4)
#define RTIT_STATUS_STOPPED         (1 << 5)

/* BTS MSRs */
#define MSR_BTS_BUFFER_BASE         0x2A0
#define MSR_BTS_INDEX               0x2A1
#define MSR_BTS_ABSOLUTE_MAXIMUM    0x2A2
#define MSR_BTS_INTERRUPT_THRESHOLD 0x2A3

/* BTS record format */
typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 flags;
} bts_record_t;

/* Trace buffer management */
typedef struct {
    u64 base;
    u64 size;
    u64 head;
    u64 tail;
    u8 wrapped;
} trace_buffer_t;

typedef struct {
    u8 trace_hw_supported;
    u8 intel_pt_supported;
    u8 bts_supported;
    u8 lbr_extended_supported;
    u8 intel_pt_enabled;
    u8 bts_enabled;
    u8 trace_user_mode;
    u8 trace_kernel_mode;
    u8 cycle_accurate;
    u8 timing_packets;
    u8 mtc_packets;
    u8 branch_packets;
    u8 power_event_trace;
    u8 psb_frequency;
    u32 num_address_ranges;
    u64 address_ranges[8];
    trace_buffer_t pt_buffer;
    trace_buffer_t bts_buffer;
    u64 rtit_ctl;
    u64 rtit_status;
    u32 trace_packets_generated;
    u32 buffer_overflows;
    u32 trace_errors;
} trace_hw_info_t;

static trace_hw_info_t trace_hw_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void trace_hw_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    trace_hw_info.intel_pt_supported = (ebx & (1 << 25)) != 0;
    
    if (trace_hw_info.intel_pt_supported) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x14), "c"(0));
        
        trace_hw_info.cycle_accurate = (ebx & (1 << 1)) != 0;
        trace_hw_info.timing_packets = (ebx & (1 << 3)) != 0;
        trace_hw_info.mtc_packets = (ebx & (1 << 4)) != 0;
        trace_hw_info.branch_packets = (ebx & (1 << 5)) != 0;
        trace_hw_info.power_event_trace = (ebx & (1 << 6)) != 0;
        trace_hw_info.psb_frequency = (ebx & (1 << 7)) != 0;
        
        if (eax >= 1) {
            __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x14), "c"(1));
            trace_hw_info.num_address_ranges = eax & 0x7;
            if (trace_hw_info.num_address_ranges > 4) {
                trace_hw_info.num_address_ranges = 4;
            }
        }
    }
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    trace_hw_info.bts_supported = (edx & (1 << 21)) != 0;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    trace_hw_info.lbr_extended_supported = (ebx & (1 << 6)) != 0;
    
    if (trace_hw_info.intel_pt_supported || trace_hw_info.bts_supported) {
        trace_hw_info.trace_hw_supported = 1;
    }
}

void trace_hw_init(void) {
    trace_hw_info.trace_hw_supported = 0;
    trace_hw_info.intel_pt_supported = 0;
    trace_hw_info.bts_supported = 0;
    trace_hw_info.lbr_extended_supported = 0;
    trace_hw_info.intel_pt_enabled = 0;
    trace_hw_info.bts_enabled = 0;
    trace_hw_info.trace_user_mode = 0;
    trace_hw_info.trace_kernel_mode = 0;
    trace_hw_info.cycle_accurate = 0;
    trace_hw_info.timing_packets = 0;
    trace_hw_info.mtc_packets = 0;
    trace_hw_info.branch_packets = 0;
    trace_hw_info.power_event_trace = 0;
    trace_hw_info.psb_frequency = 0;
    trace_hw_info.num_address_ranges = 0;
    trace_hw_info.rtit_ctl = 0;
    trace_hw_info.rtit_status = 0;
    trace_hw_info.trace_packets_generated = 0;
    trace_hw_info.buffer_overflows = 0;
    trace_hw_info.trace_errors = 0;
    
    for (u8 i = 0; i < 8; i++) {
        trace_hw_info.address_ranges[i] = 0;
    }
    
    trace_hw_info.pt_buffer.base = 0;
    trace_hw_info.pt_buffer.size = 0;
    trace_hw_info.pt_buffer.head = 0;
    trace_hw_info.pt_buffer.tail = 0;
    trace_hw_info.pt_buffer.wrapped = 0;
    
    trace_hw_info.bts_buffer.base = 0;
    trace_hw_info.bts_buffer.size = 0;
    trace_hw_info.bts_buffer.head = 0;
    trace_hw_info.bts_buffer.tail = 0;
    trace_hw_info.bts_buffer.wrapped = 0;
    
    trace_hw_detect_capabilities();
}

u8 trace_hw_is_supported(void) {
    return trace_hw_info.trace_hw_supported;
}

u8 trace_hw_is_intel_pt_supported(void) {
    return trace_hw_info.intel_pt_supported;
}

u8 trace_hw_is_bts_supported(void) {
    return trace_hw_info.bts_supported;
}

u8 trace_hw_setup_pt_buffer(u64 buffer_base, u32 buffer_size) {
    if (!trace_hw_info.intel_pt_supported || !msr_is_supported()) return 0;
    
    trace_hw_info.pt_buffer.base = buffer_base;
    trace_hw_info.pt_buffer.size = buffer_size;
    trace_hw_info.pt_buffer.head = 0;
    trace_hw_info.pt_buffer.tail = 0;
    trace_hw_info.pt_buffer.wrapped = 0;
    
    msr_write(MSR_IA32_RTIT_OUTPUT_BASE, buffer_base);
    msr_write(MSR_IA32_RTIT_OUTPUT_MASK_PTRS, buffer_size - 1);
    
    return 1;
}

u8 trace_hw_enable_intel_pt(u8 user_mode, u8 kernel_mode) {
    if (!trace_hw_info.intel_pt_supported || !msr_is_supported()) return 0;
    
    u64 rtit_ctl = 0;
    
    rtit_ctl |= RTIT_CTL_TRACEEN;
    rtit_ctl |= RTIT_CTL_TOPA;
    rtit_ctl |= RTIT_CTL_TSCEN;
    rtit_ctl |= RTIT_CTL_BRANCHEN;
    
    if (user_mode) {
        rtit_ctl |= RTIT_CTL_USER;
        trace_hw_info.trace_user_mode = 1;
    }
    
    if (kernel_mode) {
        rtit_ctl |= RTIT_CTL_OS;
        trace_hw_info.trace_kernel_mode = 1;
    }
    
    if (trace_hw_info.cycle_accurate) {
        rtit_ctl |= RTIT_CTL_CYCEN;
    }
    
    if (trace_hw_info.mtc_packets) {
        rtit_ctl |= RTIT_CTL_MTCEN;
    }
    
    msr_write(MSR_IA32_RTIT_CTL, rtit_ctl);
    trace_hw_info.rtit_ctl = rtit_ctl;
    trace_hw_info.intel_pt_enabled = 1;
    
    return 1;
}

void trace_hw_disable_intel_pt(void) {
    if (!trace_hw_info.intel_pt_supported || !msr_is_supported()) return;
    
    msr_write(MSR_IA32_RTIT_CTL, 0);
    trace_hw_info.rtit_ctl = 0;
    trace_hw_info.intel_pt_enabled = 0;
}

u8 trace_hw_is_intel_pt_enabled(void) {
    return trace_hw_info.intel_pt_enabled;
}

u8 trace_hw_set_address_filter(u8 range_index, u64 start_addr, u64 end_addr) {
    if (!trace_hw_info.intel_pt_supported || !msr_is_supported()) return 0;
    if (range_index >= trace_hw_info.num_address_ranges) return 0;
    
    u32 addr_a_msr = MSR_IA32_RTIT_ADDR0_A + (range_index * 2);
    u32 addr_b_msr = MSR_IA32_RTIT_ADDR0_B + (range_index * 2);
    
    msr_write(addr_a_msr, start_addr);
    msr_write(addr_b_msr, end_addr);
    
    trace_hw_info.address_ranges[range_index * 2] = start_addr;
    trace_hw_info.address_ranges[range_index * 2 + 1] = end_addr;
    
    u64 rtit_ctl = msr_read(MSR_IA32_RTIT_CTL);
    rtit_ctl |= (1ULL << (32 + range_index));
    msr_write(MSR_IA32_RTIT_CTL, rtit_ctl);
    trace_hw_info.rtit_ctl = rtit_ctl;
    
    return 1;
}

void trace_hw_clear_address_filter(u8 range_index) {
    if (!trace_hw_info.intel_pt_supported || !msr_is_supported()) return;
    if (range_index >= trace_hw_info.num_address_ranges) return;
    
    u64 rtit_ctl = msr_read(MSR_IA32_RTIT_CTL);
    rtit_ctl &= ~(1ULL << (32 + range_index));
    msr_write(MSR_IA32_RTIT_CTL, rtit_ctl);
    trace_hw_info.rtit_ctl = rtit_ctl;
}

u8 trace_hw_setup_bts_buffer(u64 buffer_base, u32 buffer_size) {
    if (!trace_hw_info.bts_supported || !msr_is_supported()) return 0;
    
    trace_hw_info.bts_buffer.base = buffer_base;
    trace_hw_info.bts_buffer.size = buffer_size;
    trace_hw_info.bts_buffer.head = 0;
    trace_hw_info.bts_buffer.tail = 0;
    trace_hw_info.bts_buffer.wrapped = 0;
    
    msr_write(MSR_BTS_BUFFER_BASE, buffer_base);
    msr_write(MSR_BTS_ABSOLUTE_MAXIMUM, buffer_base + buffer_size);
    msr_write(MSR_BTS_INDEX, buffer_base);
    msr_write(MSR_BTS_INTERRUPT_THRESHOLD, buffer_base + buffer_size - 64);
    
    return 1;
}

u8 trace_hw_enable_bts(void) {
    if (!trace_hw_info.bts_supported || !msr_is_supported()) return 0;
    
    u64 debugctl = 0;
    debugctl |= (1 << 1);
    debugctl |= (1 << 2);
    
    extern void msr_write_debugctl(u64 value);
    msr_write_debugctl(debugctl);
    
    trace_hw_info.bts_enabled = 1;
    return 1;
}

void trace_hw_disable_bts(void) {
    if (!trace_hw_info.bts_supported || !msr_is_supported()) return;
    
    extern void msr_write_debugctl(u64 value);
    msr_write_debugctl(0);
    
    trace_hw_info.bts_enabled = 0;
}

u8 trace_hw_is_bts_enabled(void) {
    return trace_hw_info.bts_enabled;
}

u64 trace_hw_get_pt_status(void) {
    if (!trace_hw_info.intel_pt_supported || !msr_is_supported()) return 0;
    
    trace_hw_info.rtit_status = msr_read(MSR_IA32_RTIT_STATUS);
    return trace_hw_info.rtit_status;
}

u64 trace_hw_get_pt_buffer_head(void) {
    if (!trace_hw_info.intel_pt_supported || !msr_is_supported()) return 0;
    
    u64 mask_ptrs = msr_read(MSR_IA32_RTIT_OUTPUT_MASK_PTRS);
    trace_hw_info.pt_buffer.head = (mask_ptrs >> 32) & 0xFFFFFFFF;
    return trace_hw_info.pt_buffer.head;
}

u32 trace_hw_get_bts_index(void) {
    if (!trace_hw_info.bts_supported || !msr_is_supported()) return 0;
    
    u64 index = msr_read(MSR_BTS_INDEX);
    trace_hw_info.bts_buffer.head = index;
    return (u32)index;
}

const bts_record_t* trace_hw_read_bts_record(u32 index) {
    if (!trace_hw_info.bts_enabled) return 0;
    
    u64 record_addr = trace_hw_info.bts_buffer.base + (index * sizeof(bts_record_t));
    if (record_addr >= trace_hw_info.bts_buffer.base + trace_hw_info.bts_buffer.size) {
        return 0;
    }
    
    return (const bts_record_t*)record_addr;
}

u32 trace_hw_get_num_bts_records(void) {
    if (!trace_hw_info.bts_enabled) return 0;
    
    u64 current_index = msr_read(MSR_BTS_INDEX);
    u64 records = (current_index - trace_hw_info.bts_buffer.base) / sizeof(bts_record_t);
    return (u32)records;
}

void trace_hw_handle_overflow(void) {
    trace_hw_info.buffer_overflows++;
    
    if (trace_hw_info.intel_pt_enabled) {
        u64 status = msr_read(MSR_IA32_RTIT_STATUS);
        if (status & RTIT_STATUS_ERROR) {
            trace_hw_info.trace_errors++;
        }
        if (status & RTIT_STATUS_STOPPED) {
            trace_hw_info.intel_pt_enabled = 0;
        }
    }
}

u32 trace_hw_get_packets_generated(void) {
    return trace_hw_info.trace_packets_generated;
}

u32 trace_hw_get_buffer_overflows(void) {
    return trace_hw_info.buffer_overflows;
}

u32 trace_hw_get_trace_errors(void) {
    return trace_hw_info.trace_errors;
}

u8 trace_hw_get_num_address_ranges(void) {
    return trace_hw_info.num_address_ranges;
}

u8 trace_hw_is_cycle_accurate_supported(void) {
    return trace_hw_info.cycle_accurate;
}

u8 trace_hw_is_timing_packets_supported(void) {
    return trace_hw_info.timing_packets;
}

u8 trace_hw_is_power_event_trace_supported(void) {
    return trace_hw_info.power_event_trace;
}

const trace_buffer_t* trace_hw_get_pt_buffer_info(void) {
    return &trace_hw_info.pt_buffer;
}

const trace_buffer_t* trace_hw_get_bts_buffer_info(void) {
    return &trace_hw_info.bts_buffer;
}
