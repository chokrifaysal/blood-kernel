/*
 * serial.h – x86 16550 UART serial ports
 */

#ifndef SERIAL_H
#define SERIAL_H

#include "kernel/types.h"

/* Serial port numbers */
#define SERIAL_COM1 0
#define SERIAL_COM2 1
#define SERIAL_COM3 2
#define SERIAL_COM4 3

/* Parity options */
#define SERIAL_PARITY_NONE  0
#define SERIAL_PARITY_ODD   1
#define SERIAL_PARITY_EVEN  2

/* Core functions */
u8 serial_init(u8 port, u32 baud_rate, u8 data_bits, u8 stop_bits, u8 parity);

/* I/O functions */
void serial_putc(u8 port, char c);
void serial_puts(u8 port, const char* str);
u8 serial_getc(u8 port);
u8 serial_available(u8 port);
char serial_getchar_blocking(u8 port);
void serial_printf(u8 port, const char* fmt, ...);

/* Status functions */
u8 serial_get_line_status(u8 port);
u8 serial_get_modem_status(u8 port);

/* Control functions */
void serial_set_break(u8 port, u8 enable);
void serial_set_dtr(u8 port, u8 enable);
void serial_set_rts(u8 port, u8 enable);

/* Interrupt handler */
void serial_irq_handler(u8 port);

#endif
