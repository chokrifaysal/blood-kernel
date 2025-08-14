/*
 * pio_driver.c – RP2040 PIO state machines
 */

#include "pio_driver.h"
#include "kernel/types.h"

#define PIO0_BASE 0x50200000UL
#define PIO1_BASE 0x50300000UL
#define RESETS_BASE 0x4000C000UL

typedef volatile struct {
    u32 CTRL;
    u32 FSTAT;
    u32 FDEBUG;
    u32 FLEVEL;
    u32 TXF[4];
    u32 RXF[4];
    u32 IRQ;
    u32 IRQ_FORCE;
    u32 INPUT_SYNC_BYPASS;
    u32 DBG_PADOUT;
    u32 DBG_PADOE;
    u32 DBG_CFGINFO;
    u32 INSTR_MEM[32];
    struct {
        u32 CLKDIV;
        u32 EXECCTRL;
        u32 SHIFTCTRL;
        u32 ADDR;
        u32 INSTR;
        u32 PINCTRL;
    } SM[4];
    u32 INTR;
    u32 IRQ0_INTE;
    u32 IRQ0_INTF;
    u32 IRQ0_INTS;
    u32 IRQ1_INTE;
    u32 IRQ1_INTF;
    u32 IRQ1_INTS;
} PIO_TypeDef;

static PIO_TypeDef* const PIO0 = (PIO_TypeDef*)PIO0_BASE;
static PIO_TypeDef* const PIO1 = (PIO_TypeDef*)PIO1_BASE;

/* PIO programs */
static const u16 blink_prog[] = {
    0xE081,  // set pins, 1 [31]
    0xE000,  // set pins, 0 [31]
};

static const u16 uart_tx_prog[] = {
    0x9080,  // pull block
    0x0081,  // out pins, 1
    0x1008,  // jmp !osre, 1
    0xE001,  // set pins, 1 [1]
};

static const u16 spi_prog[] = {
    0x6001,  // out pins, 1
    0x5001,  // in pins, 1
    0x0000,  // jmp 0
};

static const u16 ws2812_prog[] = {
    0x6221,  // out x, 1 side 0 [2]
    0x1123,  // jmp !x, 3 side 1 [1]
    0x1400,  // jmp 0 side 1 [4]
    0xA442,  // nop side 0 [4]
    0x0001,  // jmp 1
};

void pio_init(void) {
    /* Reset PIO blocks */
    *(u32*)(RESETS_BASE + 0x0) &= ~((1<<11) | (1<<12));
    while (*(u32*)(RESETS_BASE + 0x8) & ((1<<11) | (1<<12)));
    *(u32*)(RESETS_BASE + 0x4) |= (1<<11) | (1<<12);
    while (!(*(u32*)(RESETS_BASE + 0x8) & ((1<<11) | (1<<12))));
}

u8 pio_load_program(u8 pio_num, const u16* prog, u8 len) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;

    /* Find free instruction memory */
    u8 offset = 0;
    for (u8 i = 0; i < 32 - len; i++) {
        u8 free = 1;
        for (u8 j = 0; j < len; j++) {
            if (pio->INSTR_MEM[i + j] != 0) {
                free = 0;
                break;
            }
        }
        if (free) {
            offset = i;
            break;
        }
    }

    /* Load program */
    for (u8 i = 0; i < len; i++) {
        pio->INSTR_MEM[offset + i] = prog[i];
    }

    return offset;
}

void pio_sm_init(u8 pio_num, u8 sm, u8 offset, const pio_config_t* config) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;

    /* Disable state machine */
    pio->CTRL &= ~(1 << sm);

    /* Configure state machine */
    pio->SM[sm].CLKDIV = config->clkdiv << 8;
    pio->SM[sm].EXECCTRL = (config->wrap_top << 12) | (config->wrap_bottom << 7) | offset;
    pio->SM[sm].SHIFTCTRL = (config->autopull << 17) | (config->autopush << 16) |
                           (config->pull_thresh << 25) | (config->push_thresh << 20);
    pio->SM[sm].PINCTRL = (config->set_count << 26) | (config->out_count << 20) |
                         (config->in_base << 15) | (config->set_base << 5) | config->out_base;

    /* Clear FIFOs */
    pio->SM[sm].SHIFTCTRL |= (1<<18) | (1<<19);
    pio->SM[sm].SHIFTCTRL &= ~((1<<18) | (1<<19));
}

