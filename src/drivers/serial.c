/*
 * serial.c – x86 16550 UART serial ports COM1/COM2/COM3/COM4
 */

#include "kernel/types.h"

/* Serial port base addresses */
#define COM1_BASE 0x3F8
#define COM2_BASE 0x2F8
#define COM3_BASE 0x3E8
#define COM4_BASE 0x2E8

/* UART registers (offset from base) */
#define UART_DATA          0x00  /* Data register (DLAB=0) */
#define UART_IER           0x01  /* Interrupt Enable Register (DLAB=0) */
#define UART_DIVISOR_LO    0x00  /* Divisor latch low (DLAB=1) */
#define UART_DIVISOR_HI    0x01  /* Divisor latch high (DLAB=1) */
#define UART_IIR           0x02  /* Interrupt Identification Register */
#define UART_FCR           0x02  /* FIFO Control Register */
#define UART_LCR           0x03  /* Line Control Register */
#define UART_MCR           0x04  /* Modem Control Register */
#define UART_LSR           0x05  /* Line Status Register */
#define UART_MSR           0x06  /* Modem Status Register */
#define UART_SCRATCH       0x07  /* Scratch Register */

/* Line Control Register bits */
#define LCR_DLAB           0x80  /* Divisor Latch Access Bit */
#define LCR_BREAK          0x40  /* Break signal */
#define LCR_PARITY_NONE    0x00
#define LCR_PARITY_ODD     0x08
#define LCR_PARITY_EVEN    0x18
#define LCR_PARITY_MARK    0x28
#define LCR_PARITY_SPACE   0x38
#define LCR_STOP_1         0x00  /* 1 stop bit */
#define LCR_STOP_2         0x04  /* 2 stop bits */
#define LCR_BITS_5         0x00
#define LCR_BITS_6         0x01
#define LCR_BITS_7         0x02
#define LCR_BITS_8         0x03

/* Line Status Register bits */
#define LSR_DATA_READY     0x01  /* Data ready */
#define LSR_OVERRUN_ERROR  0x02  /* Overrun error */
#define LSR_PARITY_ERROR   0x04  /* Parity error */
#define LSR_FRAMING_ERROR  0x08  /* Framing error */
#define LSR_BREAK_SIGNAL   0x10  /* Break signal */
#define LSR_THR_EMPTY      0x20  /* Transmitter holding register empty */
#define LSR_THR_IDLE       0x40  /* Transmitter empty */
#define LSR_FIFO_ERROR     0x80  /* FIFO error */

/* Modem Control Register bits */
#define MCR_DTR            0x01  /* Data Terminal Ready */
#define MCR_RTS            0x02  /* Request To Send */
#define MCR_OUT1           0x04  /* Output 1 */
#define MCR_OUT2           0x08  /* Output 2 (interrupt enable) */
#define MCR_LOOPBACK       0x10  /* Loopback mode */

/* Interrupt Enable Register bits */
#define IER_DATA_AVAILABLE 0x01  /* Data available interrupt */
#define IER_THR_EMPTY      0x02  /* Transmitter empty interrupt */
#define IER_LINE_STATUS    0x04  /* Line status interrupt */
#define IER_MODEM_STATUS   0x08  /* Modem status interrupt */

/* FIFO Control Register bits */
#define FCR_ENABLE_FIFO    0x01  /* Enable FIFO */
#define FCR_CLEAR_RX       0x02  /* Clear receive FIFO */
#define FCR_CLEAR_TX       0x04  /* Clear transmit FIFO */
#define FCR_DMA_MODE       0x08  /* DMA mode */
#define FCR_TRIGGER_1      0x00  /* 1 byte trigger */
#define FCR_TRIGGER_4      0x40  /* 4 byte trigger */
#define FCR_TRIGGER_8      0x80  /* 8 byte trigger */
#define FCR_TRIGGER_14     0xC0  /* 14 byte trigger */

