/*
 * log.h - ELM-compatible automotive log format
 */

#ifndef _BLOOD_LOG_H
#define _BLOOD_LOG_H

#include "kernel/types.h"

#define LOG_MAGIC 0xB10DDEAD
#define LOG_ENTRY_SIZE 32

typedef struct {
    u32 magic;
    u32 timestamp;
    u32 id;
    u8  data[8];
} log_entry_t;

void log_init(void);
void log_can(const can_frame_t* f);
void log_flush(void);

#endif
