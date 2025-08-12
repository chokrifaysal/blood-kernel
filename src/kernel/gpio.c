/*
 * gpio.c - x86 (no GPIO) and STM32F4 GPIO
 */

#include "kernel/gpio.h"
#include "kernel/types.h"

#ifdef __x86_64__
// x86 PC has no GPIO - stubs for now
void gpio_init(void) { }
void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode) { (void)pin; (void)mode; }
void gpio_set(gpio_pin_t pin) { (void)pin; }
void gpio_clear(gpio_pin_t pin) { (void)pin; }
void gpio_toggle(gpio_pin_t pin) { (void)pin; }
u8   gpio_read(gpio_pin_t pin) { (void)pin; return 0; }

#elif defined(__arm__)
#define RCC_BASE     0x40023800
#define GPIOA_BASE   0x40020000
#define GPIOB_BASE   0x40020400
#define GPIOC_BASE   0x40020800

#define RCC_AHB1ENR  (*(volatile u32*)(RCC_BASE + 0x30))

typedef struct {
    volatile u32 MODER;
    volatile u32 OTYPER;
    volatile u32 OSPEEDR;
    volatile u32 PUPDR;
    volatile u32 IDR;
    volatile u32 ODR;
    volatile u32 BSRR;
    volatile u32 LCKR;
    volatile u32 AFR[2];
} GPIO_TypeDef;

static GPIO_TypeDef* const GPIOA = (GPIO_TypeDef*)GPIOA_BASE;
static GPIO_TypeDef* const GPIOB = (GPIO_TypeDef*)GPIOB_BASE;
static GPIO_TypeDef* const GPIOC = (GPIO_TypeDef*)GPIOC_BASE;

static GPIO_TypeDef* gpio_port(gpio_pin_t pin) {
    if (pin <= PA7) return GPIOA;
    if (pin <= PB7) return GPIOB;
    return GPIOC;
}

static u32 gpio_pin_num(gpio_pin_t pin) {
    if (pin <= PA7) return pin - PA0;
    if (pin <= PB7) return pin - PB0;
    return pin - PC13;
}

void gpio_init(void) {
    RCC_AHB1ENR |= (1<<0) | (1<<1) | (1<<2);  // GPIOA, B, C clocks
}

void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode) {
    GPIO_TypeDef* port = gpio_port(pin);
    u32 pin_num = gpio_pin_num(pin);
    
    port->MODER &= ~(3 << (pin_num * 2));
    port->MODER |= (mode << (pin_num * 2));
}

void gpio_set(gpio_pin_t pin) {
    gpio_port(pin)->BSRR = (1 << gpio_pin_num(pin));
}

void gpio_clear(gpio_pin_t pin) {
    gpio_port(pin)->BSRR = (1 << (gpio_pin_num(pin) + 16));
}

void gpio_toggle(gpio_pin_t pin) {
    u32 val = gpio_port(pin)->ODR;
    gpio_port(pin)->ODR ^= (1 << gpio_pin_num(pin));
}

u8 gpio_read(gpio_pin_t pin) {
    return (gpio_port(pin)->IDR >> gpio_pin_num(pin)) & 1;
}

#endif
