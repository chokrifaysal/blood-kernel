/*
 * exception_mgmt.c – x86 CPU exception handling (fault injection/recovery)
 */

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

/* Page fault error codes */
#define PF_PRESENT                      (1 << 0)
#define PF_WRITE                        (1 << 1)
#define PF_USER                         (1 << 2)
#define PF_RESERVED                     (1 << 3)
#define PF_INSTRUCTION                  (1 << 4)
#define PF_PROTECTION_KEY               (1 << 5)
#define PF_SHADOW_STACK                 (1 << 6)

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

typedef struct {
    u8 exception_mgmt_supported;
    u8 fault_injection_enabled;
    u8 recovery_enabled;
    u8 logging_enabled;
    u32 total_exceptions;
    u32 recovered_exceptions;
    u32 unhandled_exceptions;
    exception_stats_t stats[32];
    exception_record_t records[256];
    u32 record_index;
    u64 last_double_fault;
    u64 last_machine_check;
    u32 nested_exception_count;
} exception_mgmt_info_t;

static exception_mgmt_info_t exception_mgmt_info;

extern u64 timer_get_ticks(void);
extern u32 cpu_get_current_id(void);

static void exception_mgmt_log_exception(u32 vector, u64 error_code, u64 fault_address, u64 rip, u64 rsp, u64 rflags) {
    if (!exception_mgmt_info.logging_enabled) return;
    
    exception_record_t* record = &exception_mgmt_info.records[exception_mgmt_info.record_index];
    
    record->vector = vector;
    record->error_code = error_code;
    record->fault_address = fault_address;
    record->instruction_pointer = rip;
    record->stack_pointer = rsp;
    record->flags = rflags;
    record->cpu_id = cpu_get_current_id();
    record->timestamp = timer_get_ticks();
    record->recoverable = 0;
    record->handled = 0;
    
    exception_mgmt_info.record_index = (exception_mgmt_info.record_index + 1) % 256;
    exception_mgmt_info.total_exceptions++;
    
    if (vector < 32) {
        exception_mgmt_info.stats[vector].count++;
        exception_mgmt_info.stats[vector].last_timestamp = record->timestamp;
    }
}

static u8 exception_mgmt_attempt_recovery(u32 vector, u64 error_code, u64 fault_address, u64* rip, u64* rsp) {
    if (!exception_mgmt_info.recovery_enabled) return 0;
    
    switch (vector) {
        case EXCEPTION_PAGE_FAULT:
            if (error_code & PF_PRESENT) {
                if (error_code & PF_WRITE) {
                    return 0;
                }
                
                if (fault_address >= 0x1000 && fault_address < 0x100000) {
                    return 1;
                }
            }
            break;
            
        case EXCEPTION_GENERAL_PROTECTION:
            if (error_code == 0) {
                *rip += 1;
                return 1;
            }
            break;
            
        case EXCEPTION_INVALID_OPCODE:
            *rip += 1;
            return 1;
            
        case EXCEPTION_DIVIDE_ERROR:
            return 0;
            
        case EXCEPTION_ALIGNMENT_CHECK:
            if (error_code == 0) {
                *rip += 1;
                return 1;
            }
            break;
    }
    
    return 0;
}

void exception_mgmt_init(void) {
    exception_mgmt_info.exception_mgmt_supported = 1;
    exception_mgmt_info.fault_injection_enabled = 0;
    exception_mgmt_info.recovery_enabled = 1;
    exception_mgmt_info.logging_enabled = 1;
    exception_mgmt_info.total_exceptions = 0;
    exception_mgmt_info.recovered_exceptions = 0;
    exception_mgmt_info.unhandled_exceptions = 0;
    exception_mgmt_info.record_index = 0;
    exception_mgmt_info.last_double_fault = 0;
    exception_mgmt_info.last_machine_check = 0;
    exception_mgmt_info.nested_exception_count = 0;
    
    for (u32 i = 0; i < 32; i++) {
        exception_mgmt_info.stats[i].vector = i;
        exception_mgmt_info.stats[i].count = 0;
        exception_mgmt_info.stats[i].mode = EXCEPTION_MODE_LOG;
        exception_mgmt_info.stats[i].enabled = 1;
        exception_mgmt_info.stats[i].last_timestamp = 0;
        exception_mgmt_info.stats[i].total_recovery_time = 0;
    }
    
    for (u32 i = 0; i < 256; i++) {
        exception_mgmt_info.records[i].vector = 0;
        exception_mgmt_info.records[i].handled = 0;
    }
    
    exception_mgmt_info.stats[EXCEPTION_DOUBLE_FAULT].mode = EXCEPTION_MODE_PANIC;
    exception_mgmt_info.stats[EXCEPTION_MACHINE_CHECK].mode = EXCEPTION_MODE_PANIC;
}

