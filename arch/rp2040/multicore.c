/*
 * multicore.c – RP2040 dual-core Cortex-M0+ support
 */

#include "kernel/types.h"

#define SIO_BASE 0xD0000000UL
#define PSM_BASE 0x40010000UL

typedef volatile struct {
    u32 CPUID;
    u32 GPIO_IN;
    u32 GPIO_HI_IN;
    u32 _pad1;
    u32 GPIO_OUT;
    u32 GPIO_OUT_SET;
    u32 GPIO_OUT_CLR;
    u32 GPIO_OUT_XOR;
    u32 GPIO_OE;
    u32 GPIO_OE_SET;
    u32 GPIO_OE_CLR;
    u32 GPIO_OE_XOR;
    u32 GPIO_HI_OUT;
    u32 GPIO_HI_OUT_SET;
    u32 GPIO_HI_OUT_CLR;
    u32 GPIO_HI_OUT_XOR;
    u32 GPIO_HI_OE;
    u32 GPIO_HI_OE_SET;
    u32 GPIO_HI_OE_CLR;
    u32 GPIO_HI_OE_XOR;
    u32 FIFO_ST;
    u32 FIFO_WR;
    u32 FIFO_RD;
    u32 SPINLOCK_ST;
    u32 DIV_UDIVIDEND;
    u32 DIV_UDIVISOR;
    u32 DIV_SDIVIDEND;
    u32 DIV_SDIVISOR;
    u32 DIV_QUOTIENT;
    u32 DIV_REMAINDER;
    u32 DIV_CSR;
    u32 _pad2;
    u32 INTERP0_ACCUM0;
    u32 INTERP0_ACCUM1;
    u32 INTERP0_BASE0;
    u32 INTERP0_BASE1;
    u32 INTERP0_BASE2;
    u32 INTERP0_CTRL_LANE0;
    u32 INTERP0_CTRL_LANE1;
    u32 INTERP0_ACCUM0_ADD;
    u32 INTERP0_ACCUM1_ADD;
    u32 INTERP0_BASE_1AND0;
    u32 INTERP0_PEEK_LANE0;
    u32 INTERP0_PEEK_LANE1;
    u32 INTERP0_PEEK_FULL;
    u32 INTERP0_CTRL_LANE0_FULL;
    u32 INTERP0_CTRL_LANE1_FULL;
    u32 INTERP1_ACCUM0;
    u32 INTERP1_ACCUM1;
    u32 INTERP1_BASE0;
    u32 INTERP1_BASE1;
    u32 INTERP1_BASE2;
    u32 INTERP1_CTRL_LANE0;
    u32 INTERP1_CTRL_LANE1;
    u32 INTERP1_ACCUM0_ADD;
    u32 INTERP1_ACCUM1_ADD;
    u32 INTERP1_BASE_1AND0;
    u32 INTERP1_PEEK_LANE0;
    u32 INTERP1_PEEK_LANE1;
    u32 INTERP1_PEEK_FULL;
    u32 INTERP1_CTRL_LANE0_FULL;
    u32 INTERP1_CTRL_LANE1_FULL;
    u32 SPINLOCK[32];
} SIO_TypeDef;

static SIO_TypeDef* const SIO = (SIO_TypeDef*)SIO_BASE;

static volatile u32 core1_stack[1024] __attribute__((aligned(8)));
static void (*core1_entry)(void) = 0;

u8 multicore_get_core_num(void) {
    return SIO->CPUID;
}

void multicore_fifo_push(u32 data) {
    while (SIO->FIFO_ST & (1<<1)); /* Wait for FIFO not full */
    SIO->FIFO_WR = data;
    __asm__ volatile("sev"); /* Send event to other core */
}

u32 multicore_fifo_pop(void) {
    while (SIO->FIFO_ST & (1<<0)); /* Wait for FIFO not empty */
    return SIO->FIFO_RD;
}

u8 multicore_fifo_rvalid(void) {
    return !(SIO->FIFO_ST & (1<<0));
}

u8 multicore_fifo_wready(void) {
    return !(SIO->FIFO_ST & (1<<1));
}

void multicore_fifo_drain(void) {
    while (multicore_fifo_rvalid()) {
        multicore_fifo_pop();
    }
}

void multicore_fifo_clear_irq(void) {
    SIO->FIFO_ST = 0xFF;
}

u32 multicore_lockout_start_timeout_us(u32 timeout_us) {
    u64 end_time = timer_us() + timeout_us;
    
    while (timer_us() < end_time) {
        u32 save = *(volatile u32*)(PSM_BASE + 0x0);
        *(volatile u32*)(PSM_BASE + 0x0) = save | (1<<1); /* PROC1 reset */
        
        if (*(volatile u32*)(PSM_BASE + 0x0) & (1<<1)) {
            return 1; /* Success */
        }
    }
    return 0; /* Timeout */
}

