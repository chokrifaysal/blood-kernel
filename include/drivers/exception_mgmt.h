/*
 * exception_mgmt.h – x86 CPU exception handling (fault injection/recovery)
 */

#ifndef EXCEPTION_MGMT_H
#define EXCEPTION_MGMT_H

#include "kernel/types.h"

/* Exception vectors */
#define EXCEPTION_DIVIDE_ERROR          0
#define EXCEPTION_DEBUG                 1
#define EXCEPTION_NMI                   2
#define EXCEPTION_BREAKPOINT            3
#define EXCEPTION_OVERFLOW              4
#define EXCEPTION_BOUND_RANGE           5
#define EXCEPTION_INVALID_OPCODE        6
#define EXCEPTION_DEVICE_NOT_AVAILABLE  7
#define EXCEPTION_DOUBLE_FAULT          8
#define EXCEPTION_COPROCESSOR_OVERRUN   9
#define EXCEPTION_INVALID_TSS           10
#define EXCEPTION_SEGMENT_NOT_PRESENT   11
#define EXCEPTION_STACK_FAULT           12
#define EXCEPTION_GENERAL_PROTECTION    13
#define EXCEPTION_PAGE_FAULT            14
#define EXCEPTION_X87_FPU_ERROR         16
#define EXCEPTION_ALIGNMENT_CHECK       17
#define EXCEPTION_MACHINE_CHECK         18
#define EXCEPTION_SIMD_FP_EXCEPTION     19
#define EXCEPTION_VIRTUALIZATION        20
#define EXCEPTION_CONTROL_PROTECTION    21

/* Exception handling modes */
#define EXCEPTION_MODE_IGNORE           0
#define EXCEPTION_MODE_LOG              1
#define EXCEPTION_MODE_RECOVER          2
#define EXCEPTION_MODE_PANIC            3

typedef struct {
    u32 vector;
    u64 error_code;
    u64 fault_address;
    u64 instruction_pointer;
    u64 stack_pointer;
    u64 flags;
    u32 cpu_id;
    u64 timestamp;
    u8 recoverable;
    u8 handled;
} exception_record_t;

typedef struct {
    u32 vector;
    u32 count;
    u8 mode;
    u8 enabled;
    u64 last_timestamp;
    u64 total_recovery_time;
} exception_stats_t;

/* Core functions */
void exception_mgmt_init(void);

/* Support detection */
u8 exception_mgmt_is_supported(void);

/* Exception handling */
void exception_mgmt_handle_exception(u32 vector, u64 error_code, u64 fault_address, u64* rip, u64* rsp, u64 rflags);

/* Statistics */
u32 exception_mgmt_get_total_exceptions(void);
u32 exception_mgmt_get_recovered_exceptions(void);
u32 exception_mgmt_get_unhandled_exceptions(void);
const exception_stats_t* exception_mgmt_get_stats(u32 vector);

/* Records */
const exception_record_t* exception_mgmt_get_record(u32 index);
u32 exception_mgmt_get_record_count(void);

/* Configuration */
void exception_mgmt_set_mode(u32 vector, u8 mode);
u8 exception_mgmt_get_mode(u32 vector);

/* Recovery control */
void exception_mgmt_enable_recovery(void);
void exception_mgmt_disable_recovery(void);
u8 exception_mgmt_is_recovery_enabled(void);

/* Logging control */
void exception_mgmt_enable_logging(void);
void exception_mgmt_disable_logging(void);
u8 exception_mgmt_is_logging_enabled(void);

/* Fault injection */
void exception_mgmt_inject_fault(u32 vector);
void exception_mgmt_enable_fault_injection(void);
void exception_mgmt_disable_fault_injection(void);
u8 exception_mgmt_is_fault_injection_enabled(void);

/* Special exceptions */
u64 exception_mgmt_get_last_double_fault_time(void);
u64 exception_mgmt_get_last_machine_check_time(void);
u32 exception_mgmt_get_nested_exception_count(void);

/* Utilities */
void exception_mgmt_clear_statistics(void);
const char* exception_mgmt_get_exception_name(u32 vector);
u8 exception_mgmt_is_exception_recoverable(u32 vector);
void exception_mgmt_enable_exception(u32 vector);
void exception_mgmt_disable_exception(u32 vector);
u8 exception_mgmt_is_exception_enabled(u32 vector);

#endif
