/*
 * canfd.c – STM32H745 FDCAN @ 1 Mbit/s nominal, 8 Mbit/s data
 */

#include "kernel/types.h"

#define FDCAN1_BASE 0x4000A000UL
#define RCC_APB1HENR (*(volatile u32*)0x58024454UL)

typedef volatile struct {
    u32 CREL;
    u32 ENDN;
    u32 _pad1;
    u32 DBTP;
    u32 TEST;
    u32 RWD;
    u32 CCCR;
    u32 NBTP;
    u32 TSCC;
    u32 TSCV;
    u32 TOCC;
    u32 TOCV;
    u32 _pad2[4];
    u32 ECR;
    u32 PSR;
    u32 TDCR;
    u32 _pad3;
    u32 IR;
    u32 IE;
    u32 ILS;
    u32 ILE;
    u32 _pad4[8];
    u32 GFC;
    u32 SIDFC;
    u32 XIDFC;
    u32 _pad5;
    u32 XIDAM;
    u32 HPMS;
    u32 NDAT1;
    u32 NDAT2;
    u32 RXF0C;
    u32 RXF0S;
    u32 RXF0A;
    u32 RXBC;
    u32 RXF1C;
    u32 RXF1S;
    u32 RXF1A;
    u32 RXESC;
    u32 TXBC;
    u32 TXFQS;
    u32 TXESC;
    u32 TXBRP;
    u32 TXBAR;
    u32 TXBCR;
    u32 TXBTO;
    u32 TXBCF;
    u32 TXBTIE;
    u32 TXBCIE;
    u32 _pad6[2];
    u32 TXEFC;
    u32 TXEFS;
    u32 TXEFA;
} FDCAN_TypeDef;

static FDCAN_TypeDef* const FDCAN1 = (FDCAN_TypeDef*)FDCAN1_BASE;

typedef struct {
    u32 id;
    u8 len;
    u8 flags;
    u8 data[64];
} canfd_frame_t;

static u32 msg_ram[512] __attribute__((aligned(4)));

void canfd_init(u32 bitrate) {
    /* Enable FDCAN1 clock */
    RCC_APB1HENR |= (1<<8);
    
    /* Init mode */
    FDCAN1->CCCR = (1<<0);
    while (!(FDCAN1->CCCR & (1<<0)));
    
    /* Config change enable */
    FDCAN1->CCCR |= (1<<1);
    while (!(FDCAN1->CCCR & (1<<1)));
    
    /* Nominal bit timing: 1 Mbit/s @ 80 MHz */
    FDCAN1->NBTP = (1<<25) | (7<<16) | (9<<8) | 1;
    
    /* Data bit timing: 8 Mbit/s @ 80 MHz */
    FDCAN1->DBTP = (1<<23) | (1<<16) | (1<<8) | 1;
    
    /* FD mode + bit rate switching */
    FDCAN1->CCCR |= (1<<9) | (1<<8);
    
    /* Message RAM config */
    FDCAN1->SIDFC = ((u32)msg_ram) | (128<<16);
    FDCAN1->XIDFC = ((u32)msg_ram + 512) | (64<<16);
    FDCAN1->RXF0C = ((u32)msg_ram + 1024) | (64<<16);
    FDCAN1->TXBC = ((u32)msg_ram + 1536) | (32<<24);
    
    /* Exit init mode */
    FDCAN1->CCCR &= ~(1<<0);
    while (FDCAN1->CCCR & (1<<0));
}

u8 canfd_send(const canfd_frame_t* f) {
    /* Check TX FIFO free */
    if (!(FDCAN1->TXFQS & 0x3F)) return 1;
    
    u32 put_idx = (FDCAN1->TXFQS >> 16) & 0x1F;
    u32* tx_buf = (u32*)((u32)msg_ram + 1536 + put_idx * 72);
    
    /* Header */
    tx_buf[0] = (f->id << 18) | (f->len << 16);
    if (f->flags & 1) tx_buf[0] |= (1<<21); // FDF
    if (f->flags & 2) tx_buf[0] |= (1<<20); // BRS
    
    /* Data */
    for (u8 i = 0; i < f->len; i += 4) {
        tx_buf[2 + i/4] = *(u32*)(f->data + i);
    }
    
    /* Request transmission */
    FDCAN1->TXBAR = (1 << put_idx);
    return 0;
}

u8 canfd_recv(canfd_frame_t* f) {
    /* Check RX FIFO 0 */
    if (!(FDCAN1->RXF0S & 0x7F)) return 1;
    
    u32 get_idx = (FDCAN1->RXF0S >> 8) & 0x3F;
    u32* rx_buf = (u32*)((u32)msg_ram + 1024 + get_idx * 72);
    
    /* Header */
    f->id = (rx_buf[0] >> 18) & 0x1FFFFFFF;
    f->len = (rx_buf[1] >> 16) & 0xF;
    f->flags = 0;
    if (rx_buf[0] & (1<<21)) f->flags |= 1; // FDF
    if (rx_buf[0] & (1<<20)) f->flags |= 2; // BRS
    
    /* Data */
    for (u8 i = 0; i < f->len; i += 4) {
        *(u32*)(f->data + i) = rx_buf[2 + i/4];
    }
    
    /* Acknowledge */
    FDCAN1->RXF0A = get_idx;
    return 0;
}

void canfd_set_filter(u16 id, u16 mask) {
    /* Standard ID filter 0 */
    u32* filt = (u32*)msg_ram;
    filt[0] = (mask << 16) | id;
}

u8 canfd_get_state(void) {
    return (FDCAN1->PSR >> 3) & 0x7;
}