void pio_sm_start(u8 pio_num, u8 sm) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;
    pio->CTRL |= (1 << sm);
}

void pio_sm_stop(u8 pio_num, u8 sm) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;
    pio->CTRL &= ~(1 << sm);
}

void pio_sm_put(u8 pio_num, u8 sm, u32 data) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;
    while (pio->FSTAT & (1 << (16 + sm))); /* Wait for TX FIFO not full */
    pio->TXF[sm] = data;
}

u32 pio_sm_get(u8 pio_num, u8 sm) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;
    while (pio->FSTAT & (1 << (8 + sm))); /* Wait for RX FIFO not empty */
    return pio->RXF[sm];
}

u8 pio_sm_tx_full(u8 pio_num, u8 sm) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;
    return (pio->FSTAT >> (16 + sm)) & 1;
}

u8 pio_sm_rx_empty(u8 pio_num, u8 sm) {
    PIO_TypeDef* pio = (pio_num == 0) ? PIO0 : PIO1;
    return (pio->FSTAT >> (8 + sm)) & 1;
}

void pio_load_blink(u8 pin) {
    pio_config_t config = {
        .clkdiv = 125000,  /* 125 MHz / 125000 = 1 kHz */
        .wrap_top = 1,
        .wrap_bottom = 0,
        .autopull = 0,
        .autopush = 0,
        .pull_thresh = 32,
        .push_thresh = 32,
        .set_count = 1,
        .out_count = 0,
        .in_base = 0,
        .set_base = pin,
        .out_base = 0
    };

    u8 offset = pio_load_program(0, blink_prog, 2);
    pio_sm_init(0, 0, offset, &config);
    pio_sm_start(0, 0);
}

void pio_uart_tx_init(u8 pin, u32 baud) {
    pio_config_t config = {
        .clkdiv = (125000000UL * 8) / baud,
        .wrap_top = 3,
        .wrap_bottom = 0,
        .autopull = 1,
        .autopush = 0,
        .pull_thresh = 8,
        .push_thresh = 32,
        .set_count = 1,
        .out_count = 1,
        .in_base = 0,
        .set_base = pin,
        .out_base = pin
    };

    u8 offset = pio_load_program(0, uart_tx_prog, 4);
    pio_sm_init(0, 1, offset, &config);
    pio_sm_start(0, 1);
}

void pio_spi_init(u8 clk_pin, u8 mosi_pin, u8 miso_pin) {
    pio_config_t config = {
        .clkdiv = 125,  /* 1 MHz SPI */
        .wrap_top = 2,
        .wrap_bottom = 0,
        .autopull = 1,
        .autopush = 1,
        .pull_thresh = 8,
        .push_thresh = 8,
        .set_count = 0,
        .out_count = 1,
        .in_base = miso_pin,
        .set_base = 0,
        .out_base = mosi_pin
    };

    u8 offset = pio_load_program(0, spi_prog, 3);
    pio_sm_init(0, 2, offset, &config);
    pio_sm_start(0, 2);
}

void pio_ws2812_init(u8 pin) {
    pio_config_t config = {
        .clkdiv = 125,  /* 1 MHz for WS2812 timing */
        .wrap_top = 4,
        .wrap_bottom = 0,
        .autopull = 1,
        .autopush = 0,
        .pull_thresh = 24,
        .push_thresh = 32,
        .set_count = 0,
        .out_count = 1,
        .in_base = 0,
        .set_base = 0,
        .out_base = pin
    };

    u8 offset = pio_load_program(1, ws2812_prog, 5);
    pio_sm_init(1, 0, offset, &config);
    pio_sm_start(1, 0);
}

void pio_uart_tx_byte(u8 data) {
    pio_sm_put(0, 1, data);
}

u8 pio_spi_xfer(u8 data) {
    pio_sm_put(0, 2, data);
    return pio_sm_get(0, 2);
}

void pio_ws2812_put_pixel(u32 rgb) {
    pio_sm_put(1, 0, rgb << 8);
}
