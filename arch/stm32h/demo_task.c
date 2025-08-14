/*
 * demo_task.c – STM32H745 CAN-FD + Ethernet + QSPI demo
 */

#include "kernel/types.h"
#include "canfd.h"

extern void timer_delay(u32 ms);
extern void qspi_init(void);
extern u32 qspi_read_id(void);
extern void eth_phy_init(void);
extern u8 eth_phy_link_up(void);
extern void usb_hs_init(void);

static void canfd_demo_task(void) {
    canfd_frame_t tx_frame, rx_frame;
    u32 cnt = 0;
    
    canfd_init(1000000); // 1 Mbit/s
    canfd_set_filter(0x123, 0x7FF);
    
    while (1) {
        /* Send CAN-FD frame */
        tx_frame.id = 0x123;
        tx_frame.len = 8;
        tx_frame.flags = CANFD_FDF | CANFD_BRS;
        
        for (u8 i = 0; i < 8; i++) {
            tx_frame.data[i] = cnt + i;
        }
        
        if (canfd_send(&tx_frame) == 0) {
            cnt++;
        }
        
        /* Check for received frames */
        if (canfd_recv(&rx_frame) == 0) {
            /* Process received frame */
        }
        
        timer_delay(100);
    }
}

static void eth_demo_task(void) {
    u8 link, speed, duplex;
    
    eth_phy_init();
    
    while (1) {
        eth_phy_get_status(&link, &speed, &duplex);
        
        if (link) {
            /* Link is up - could send/receive packets */
        }
        
        timer_delay(1000);
    }
}

static void qspi_demo_task(void) {
    u32 flash_id;
    u8 test_data[256];
    u8 read_data[256];
    
    qspi_init();
    flash_id = qspi_read_id();
    
    /* Test pattern */
    for (u16 i = 0; i < 256; i++) {
        test_data[i] = i & 0xFF;
    }
    
    while (1) {
        /* Erase sector 0 */
        qspi_sector_erase(0x0000);
        
        /* Write test data */
        qspi_write(0x0000, test_data, 256);
        
        /* Read back and verify */
        qspi_read(0x0000, read_data, 256);
        
        timer_delay(5000);
    }
}

static void usb_demo_task(void) {
    usb_hs_init();
    usb_hs_connect();
    
    while (1) {
        if (usb_hs_configured()) {
            /* USB is configured - can communicate */
        }
        
        timer_delay(100);
    }
}

void stm32h_demo_init(void) {
    /* Called from kernel_main for STM32H745 */
    extern void task_create(void (*func)(void), u8 prio, u16 stack_size);
    
    task_create(canfd_demo_task, 1, 256);
    task_create(eth_demo_task, 2, 256);
    task_create(qspi_demo_task, 3, 256);
    task_create(usb_demo_task, 4, 256);
}
