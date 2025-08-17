/*
 * debug_hw.c – x86 hardware debugging (breakpoints/watchpoints/tracing)
 */

#include "kernel/types.h"

/* Debug register definitions */
#define DR0_REG                 0
#define DR1_REG                 1
#define DR2_REG                 2
#define DR3_REG                 3
#define DR6_REG                 6
#define DR7_REG                 7

/* DR7 control bits */
#define DR7_L0                  (1 << 0)
#define DR7_G0                  (1 << 1)
#define DR7_L1                  (1 << 2)
#define DR7_G1                  (1 << 3)
#define DR7_L2                  (1 << 4)
#define DR7_G2                  (1 << 5)
#define DR7_L3                  (1 << 6)
#define DR7_G3                  (1 << 7)
#define DR7_LE                  (1 << 8)
#define DR7_GE                  (1 << 9)
#define DR7_GD                  (1 << 13)

/* DR7 condition codes */
#define DR7_RW_EXEC             0x0
#define DR7_RW_WRITE            0x1
#define DR7_RW_IO               0x2
#define DR7_RW_RW               0x3

/* DR7 length codes */
#define DR7_LEN_1               0x0
#define DR7_LEN_2               0x1
#define DR7_LEN_8               0x2
#define DR7_LEN_4               0x3

/* DR6 status bits */
#define DR6_B0                  (1 << 0)
#define DR6_B1                  (1 << 1)
#define DR6_B2                  (1 << 2)
#define DR6_B3                  (1 << 3)
#define DR6_BD                  (1 << 13)
#define DR6_BS                  (1 << 14)
#define DR6_BT                  (1 << 15)

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

typedef struct {
    u8 debug_hw_supported;
    u8 num_breakpoints;
    u8 single_step_enabled;
    u8 branch_trace_enabled;
    hardware_breakpoint_t breakpoints[4];
    u64 dr6_status;
    u64 dr7_control;
    u32 debug_exception_count;
    u32 single_step_count;
    u32 branch_trace_count;
} debug_hw_info_t;

static debug_hw_info_t debug_hw_info;

static u64 debug_hw_read_dr(u8 reg) {
    u64 value = 0;
    
    switch (reg) {
        case DR0_REG:
            __asm__ volatile("mov %%dr0, %0" : "=r"(value));
            break;
        case DR1_REG:
            __asm__ volatile("mov %%dr1, %0" : "=r"(value));
            break;
        case DR2_REG:
            __asm__ volatile("mov %%dr2, %0" : "=r"(value));
            break;
        case DR3_REG:
            __asm__ volatile("mov %%dr3, %0" : "=r"(value));
            break;
        case DR6_REG:
            __asm__ volatile("mov %%dr6, %0" : "=r"(value));
            break;
        case DR7_REG:
            __asm__ volatile("mov %%dr7, %0" : "=r"(value));
            break;
    }
    
    return value;
}

static void debug_hw_write_dr(u8 reg, u64 value) {
    switch (reg) {
        case DR0_REG:
            __asm__ volatile("mov %0, %%dr0" : : "r"(value));
            break;
        case DR1_REG:
            __asm__ volatile("mov %0, %%dr1" : : "r"(value));
            break;
        case DR2_REG:
            __asm__ volatile("mov %0, %%dr2" : : "r"(value));
            break;
        case DR3_REG:
            __asm__ volatile("mov %0, %%dr3" : : "r"(value));
            break;
        case DR6_REG:
            __asm__ volatile("mov %0, %%dr6" : : "r"(value));
            break;
        case DR7_REG:
            __asm__ volatile("mov %0, %%dr7" : : "r"(value));
            break;
    }
}

