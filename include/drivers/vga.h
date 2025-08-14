/*
 * vga.h – x86 VGA text mode driver
 */

#ifndef VGA_H
#define VGA_H

#include "kernel/types.h"

/* VGA colors */
#define VGA_BLACK     0
#define VGA_BLUE      1
#define VGA_GREEN     2
#define VGA_CYAN      3
#define VGA_RED       4
#define VGA_MAGENTA   5
#define VGA_BROWN     6
#define VGA_LGRAY     7
#define VGA_DGRAY     8
#define VGA_LBLUE     9
#define VGA_LGREEN    10
#define VGA_LCYAN     11
#define VGA_LRED      12
#define VGA_LMAGENTA  13
#define VGA_YELLOW    14
#define VGA_WHITE     15

/* Core functions */
void vga_init(void);
void vga_set_color(u8 fg, u8 bg);
void vga_putc(char c);
void vga_puts(const char* str);
void vga_clear(void);

/* Cursor functions */
void vga_set_cursor(u8 row, u8 col);
void vga_get_cursor(u8* row, u8* col);
void vga_enable_cursor(void);
void vga_disable_cursor(void);

/* Advanced functions */
void vga_putc_at(char c, u8 row, u8 col, u8 color);
void vga_puts_at(const char* str, u8 row, u8 col, u8 color);
void vga_fill_rect(u8 row, u8 col, u8 width, u8 height, char c, u8 color);
void vga_draw_box(u8 row, u8 col, u8 width, u8 height, u8 color);
void vga_printf(const char* fmt, ...);

#endif