u8 exception_mgmt_is_supported(void) {
    return exception_mgmt_info.exception_mgmt_supported;
}

void exception_mgmt_handle_exception(u32 vector, u64 error_code, u64 fault_address, u64* rip, u64* rsp, u64 rflags) {
    u64 start_time = timer_get_ticks();
    
    exception_mgmt_log_exception(vector, error_code, fault_address, *rip, *rsp, rflags);
    
    if (vector == EXCEPTION_DOUBLE_FAULT) {
        exception_mgmt_info.last_double_fault = start_time;
        exception_mgmt_info.nested_exception_count++;
        return;
    }
    
    if (vector == EXCEPTION_MACHINE_CHECK) {
        exception_mgmt_info.last_machine_check = start_time;
        return;
    }
    
    if (vector < 32 && exception_mgmt_info.stats[vector].mode == EXCEPTION_MODE_RECOVER) {
        if (exception_mgmt_attempt_recovery(vector, error_code, fault_address, rip, rsp)) {
            exception_mgmt_info.recovered_exceptions++;
            exception_mgmt_info.records[exception_mgmt_info.record_index - 1].recoverable = 1;
            exception_mgmt_info.records[exception_mgmt_info.record_index - 1].handled = 1;
            
            u64 recovery_time = timer_get_ticks() - start_time;
            exception_mgmt_info.stats[vector].total_recovery_time += recovery_time;
            return;
        }
    }
    
    exception_mgmt_info.unhandled_exceptions++;
}

u32 exception_mgmt_get_total_exceptions(void) {
    return exception_mgmt_info.total_exceptions;
}

u32 exception_mgmt_get_recovered_exceptions(void) {
    return exception_mgmt_info.recovered_exceptions;
}

u32 exception_mgmt_get_unhandled_exceptions(void) {
    return exception_mgmt_info.unhandled_exceptions;
}

const exception_stats_t* exception_mgmt_get_stats(u32 vector) {
    if (vector >= 32) return 0;
    return &exception_mgmt_info.stats[vector];
}

const exception_record_t* exception_mgmt_get_record(u32 index) {
    if (index >= 256) return 0;
    return &exception_mgmt_info.records[index];
}

u32 exception_mgmt_get_record_count(void) {
    return exception_mgmt_info.total_exceptions > 256 ? 256 : exception_mgmt_info.total_exceptions;
}

void exception_mgmt_set_mode(u32 vector, u8 mode) {
    if (vector >= 32) return;
    exception_mgmt_info.stats[vector].mode = mode;
}

u8 exception_mgmt_get_mode(u32 vector) {
    if (vector >= 32) return EXCEPTION_MODE_IGNORE;
    return exception_mgmt_info.stats[vector].mode;
}

void exception_mgmt_enable_recovery(void) {
    exception_mgmt_info.recovery_enabled = 1;
}

void exception_mgmt_disable_recovery(void) {
    exception_mgmt_info.recovery_enabled = 0;
}

u8 exception_mgmt_is_recovery_enabled(void) {
    return exception_mgmt_info.recovery_enabled;
}

void exception_mgmt_enable_logging(void) {
    exception_mgmt_info.logging_enabled = 1;
}

void exception_mgmt_disable_logging(void) {
    exception_mgmt_info.logging_enabled = 0;
}

u8 exception_mgmt_is_logging_enabled(void) {
    return exception_mgmt_info.logging_enabled;
}

void exception_mgmt_inject_fault(u32 vector) {
    if (!exception_mgmt_info.fault_injection_enabled) return;
    
    switch (vector) {
        case EXCEPTION_DIVIDE_ERROR:
            __asm__ volatile("div %0" : : "r"(0) : "eax", "edx");
            break;
        case EXCEPTION_BREAKPOINT:
            __asm__ volatile("int3");
            break;
        case EXCEPTION_OVERFLOW:
            __asm__ volatile("into");
            break;
        case EXCEPTION_INVALID_OPCODE:
            __asm__ volatile(".byte 0x0F, 0xFF");
            break;
        default:
            break;
    }
}