void debug_hw_init(void) {
    debug_hw_info.debug_hw_supported = 1;
    debug_hw_info.num_breakpoints = 4;
    debug_hw_info.single_step_enabled = 0;
    debug_hw_info.branch_trace_enabled = 0;
    debug_hw_info.debug_exception_count = 0;
    debug_hw_info.single_step_count = 0;
    debug_hw_info.branch_trace_count = 0;
    
    for (u8 i = 0; i < 4; i++) {
        debug_hw_info.breakpoints[i].address = 0;
        debug_hw_info.breakpoints[i].type = BP_TYPE_EXECUTION;
        debug_hw_info.breakpoints[i].length = BP_LEN_1_BYTE;
        debug_hw_info.breakpoints[i].enabled = 0;
        debug_hw_info.breakpoints[i].global = 0;
        debug_hw_info.breakpoints[i].hit_count = 0;
    }
    
    debug_hw_write_dr(DR6_REG, 0);
    debug_hw_write_dr(DR7_REG, 0x400);
    
    debug_hw_info.dr6_status = 0;
    debug_hw_info.dr7_control = 0x400;
}

u8 debug_hw_is_supported(void) {
    return debug_hw_info.debug_hw_supported;
}

u8 debug_hw_get_num_breakpoints(void) {
    return debug_hw_info.num_breakpoints;
}

u8 debug_hw_set_breakpoint(u8 index, u64 address, u8 type, u8 length) {
    if (index >= 4) return 0;
    
    hardware_breakpoint_t* bp = &debug_hw_info.breakpoints[index];
    
    bp->address = address;
    bp->type = type;
    bp->length = length;
    bp->enabled = 1;
    bp->global = 1;
    bp->hit_count = 0;
    
    debug_hw_write_dr(index, address);
    
    u64 dr7 = debug_hw_read_dr(DR7_REG);
    
    dr7 |= (1 << (index * 2 + 1));
    
    u32 rw_shift = 16 + (index * 4);
    u32 len_shift = 18 + (index * 4);
    
    dr7 &= ~(3 << rw_shift);
    dr7 &= ~(3 << len_shift);
    
    dr7 |= (type << rw_shift);
    dr7 |= (length << len_shift);
    
    debug_hw_write_dr(DR7_REG, dr7);
    debug_hw_info.dr7_control = dr7;
    
    return 1;
}

u8 debug_hw_clear_breakpoint(u8 index) {
    if (index >= 4) return 0;
    
    hardware_breakpoint_t* bp = &debug_hw_info.breakpoints[index];
    
    bp->enabled = 0;
    bp->address = 0;
    
    debug_hw_write_dr(index, 0);
    
    u64 dr7 = debug_hw_read_dr(DR7_REG);
    dr7 &= ~(3 << (index * 2));
    
    debug_hw_write_dr(DR7_REG, dr7);
    debug_hw_info.dr7_control = dr7;
    
    return 1;
}

const hardware_breakpoint_t* debug_hw_get_breakpoint(u8 index) {
    if (index >= 4) return 0;
    return &debug_hw_info.breakpoints[index];
}

void debug_hw_enable_single_step(void) {
    u32 eflags;
    __asm__ volatile("pushf; pop %0" : "=r"(eflags));
    eflags |= (1 << 8);
    __asm__ volatile("push %0; popf" : : "r"(eflags));
    
    debug_hw_info.single_step_enabled = 1;
}

void debug_hw_disable_single_step(void) {
    u32 eflags;
    __asm__ volatile("pushf; pop %0" : "=r"(eflags));
    eflags &= ~(1 << 8);
    __asm__ volatile("push %0; popf" : : "r"(eflags));
    
    debug_hw_info.single_step_enabled = 0;
}

u8 debug_hw_is_single_step_enabled(void) {
    return debug_hw_info.single_step_enabled;
}

void debug_hw_enable_branch_trace(void) {
    u64 dr7 = debug_hw_read_dr(DR7_REG);
    dr7 |= (1 << 15);
    debug_hw_write_dr(DR7_REG, dr7);
    
    debug_hw_info.branch_trace_enabled = 1;
    debug_hw_info.dr7_control = dr7;
}

void debug_hw_disable_branch_trace(void) {
    u64 dr7 = debug_hw_read_dr(DR7_REG);
    dr7 &= ~(1 << 15);
    debug_hw_write_dr(DR7_REG, dr7);
    
    debug_hw_info.branch_trace_enabled = 0;
    debug_hw_info.dr7_control = dr7;
}

u8 debug_hw_is_branch_trace_enabled(void) {
    return debug_hw_info.branch_trace_enabled;
}

