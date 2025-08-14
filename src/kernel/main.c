/*
 * kernel_main – dual-core entry points
 */

#ifdef __arm__ && defined(__ARM_ARCH_8M_MAIN__)
#include "arch/stm32h745/clock.c"
#include "arch/stm32h745/uart_usart3.c"
#include "arch/stm32h745/ipc_mailbox_500loc.c"
#include "arch/stm32h745/dual_scheduler.c"
#endif

void kernel_main(void) {
#ifdef __ARM_ARCH_8M_MAIN__
    clock_init();
    uart_early_init();
    kprintf("STM32H745 CM7 480 MHz\r\n");
    ipc_init();
    sched_init_m7();
    sched_start_m7();
#endif
}

void kernel_main_m4(void) {
#ifdef __ARM_ARCH_8M_MAIN__
    clock_init();
    uart_early_init();
    kprintf("STM32H745 CM4 240 MHz\r\n");
    sched_init_m4();
    sched_start_m4();
#endif
}
