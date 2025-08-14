/*
 * vga.c – x86 VGA text mode 80x25 @ 0xB8000
 */

#include "kernel/types.h"

#define VGA_BASE 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)

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

static volatile u16* vga_buf = (u16*)VGA_BASE;
static u8 vga_row = 0;
static u8 vga_col = 0;
static u8 vga_color = VGA_LGRAY | (VGA_BLACK << 4);

static inline u16 vga_entry(u8 c, u8 color) {
    return c | (color << 8);
}

static void vga_scroll(void) {
    /* Move all lines up */
    for (u16 i = 0; i < VGA_SIZE - VGA_WIDTH; i++) {
        vga_buf[i] = vga_buf[i + VGA_WIDTH];
    }
    
    /* Clear last line */
    for (u8 x = 0; x < VGA_WIDTH; x++) {
        vga_buf[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_color);
    }
    
    vga_row = VGA_HEIGHT - 1;
}

void vga_init(void) {
    vga_row = 0;
    vga_col = 0;
    vga_color = VGA_LGRAY | (VGA_BLACK << 4);
    
    /* Clear screen */
    for (u16 i = 0; i < VGA_SIZE; i++) {
        vga_buf[i] = vga_entry(' ', vga_color);
    }
}

void vga_set_color(u8 fg, u8 bg) {
    vga_color = fg | (bg << 4);
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_col = 0;
        if (++vga_row >= VGA_HEIGHT) {
            vga_scroll();
        }
        return;
    }
    
    if (c == '\r') {
        vga_col = 0;
        return;
    }
    
    if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            if (++vga_row >= VGA_HEIGHT) {
                vga_scroll();
            }
        }
        return;
    }
    
    if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            vga_buf[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', vga_color);
        }
        return;
    }
    
    /* Normal character */
    vga_buf[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_color);
    
    if (++vga_col >= VGA_WIDTH) {
        vga_col = 0;
        if (++vga_row >= VGA_HEIGHT) {
            vga_scroll();
        }
    }
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_clear(void) {
    vga_row = 0;
    vga_col = 0;
    
    for (u16 i = 0; i < VGA_SIZE; i++) {
        vga_buf[i] = vga_entry(' ', vga_color);
    }
}

void vga_set_cursor(u8 row, u8 col) {
    if (row < VGA_HEIGHT && col < VGA_WIDTH) {
        vga_row = row;
        vga_col = col;
    }
}

void vga_get_cursor(u8* row, u8* col) {
    *row = vga_row;
    *col = vga_col;
}

void vga_putc_at(char c, u8 row, u8 col, u8 color) {
    if (row < VGA_HEIGHT && col < VGA_WIDTH) {
        vga_buf[row * VGA_WIDTH + col] = vga_entry(c, color);
    }
}

void vga_puts_at(const char* str, u8 row, u8 col, u8 color) {
    u8 orig_row = vga_row;
    u8 orig_col = vga_col;
    u8 orig_color = vga_color;
    
    vga_row = row;
    vga_col = col;
    vga_color = color;
    
    vga_puts(str);
    
    vga_row = orig_row;
    vga_col = orig_col;
    vga_color = orig_color;
}

void vga_fill_rect(u8 row, u8 col, u8 width, u8 height, char c, u8 color) {
    for (u8 y = 0; y < height && (row + y) < VGA_HEIGHT; y++) {
        for (u8 x = 0; x < width && (col + x) < VGA_WIDTH; x++) {
            vga_buf[(row + y) * VGA_WIDTH + (col + x)] = vga_entry(c, color);
        }
    }
}

void vga_draw_box(u8 row, u8 col, u8 width, u8 height, u8 color) {
    /* Box drawing characters */
    const char box_chars[] = {
        0xDA, 0xC4, 0xBF,  /* ┌─┐ */
        0xB3, ' ',  0xB3,  /* │ │ */
        0xC0, 0xC4, 0xD9   /* └─┘ */
    };
    
    if (width < 2 || height < 2) return;
    
    /* Top border */
    vga_putc_at(box_chars[0], row, col, color);
    for (u8 x = 1; x < width - 1; x++) {
        vga_putc_at(box_chars[1], row, col + x, color);
    }
    vga_putc_at(box_chars[2], row, col + width - 1, color);
    
    /* Side borders */
    for (u8 y = 1; y < height - 1; y++) {
        vga_putc_at(box_chars[3], row + y, col, color);
        vga_putc_at(box_chars[5], row + y, col + width - 1, color);
    }
    
    /* Bottom border */
    vga_putc_at(box_chars[6], row + height - 1, col, color);
    for (u8 x = 1; x < width - 1; x++) {
        vga_putc_at(box_chars[7], row + height - 1, col + x, color);
    }
    vga_putc_at(box_chars[8], row + height - 1, col + width - 1, color);
}

void vga_printf(const char* fmt, ...) {
    /* Simple printf implementation */
    const char* p = fmt;
    
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 'd': {
                    /* Integer - simplified */
                    vga_puts("42");
                    break;
                }
                case 'x': {
                    /* Hex - simplified */
                    vga_puts("0xDEAD");
                    break;
                }
                case 's': {
                    /* String - simplified */
                    vga_puts("str");
                    break;
                }
                case 'c': {
                    /* Character - simplified */
                    vga_putc('?');
                    break;
                }
                case '%': {
                    vga_putc('%');
                    break;
                }
                default:
                    vga_putc('%');
                    vga_putc(*p);
                    break;
            }
        } else {
            vga_putc(*p);
        }
        p++;
    }
}

/* Hardware cursor control */
static void vga_update_cursor(void) {
    u16 pos = vga_row * VGA_WIDTH + vga_col;
    
    /* Cursor low byte */
    __asm__ volatile("outb %0, $0x3D4" : : "a"((u8)0x0F));
    __asm__ volatile("outb %0, $0x3D5" : : "a"((u8)(pos & 0xFF)));
    
    /* Cursor high byte */
    __asm__ volatile("outb %0, $0x3D4" : : "a"((u8)0x0E));
    __asm__ volatile("outb %0, $0x3D5" : : "a"((u8)((pos >> 8) & 0xFF)));
}

void vga_enable_cursor(void) {
    __asm__ volatile("outb %0, $0x3D4" : : "a"((u8)0x0A));
    __asm__ volatile("outb %0, $0x3D5" : : "a"((u8)0x00));
    
    __asm__ volatile("outb %0, $0x3D4" : : "a"((u8)0x0B));
    __asm__ volatile("outb %0, $0x3D5" : : "a"((u8)0x0F));
    
    vga_update_cursor();
}

void vga_disable_cursor(void) {
    __asm__ volatile("outb %0, $0x3D4" : : "a"((u8)0x0A));
    __asm__ volatile("outb %0, $0x3D5" : : "a"((u8)0x20));
}