void multicore_lockout_end(void) {
    *(volatile u32*)(PSM_BASE + 0x4) = (1<<1); /* Release PROC1 reset */
}

static void core1_wrapper(void) {
    multicore_fifo_clear_irq();
    
    if (core1_entry) {
        core1_entry();
    }
    
    /* Core1 finished, signal core0 */
    multicore_fifo_push(0xDEADBEEF);
    
    while (1) {
        __asm__ volatile("wfi");
    }
}

u8 multicore_launch_core1(void (*entry)(void)) {
    if (multicore_get_core_num() != 0) return 0;
    
    core1_entry = entry;
    
    /* Reset core1 */
    if (!multicore_lockout_start_timeout_us(1000000)) {
        return 0;
    }
    
    /* Clear FIFOs */
    multicore_fifo_drain();
    multicore_fifo_clear_irq();
    
    /* Set up core1 stack */
    u32 stack_top = (u32)&core1_stack[1024];
    
    /* Send boot sequence to core1 */
    u32 cmd_sequence[] = {
        0,
        0,
        1,
        0xE25A2D42,  /* Vector table magic */
        (u32)core1_wrapper,
        stack_top,
        1
    };
    
    for (u8 i = 0; i < 7; i++) {
        multicore_fifo_push(cmd_sequence[i]);
    }
    
    /* Release core1 reset */
    multicore_lockout_end();
    
    /* Wait for core1 to acknowledge */
    u32 response = multicore_fifo_pop();
    return (response == cmd_sequence[4]);
}

void multicore_reset_core1(void) {
    multicore_lockout_start_timeout_us(1000000);
    multicore_fifo_drain();
    multicore_fifo_clear_irq();
}

/* Spinlock functions */
u32 multicore_spin_lock_get(u8 lock_num) {
    if (lock_num >= 32) return 0;
    return SIO->SPINLOCK[lock_num];
}

void multicore_spin_lock_release(u8 lock_num) {
    if (lock_num < 32) {
        SIO->SPINLOCK[lock_num] = 0;
    }
}

u8 multicore_spin_lock_try(u8 lock_num) {
    if (lock_num >= 32) return 0;
    return (SIO->SPINLOCK[lock_num] != 0);
}

void multicore_spin_lock_blocking(u8 lock_num) {
    while (!multicore_spin_lock_try(lock_num)) {
        __asm__ volatile("wfe");
    }
}

/* Hardware divider */
s32 multicore_hw_divider_s32(s32 dividend, s32 divisor) {
    SIO->DIV_SDIVIDEND = dividend;
    SIO->DIV_SDIVISOR = divisor;
    
    /* Wait 8 cycles for result */
    __asm__ volatile("nop; nop; nop; nop; nop; nop; nop; nop");
    
    return SIO->DIV_QUOTIENT;
}

u32 multicore_hw_divider_u32(u32 dividend, u32 divisor) {
    SIO->DIV_UDIVIDEND = dividend;
    SIO->DIV_UDIVISOR = divisor;
    
    /* Wait 8 cycles for result */
    __asm__ volatile("nop; nop; nop; nop; nop; nop; nop; nop");
    
    return SIO->DIV_QUOTIENT;
}

s32 multicore_hw_remainder_s32(s32 dividend, s32 divisor) {
    SIO->DIV_SDIVIDEND = dividend;
    SIO->DIV_SDIVISOR = divisor;
    
    /* Wait 8 cycles for result */
    __asm__ volatile("nop; nop; nop; nop; nop; nop; nop; nop");
    
    return SIO->DIV_REMAINDER;
}

/* Interpolator functions */
void multicore_interp_config(u8 interp, u8 lane, u32 ctrl) {
    if (interp == 0) {
        if (lane == 0) SIO->INTERP0_CTRL_LANE0 = ctrl;
        else SIO->INTERP0_CTRL_LANE1 = ctrl;
    } else {
        if (lane == 0) SIO->INTERP1_CTRL_LANE0 = ctrl;
        else SIO->INTERP1_CTRL_LANE1 = ctrl;
    }
}

u32 multicore_interp_pop(u8 interp, u8 lane) {
    if (interp == 0) {
        return (lane == 0) ? SIO->INTERP0_PEEK_LANE0 : SIO->INTERP0_PEEK_LANE1;
    } else {
        return (lane == 0) ? SIO->INTERP1_PEEK_LANE0 : SIO->INTERP1_PEEK_LANE1;
    }
}
