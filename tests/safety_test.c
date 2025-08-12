/*
 * safety_test.c - automated safety regression
 * Runs in task context, triggers on demand via CAN 0x7FF
 */

#include "kernel/types.h"
#include "kernel/can.h"
#include "kernel/timer.h"
#include "kernel/sched.h"

static void test_stack_overflow(void) {
    u8 huge[4096];
    memset(huge, 0xAA, sizeof(huge));   // should trigger canary
}

static void test_watchdog_timeout(void) {
    while (1);   // no refresh -> reset
}

void safety_test_task(void) {
    can_frame_t rx;
    while (1) {
        if (can_recv(&rx) == 0 && rx.id == 0x7FF) {
            switch (rx.data[0]) {
                case 0x01: test_stack_overflow(); break;
                case 0x02: test_watchdog_timeout(); break;
            }
        }
        timer_delay(100);
    }
}
