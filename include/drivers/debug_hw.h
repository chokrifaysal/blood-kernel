/*
 * debug_hw.h – x86 hardware debugging (breakpoints/watchpoints/tracing)
 */

#ifndef DEBUG_HW_H
#define DEBUG_HW_H

#include "kernel/types.h"

/* Breakpoint types */
#define BP_TYPE_EXECUTION       0
#define BP_TYPE_WRITE           1
#define BP_TYPE_IO              2
#define BP_TYPE_ACCESS          3

/* Breakpoint lengths */
#define BP_LEN_1_BYTE           0
#define BP_LEN_2_BYTE           1
#define BP_LEN_8_BYTE           2
#define BP_LEN_4_BYTE           3

typedef struct {
    u64 address;
    u8 type;
    u8 length;
    u8 enabled;
    u8 global;
    u32 hit_count;
} hardware_breakpoint_t;

/* Core functions */
void debug_hw_init(void);

/* Support detection */
u8 debug_hw_is_supported(void);
u8 debug_hw_get_num_breakpoints(void);

/* Breakpoint management */
u8 debug_hw_set_breakpoint(u8 index, u64 address, u8 type, u8 length);
u8 debug_hw_clear_breakpoint(u8 index);
const hardware_breakpoint_t* debug_hw_get_breakpoint(u8 index);
void debug_hw_clear_all_breakpoints(void);
u8 debug_hw_find_free_breakpoint(void);

/* Convenience functions */
u8 debug_hw_set_execution_breakpoint(u8 index, u64 address);
u8 debug_hw_set_write_watchpoint(u8 index, u64 address, u8 size);
u8 debug_hw_set_access_watchpoint(u8 index, u64 address, u8 size);
u8 debug_hw_set_io_breakpoint(u8 index, u16 port);

/* Single stepping */
void debug_hw_enable_single_step(void);
void debug_hw_disable_single_step(void);
u8 debug_hw_is_single_step_enabled(void);

/* Branch tracing */
void debug_hw_enable_branch_trace(void);
void debug_hw_disable_branch_trace(void);
u8 debug_hw_is_branch_trace_enabled(void);

/* Exception handling */
void debug_hw_handle_exception(u64 error_code, u64 rip);

/* Statistics */
u32 debug_hw_get_debug_exception_count(void);
u32 debug_hw_get_single_step_count(void);
u32 debug_hw_get_branch_trace_count(void);
u32 debug_hw_get_breakpoint_hit_count(u8 index);
void debug_hw_reset_statistics(void);

/* Status */
u64 debug_hw_get_dr6_status(void);
u64 debug_hw_get_dr7_control(void);
u8 debug_hw_is_breakpoint_hit(u8 index);

/* General detect */
void debug_hw_enable_general_detect(void);
void debug_hw_disable_general_detect(void);

#endif
