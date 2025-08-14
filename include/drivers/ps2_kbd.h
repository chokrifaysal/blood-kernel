/*
 * ps2_kbd.h – x86 PS/2 keyboard driver
 */

#ifndef PS2_KBD_H
#define PS2_KBD_H

#include "kernel/types.h"

/* Modifier flags */
#define KBD_MOD_SHIFT 0x01
#define KBD_MOD_CTRL  0x02
#define KBD_MOD_ALT   0x04
#define KBD_MOD_CAPS  0x08

/* LED flags */
#define KBD_LED_SCROLL 0x01
#define KBD_LED_NUM    0x02
#define KBD_LED_CAPS   0x04

/* Core functions */
void ps2_kbd_init(void);
void ps2_kbd_irq_handler(void);

/* Input functions */
u8 ps2_kbd_getc(void);
u8 ps2_kbd_available(void);
char ps2_kbd_getchar_blocking(void);
void ps2_kbd_flush(void);

/* Status functions */
u8 ps2_kbd_get_modifiers(void);
void ps2_kbd_set_leds(u8 leds);

#endif