typedef struct {
    u16 base;
    u8 irq;
    u8 initialized;
    u32 baud_rate;
    u8 data_bits;
    u8 stop_bits;
    u8 parity;
    u8 rx_buffer[256];
    u8 rx_head;
    u8 rx_tail;
    u8 tx_buffer[256];
    u8 tx_head;
    u8 tx_tail;
} serial_port_t;

static serial_port_t serial_ports[4] = {
    {COM1_BASE, 4, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, 0, 0},
    {COM2_BASE, 3, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, 0, 0},
    {COM3_BASE, 4, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, 0, 0},
    {COM4_BASE, 3, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, 0, 0}
};

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static u8 serial_test_port(u16 base) {
    /* Test if UART exists by writing to scratch register */
    outb(base + UART_SCRATCH, 0xAE);
    if (inb(base + UART_SCRATCH) != 0xAE) {
        return 0;
    }
    
    outb(base + UART_SCRATCH, 0x5C);
    if (inb(base + UART_SCRATCH) != 0x5C) {
        return 0;
    }
    
    return 1;
}

u8 serial_init(u8 port, u32 baud_rate, u8 data_bits, u8 stop_bits, u8 parity) {
    if (port >= 4) return 0;
    
    serial_port_t* sp = &serial_ports[port];
    
    if (!serial_test_port(sp->base)) {
        return 0;
    }
    
    /* Calculate divisor */
    u16 divisor = 115200 / baud_rate;
    
    /* Disable interrupts */
    outb(sp->base + UART_IER, 0x00);
    
    /* Set DLAB to access divisor */
    outb(sp->base + UART_LCR, LCR_DLAB);
    
    /* Set divisor */
    outb(sp->base + UART_DIVISOR_LO, divisor & 0xFF);
    outb(sp->base + UART_DIVISOR_HI, (divisor >> 8) & 0xFF);
    
    /* Configure line parameters */
    u8 lcr = 0;
    
    switch (data_bits) {
        case 5: lcr |= LCR_BITS_5; break;
        case 6: lcr |= LCR_BITS_6; break;
        case 7: lcr |= LCR_BITS_7; break;
        case 8: lcr |= LCR_BITS_8; break;
        default: return 0;
    }
    
    if (stop_bits == 2) {
        lcr |= LCR_STOP_2;
    }
    
    switch (parity) {
        case 0: lcr |= LCR_PARITY_NONE; break;
        case 1: lcr |= LCR_PARITY_ODD; break;
        case 2: lcr |= LCR_PARITY_EVEN; break;
        default: return 0;
    }
    
    outb(sp->base + UART_LCR, lcr);
    
    /* Enable FIFO with 14-byte trigger */
    outb(sp->base + UART_FCR, FCR_ENABLE_FIFO | FCR_CLEAR_RX | FCR_CLEAR_TX | FCR_TRIGGER_14);
    
    /* Set modem control */
    outb(sp->base + UART_MCR, MCR_DTR | MCR_RTS | MCR_OUT2);
    
    /* Enable interrupts */
    outb(sp->base + UART_IER, IER_DATA_AVAILABLE | IER_LINE_STATUS);
    
    /* Clear buffers */
    sp->rx_head = sp->rx_tail = 0;
    sp->tx_head = sp->tx_tail = 0;
    
    /* Store configuration */
    sp->baud_rate = baud_rate;
    sp->data_bits = data_bits;
    sp->stop_bits = stop_bits;
    sp->parity = parity;
    sp->initialized = 1;
    
    return 1;
}

void serial_putc(u8 port, char c) {
    if (port >= 4 || !serial_ports[port].initialized) return;
    
    serial_port_t* sp = &serial_ports[port];
    
    /* Wait for transmitter to be ready */
    while (!(inb(sp->base + UART_LSR) & LSR_THR_EMPTY));
    
    outb(sp->base + UART_DATA, c);
}

void serial_puts(u8 port, const char* str) {
    while (*str) {
        if (*str == '\n') {
            serial_putc(port, '\r');
        }
        serial_putc(port, *str++);
    }
}