void exception_mgmt_enable_fault_injection(void) {
    exception_mgmt_info.fault_injection_enabled = 1;
}

void exception_mgmt_disable_fault_injection(void) {
    exception_mgmt_info.fault_injection_enabled = 0;
}

u8 exception_mgmt_is_fault_injection_enabled(void) {
    return exception_mgmt_info.fault_injection_enabled;
}

u64 exception_mgmt_get_last_double_fault_time(void) {
    return exception_mgmt_info.last_double_fault;
}

u64 exception_mgmt_get_last_machine_check_time(void) {
    return exception_mgmt_info.last_machine_check;
}

u32 exception_mgmt_get_nested_exception_count(void) {
    return exception_mgmt_info.nested_exception_count;
}

void exception_mgmt_clear_statistics(void) {
    exception_mgmt_info.total_exceptions = 0;
    exception_mgmt_info.recovered_exceptions = 0;
    exception_mgmt_info.unhandled_exceptions = 0;
    exception_mgmt_info.nested_exception_count = 0;
    
    for (u32 i = 0; i < 32; i++) {
        exception_mgmt_info.stats[i].count = 0;
        exception_mgmt_info.stats[i].last_timestamp = 0;
        exception_mgmt_info.stats[i].total_recovery_time = 0;
    }
}

const char* exception_mgmt_get_exception_name(u32 vector) {
    switch (vector) {
        case EXCEPTION_DIVIDE_ERROR: return "Divide Error";
        case EXCEPTION_DEBUG: return "Debug";
        case EXCEPTION_NMI: return "NMI";
        case EXCEPTION_BREAKPOINT: return "Breakpoint";
        case EXCEPTION_OVERFLOW: return "Overflow";
        case EXCEPTION_BOUND_RANGE: return "Bound Range";
        case EXCEPTION_INVALID_OPCODE: return "Invalid Opcode";
        case EXCEPTION_DEVICE_NOT_AVAILABLE: return "Device Not Available";
        case EXCEPTION_DOUBLE_FAULT: return "Double Fault";
        case EXCEPTION_INVALID_TSS: return "Invalid TSS";
        case EXCEPTION_SEGMENT_NOT_PRESENT: return "Segment Not Present";
        case EXCEPTION_STACK_FAULT: return "Stack Fault";
        case EXCEPTION_GENERAL_PROTECTION: return "General Protection";
        case EXCEPTION_PAGE_FAULT: return "Page Fault";
        case EXCEPTION_X87_FPU_ERROR: return "x87 FPU Error";
        case EXCEPTION_ALIGNMENT_CHECK: return "Alignment Check";
        case EXCEPTION_MACHINE_CHECK: return "Machine Check";
        case EXCEPTION_SIMD_FP_EXCEPTION: return "SIMD FP Exception";
        case EXCEPTION_VIRTUALIZATION: return "Virtualization";
        case EXCEPTION_CONTROL_PROTECTION: return "Control Protection";
        default: return "Unknown";
    }
}

u8 exception_mgmt_is_exception_recoverable(u32 vector) {
    switch (vector) {
        case EXCEPTION_DIVIDE_ERROR:
        case EXCEPTION_DOUBLE_FAULT:
        case EXCEPTION_MACHINE_CHECK:
            return 0;
        case EXCEPTION_PAGE_FAULT:
        case EXCEPTION_GENERAL_PROTECTION:
        case EXCEPTION_INVALID_OPCODE:
        case EXCEPTION_ALIGNMENT_CHECK:
            return 1;
        default:
            return 0;
    }
}

void exception_mgmt_enable_exception(u32 vector) {
    if (vector >= 32) return;
    exception_mgmt_info.stats[vector].enabled = 1;
}

void exception_mgmt_disable_exception(u32 vector) {
    if (vector >= 32) return;
    exception_mgmt_info.stats[vector].enabled = 0;
}

u8 exception_mgmt_is_exception_enabled(u32 vector) {
    if (vector >= 32) return 0;
    return exception_mgmt_info.stats[vector].enabled;
}
