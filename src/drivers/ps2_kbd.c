/*
 * ps2_kbd.c – x86 PS/2 keyboard driver
 */

#include "kernel/types.h"

#define PS2_DATA_PORT 0x60
#define PS2_CMD_PORT  0x64

/* Keyboard scan codes */
#define KEY_ESC       0x01
#define KEY_1         0x02
#define KEY_2         0x03
#define KEY_3         0x04
#define KEY_4         0x05
#define KEY_5         0x06
#define KEY_6         0x07
#define KEY_7         0x08
#define KEY_8         0x09
#define KEY_9         0x0A
#define KEY_0         0x0B
#define KEY_MINUS     0x0C
#define KEY_EQUALS    0x0D
#define KEY_BACKSPACE 0x0E
#define KEY_TAB       0x0F
#define KEY_Q         0x10
#define KEY_W         0x11
#define KEY_E         0x12
#define KEY_R         0x13
#define KEY_T         0x14
#define KEY_Y         0x15
#define KEY_U         0x16
#define KEY_I         0x17
#define KEY_O         0x18
#define KEY_P         0x19
#define KEY_LBRACKET  0x1A
#define KEY_RBRACKET  0x1B
#define KEY_ENTER     0x1C
#define KEY_LCTRL     0x1D
#define KEY_A         0x1E
#define KEY_S         0x1F
#define KEY_D         0x20
#define KEY_F         0x21
#define KEY_G         0x22
#define KEY_H         0x23
#define KEY_J         0x24
#define KEY_K         0x25
#define KEY_L         0x26
#define KEY_SEMICOLON 0x27
#define KEY_QUOTE     0x28
#define KEY_BACKTICK  0x29
#define KEY_LSHIFT    0x2A
#define KEY_BACKSLASH 0x2B
#define KEY_Z         0x2C
#define KEY_X         0x2D
#define KEY_C         0x2E
#define KEY_V         0x2F
#define KEY_B         0x30
#define KEY_N         0x31
#define KEY_M         0x32
#define KEY_COMMA     0x33
#define KEY_PERIOD    0x34
#define KEY_SLASH     0x35
#define KEY_RSHIFT    0x36
#define KEY_SPACE     0x39

static const char scancode_to_ascii[] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_shift[] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static u8 shift_pressed = 0;
static u8 ctrl_pressed = 0;
static u8 alt_pressed = 0;
static u8 caps_lock = 0;

static u8 kbd_buffer[256];
static u8 kbd_head = 0;
static u8 kbd_tail = 0;

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static u8 ps2_wait_input(void) {
    u32 timeout = 100000;
    while (timeout-- && !(inb(PS2_CMD_PORT) & 0x02));
    return timeout > 0;
}

static u8 ps2_wait_output(void) {
    u32 timeout = 100000;
    while (timeout-- && !(inb(PS2_CMD_PORT) & 0x01));
    return timeout > 0;
}

static void ps2_send_cmd(u8 cmd) {
    ps2_wait_input();
    outb(PS2_CMD_PORT, cmd);
}

static void ps2_send_data(u8 data) {
    ps2_wait_input();
    outb(PS2_DATA_PORT, data);
}

static u8 ps2_read_data(void) {
    ps2_wait_output();
    return inb(PS2_DATA_PORT);
}

void ps2_kbd_init(void) {
    /* Disable devices */
    ps2_send_cmd(0xAD);
    ps2_send_cmd(0xA7);
    
    /* Flush output buffer */
    inb(PS2_DATA_PORT);
    
    /* Set controller config */
    ps2_send_cmd(0x20);
    u8 config = ps2_read_data();
    config &= ~(0x01 | 0x02 | 0x40); /* Disable IRQs and translation */
    ps2_send_cmd(0x60);
    ps2_send_data(config);
    
    /* Test controller */
    ps2_send_cmd(0xAA);
    if (ps2_read_data() != 0x55) {
        return; /* Controller test failed */
    }
    
    /* Test first port */
    ps2_send_cmd(0xAB);
    if (ps2_read_data() != 0x00) {
        return; /* Port test failed */
    }
    
    /* Enable first port */
    ps2_send_cmd(0xAE);
    
    /* Reset keyboard */
    ps2_send_data(0xFF);
    if (ps2_read_data() != 0xFA) {
        return; /* Reset failed */
    }
    if (ps2_read_data() != 0xAA) {
        return; /* Self-test failed */
    }
    
    /* Set scan code set 1 */
    ps2_send_data(0xF0);
    ps2_read_data(); /* ACK */
    ps2_send_data(0x01);
    ps2_read_data(); /* ACK */
    
    /* Enable scanning */
    ps2_send_data(0xF4);
    ps2_read_data(); /* ACK */
    
    /* Enable IRQ */
    config |= 0x01;
    ps2_send_cmd(0x60);
    ps2_send_data(config);
}

static void kbd_buffer_put(u8 c) {
    u8 next_head = (kbd_head + 1) % 256;
    if (next_head != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next_head;
    }
}

void ps2_kbd_irq_handler(void) {
    u8 scancode = inb(PS2_DATA_PORT);
    
    /* Handle key release */
    if (scancode & 0x80) {
        scancode &= 0x7F;
        switch (scancode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                shift_pressed = 0;
                break;
            case KEY_LCTRL:
                ctrl_pressed = 0;
                break;
        }
        return;
    }
    
    /* Handle key press */
    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_pressed = 1;
            break;
        case KEY_LCTRL:
            ctrl_pressed = 1;
            break;
        default: {
            char ascii = 0;
            
            if (scancode < sizeof(scancode_to_ascii)) {
                if (shift_pressed) {
                    ascii = scancode_to_ascii_shift[scancode];
                } else {
                    ascii = scancode_to_ascii[scancode];
                }
                
                /* Handle caps lock for letters */
                if (caps_lock && ascii >= 'a' && ascii <= 'z') {
                    ascii = ascii - 'a' + 'A';
                } else if (caps_lock && ascii >= 'A' && ascii <= 'Z') {
                    ascii = ascii - 'A' + 'a';
                }
                
                /* Handle Ctrl combinations */
                if (ctrl_pressed && ascii >= 'a' && ascii <= 'z') {
                    ascii = ascii - 'a' + 1; /* Ctrl+A = 1, etc. */
                } else if (ctrl_pressed && ascii >= 'A' && ascii <= 'Z') {
                    ascii = ascii - 'A' + 1;
                }
                
                if (ascii) {
                    kbd_buffer_put(ascii);
                }
            }
            break;
        }
    }
}

u8 ps2_kbd_getc(void) {
    if (kbd_head == kbd_tail) {
        return 0; /* No key available */
    }
    
    u8 c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % 256;
    return c;
}

u8 ps2_kbd_available(void) {
    return kbd_head != kbd_tail;
}

void ps2_kbd_flush(void) {
    kbd_head = kbd_tail = 0;
}

u8 ps2_kbd_get_modifiers(void) {
    u8 mods = 0;
    if (shift_pressed) mods |= 0x01;
    if (ctrl_pressed)  mods |= 0x02;
    if (alt_pressed)   mods |= 0x04;
    if (caps_lock)     mods |= 0x08;
    return mods;
}

void ps2_kbd_set_leds(u8 leds) {
    ps2_send_data(0xED);
    ps2_read_data(); /* ACK */
    ps2_send_data(leds & 0x07);
    ps2_read_data(); /* ACK */
}

char ps2_kbd_getchar_blocking(void) {
    while (!ps2_kbd_available()) {
        __asm__ volatile("hlt"); /* Wait for interrupt */
    }
    return ps2_kbd_getc();
}