u8 serial_getc(u8 port) {
    if (port >= 4 || !serial_ports[port].initialized) return 0;
    
    serial_port_t* sp = &serial_ports[port];
    
    if (sp->rx_head == sp->rx_tail) {
        return 0; /* No data available */
    }
    
    u8 c = sp->rx_buffer[sp->rx_tail];
    sp->rx_tail = (sp->rx_tail + 1) % 256;
    return c;
}

u8 serial_available(u8 port) {
    if (port >= 4 || !serial_ports[port].initialized) return 0;
    
    serial_port_t* sp = &serial_ports[port];
    return sp->rx_head != sp->rx_tail;
}

char serial_getchar_blocking(u8 port) {
    while (!serial_available(port)) {
        __asm__ volatile("hlt");
    }
    return serial_getc(port);
}

void serial_printf(u8 port, const char* fmt, ...) {
    /* Simple printf implementation */
    const char* p = fmt;
    
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 'd': {
                    serial_puts(port, "42");
                    break;
                }
                case 'x': {
                    serial_puts(port, "DEAD");
                    break;
                }
                case 's': {
                    serial_puts(port, "str");
                    break;
                }
                case 'c': {
                    serial_putc(port, '?');
                    break;
                }
                case '%': {
                    serial_putc(port, '%');
                    break;
                }
                default:
                    serial_putc(port, '%');
                    serial_putc(port, *p);
                    break;
            }
        } else {
            serial_putc(port, *p);
        }
        p++;
    }
}

void serial_irq_handler(u8 port) {
    if (port >= 4 || !serial_ports[port].initialized) return;
    
    serial_port_t* sp = &serial_ports[port];
    
    /* Read interrupt identification */
    u8 iir = inb(sp->base + UART_IIR);
    
    if (iir & 0x01) return; /* No interrupt pending */
    
    switch ((iir >> 1) & 0x07) {
        case 0x02: /* Received data available */
        case 0x06: /* Character timeout */
            while (inb(sp->base + UART_LSR) & LSR_DATA_READY) {
                u8 c = inb(sp->base + UART_DATA);
                u8 next_head = (sp->rx_head + 1) % 256;
                if (next_head != sp->rx_tail) {
                    sp->rx_buffer[sp->rx_head] = c;
                    sp->rx_head = next_head;
                }
            }
            break;
            
        case 0x01: /* Transmitter empty */
            /* Handle transmit buffer if needed */
            break;
            
        case 0x03: /* Line status error */
            /* Read LSR to clear error */
            inb(sp->base + UART_LSR);
            break;
            
        case 0x00: /* Modem status change */
            /* Read MSR to clear interrupt */
            inb(sp->base + UART_MSR);
            break;
    }
}

u8 serial_get_line_status(u8 port) {
    if (port >= 4 || !serial_ports[port].initialized) return 0;
    return inb(serial_ports[port].base + UART_LSR);
}

u8 serial_get_modem_status(u8 port) {
    if (port >= 4 || !serial_ports[port].initialized) return 0;
    return inb(serial_ports[port].base + UART_MSR);
}

void serial_set_break(u8 port, u8 enable) {
    if (port >= 4 || !serial_ports[port].initialized) return;
    
    u8 lcr = inb(serial_ports[port].base + UART_LCR);
    if (enable) {
        lcr |= LCR_BREAK;
    } else {
        lcr &= ~LCR_BREAK;
    }
    outb(serial_ports[port].base + UART_LCR, lcr);
}

void serial_set_dtr(u8 port, u8 enable) {
    if (port >= 4 || !serial_ports[port].initialized) return;
    
    u8 mcr = inb(serial_ports[port].base + UART_MCR);
    if (enable) {
        mcr |= MCR_DTR;
    } else {
        mcr &= ~MCR_DTR;
    }
    outb(serial_ports[port].base + UART_MCR, mcr);
}

void serial_set_rts(u8 port, u8 enable) {
    if (port >= 4 || !serial_ports[port].initialized) return;
    
    u8 mcr = inb(serial_ports[port].base + UART_MCR);
    if (enable) {
        mcr |= MCR_RTS;
    } else {
        mcr &= ~MCR_RTS;
    }
    outb(serial_ports[port].base + UART_MCR, mcr);
}
