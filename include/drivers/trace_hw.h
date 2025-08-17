/*
 * trace_hw.h – x86 hardware instruction tracing (Intel PT/LBR extensions)
 */

#ifndef TRACE_HW_H
#define TRACE_HW_H

#include "kernel/types.h"

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

/* Core functions */
void trace_hw_init(void);

/* Support detection */
u8 trace_hw_is_supported(void);
u8 trace_hw_is_intel_pt_supported(void);
u8 trace_hw_is_bts_supported(void);

/* Intel Processor Trace (PT) */
u8 trace_hw_setup_pt_buffer(u64 buffer_base, u32 buffer_size);
u8 trace_hw_enable_intel_pt(u8 user_mode, u8 kernel_mode);
void trace_hw_disable_intel_pt(void);
u8 trace_hw_is_intel_pt_enabled(void);

/* Address filtering */
u8 trace_hw_set_address_filter(u8 range_index, u64 start_addr, u64 end_addr);
void trace_hw_clear_address_filter(u8 range_index);
u8 trace_hw_get_num_address_ranges(void);

/* Branch Trace Store (BTS) */
u8 trace_hw_setup_bts_buffer(u64 buffer_base, u32 buffer_size);
u8 trace_hw_enable_bts(void);
void trace_hw_disable_bts(void);
u8 trace_hw_is_bts_enabled(void);

/* BTS data access */
const bts_record_t* trace_hw_read_bts_record(u32 index);
u32 trace_hw_get_num_bts_records(void);
u32 trace_hw_get_bts_index(void);

/* Status and control */
u64 trace_hw_get_pt_status(void);
u64 trace_hw_get_pt_buffer_head(void);

/* Capabilities */
u8 trace_hw_is_cycle_accurate_supported(void);
u8 trace_hw_is_timing_packets_supported(void);
u8 trace_hw_is_power_event_trace_supported(void);

/* Buffer management */
const trace_buffer_t* trace_hw_get_pt_buffer_info(void);
const trace_buffer_t* trace_hw_get_bts_buffer_info(void);

/* Error handling */
void trace_hw_handle_overflow(void);
u32 trace_hw_get_packets_generated(void);
u32 trace_hw_get_buffer_overflows(void);
u32 trace_hw_get_trace_errors(void);

#endif
