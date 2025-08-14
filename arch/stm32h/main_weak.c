/*
 * arch/stm32h/main_weak.c – STM32H745 overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "ARM-M7+M4"; }
const char *mcu_name(void)  { return "STM32H745"; }
const char *boot_name(void) { return "CAN-FD+ETH"; }

void canfd_init(u32 bitrate);
void eth_phy_init(void);
void qspi_init(void);
void usb_hs_init(void);

void ipc_init(void) {
    /* Dual-core IPC already initialized */
}

void gpio_init(void) {
    /* CAN-FD pins: PD0=RX, PD1=TX */
    /* ETH pins: PA1,PA2,PA7,PB13,PC1,PC4,PC5,PG11,PG13 */
    /* QSPI pins: PB2,PB6,PD11,PD12,PD13,PE2 */
    /* USB HS pins: PB14,PB15 */
}