void debug_hw_handle_exception(u64 error_code, u64 rip) {
    debug_hw_info.debug_exception_count++;
    
    u64 dr6 = debug_hw_read_dr(DR6_REG);
    debug_hw_info.dr6_status = dr6;
    
    if (dr6 & DR6_BS) {
        debug_hw_info.single_step_count++;
    }
    
    if (dr6 & DR6_BT) {
        debug_hw_info.branch_trace_count++;
    }
    
    for (u8 i = 0; i < 4; i++) {
        if (dr6 & (1 << i)) {
            debug_hw_info.breakpoints[i].hit_count++;
        }
    }
    
    debug_hw_write_dr(DR6_REG, 0);
}

u32 debug_hw_get_debug_exception_count(void) {
    return debug_hw_info.debug_exception_count;
}

u32 debug_hw_get_single_step_count(void) {
    return debug_hw_info.single_step_count;
}

u32 debug_hw_get_branch_trace_count(void) {
    return debug_hw_info.branch_trace_count;
}

u64 debug_hw_get_dr6_status(void) {
    return debug_hw_info.dr6_status;
}

u64 debug_hw_get_dr7_control(void) {
    return debug_hw_info.dr7_control;
}

u8 debug_hw_set_execution_breakpoint(u8 index, u64 address) {
    return debug_hw_set_breakpoint(index, address, BP_TYPE_EXECUTION, BP_LEN_1_BYTE);
}

u8 debug_hw_set_write_watchpoint(u8 index, u64 address, u8 size) {
    u8 length;
    switch (size) {
        case 1: length = BP_LEN_1_BYTE; break;
        case 2: length = BP_LEN_2_BYTE; break;
        case 4: length = BP_LEN_4_BYTE; break;
        case 8: length = BP_LEN_8_BYTE; break;
        default: return 0;
    }
    
    return debug_hw_set_breakpoint(index, address, BP_TYPE_WRITE, length);
}

u8 debug_hw_set_access_watchpoint(u8 index, u64 address, u8 size) {
    u8 length;
    switch (size) {
        case 1: length = BP_LEN_1_BYTE; break;
        case 2: length = BP_LEN_2_BYTE; break;
        case 4: length = BP_LEN_4_BYTE; break;
        case 8: length = BP_LEN_8_BYTE; break;
        default: return 0;
    }
    
    return debug_hw_set_breakpoint(index, address, BP_TYPE_ACCESS, length);
}

u8 debug_hw_set_io_breakpoint(u8 index, u16 port) {
    return debug_hw_set_breakpoint(index, port, BP_TYPE_IO, BP_LEN_1_BYTE);
}

void debug_hw_clear_all_breakpoints(void) {
    for (u8 i = 0; i < 4; i++) {
        debug_hw_clear_breakpoint(i);
    }
}

u8 debug_hw_find_free_breakpoint(void) {
    for (u8 i = 0; i < 4; i++) {
        if (!debug_hw_info.breakpoints[i].enabled) {
            return i;
        }
    }
    return 0xFF;
}

u32 debug_hw_get_breakpoint_hit_count(u8 index) {
    if (index >= 4) return 0;
    return debug_hw_info.breakpoints[index].hit_count;
}

void debug_hw_reset_statistics(void) {
    debug_hw_info.debug_exception_count = 0;
    debug_hw_info.single_step_count = 0;
    debug_hw_info.branch_trace_count = 0;
    
    for (u8 i = 0; i < 4; i++) {
        debug_hw_info.breakpoints[i].hit_count = 0;
    }
}

u8 debug_hw_is_breakpoint_hit(u8 index) {
    if (index >= 4) return 0;
    return (debug_hw_info.dr6_status & (1 << index)) != 0;
}

void debug_hw_enable_general_detect(void) {
    u64 dr7 = debug_hw_read_dr(DR7_REG);
    dr7 |= DR7_GD;
    debug_hw_write_dr(DR7_REG, dr7);
    debug_hw_info.dr7_control = dr7;
}

void debug_hw_disable_general_detect(void) {
    u64 dr7 = debug_hw_read_dr(DR7_REG);
    dr7 &= ~DR7_GD;
    debug_hw_write_dr(DR7_REG, dr7);
    debug_hw_info.dr7_control = dr7;
}
