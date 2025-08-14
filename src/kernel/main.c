/*
 * main.c – universal entry for all 11 BLOOD_KERNEL architectures
 * SHH: boot banner, clock, uart, arch-dispatch, loader stub
 * No arch-specific includes here – each arch provides its own symbols via weak links
 */

#include "kernel/types.h"
#include "kernel/sched.h"
#include "kernel/timer.h"
#include "kernel/uart.h"
#include "kernel/flash.h"
#include "kernel/ipc.h"
#include "kernel/log.h"

static const char banner[] =
    "BLOOD_KERNEL v1.20 universal main\r\n"
    "Arch: %-12s  MCU: %-12s\r\n"
    "Boot: %-12s  Date: 2025-08-14\r\n"
    "Build: %s %s\r\n"
    "-------------------------------------------\r\n";

/* ----------------------------------------------------------
   1. Architecture identity strings (weak so each arch overrides)
   ---------------------------------------------------------- */
__attribute__((weak)) const char *arch_name(void) { return "unknown"; }
__attribute__((weak)) const char *mcu_name(void)  { return "unknown"; }
__attribute__((weak)) const char *boot_name(void) { return "cold";   }

/* ----------------------------------------------------------
   2. Universal low-level hooks (weak so arch can override)
   ---------------------------------------------------------- */
__attribute__((weak)) void uart_early_init(void) { /* nop */ }
__attribute__((weak)) void clock_init(void) { /* nop */ }
__attribute__((weak)) void gpio_init(void)  { /* nop */ }
__attribute__((weak)) void ipc_init(void)   { /* nop */ }

/* ----------------------------------------------------------
   3. Build-time stamp
   ---------------------------------------------------------- */
static const char *build_date(void) { return __DATE__; }
static const char *build_time(void) { return __TIME__; }

/* ----------------------------------------------------------
   4. Demo tasks
   ---------------------------------------------------------- */
static void idle_task(void) {
    while (1) {
        /* arch-specific idle hook */
        __asm__ volatile("nop");
    }
}

static void blink_task(void) {
    u32 cnt = 0;
    while (1) {
        /* arch-specific LED toggle */
        gpio_toggle(0);
        timer_delay(500);
        kprintf("blink %u\r\n", cnt++);
    }
}

static void log_task(void) {
    u8 buf[64];
    while (1) {
        if (log_pop(buf, sizeof(buf))) {
            uart_puts("LOG: ");
            uart_puts((char *)buf);
            uart_puts("\r\n");
        }
        timer_delay(100);
    }
}

/* ----------------------------------------------------------
   5. Runtime loader stub 
   ---------------------------------------------------------- */
#define MAX_MODULES 8
typedef struct {
    u32 addr;
    u32 size;
    char name[32];
} module_t;

static module_t modules[MAX_MODULES];
static u32 module_count = 0;

static void loader_add_module(u32 addr, u32 size, const char *name) {
    if (module_count >= MAX_MODULES) return;
    modules[module_count].addr = addr;
    modules[module_count].size = size;
    strncpy(modules[module_count].name, name, 31);
    module_count++;
}

static void loader_scan(void) {
    /* scan QSPI / flash for ELF headers */
    for (u32 addr = 0x08080000; addr < 0x08100000; addr += 0x1000) {
        u32 *hdr = (u32 *)addr;
        if (hdr[0] == 0x7F454C46) { /* ELF magic */
            loader_add_module(addr, 0x2000, "user_task");
        }
    }
}

static void loader_boot(void) {
    for (u32 i = 0; i < module_count; ++i) {
        kprintf("Loading %s @0x%08x (%u bytes)\r\n",
                modules[i].name, modules[i].addr, modules[i].size);
        /* simple jump stub */
        void (*entry)(void) = (void (*)(void))modules[i].addr;
        entry();
    }
}

/* ----------------------------------------------------------
   6. Universal kernel_main
   ---------------------------------------------------------- */
void kernel_main(void) {
    uart_early_init();
    clock_init();
    gpio_init();
    ipc_init();
    log_init();

    kprintf(banner,
            arch_name(), mcu_name(),
            boot_name(), build_date(), build_time());

    loader_scan();
    loader_boot();

    sched_init();
    task_create(idle_task, 0, 256);
    task_create(blink_task, 0, 256);
    task_create(log_task, 0, 512);
    sched_start();
}
