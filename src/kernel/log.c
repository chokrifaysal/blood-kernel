/*
 * log.c - append-only ELM log on SD
 */

#include "kernel/log.h"
#include "kernel/sd.h"
#include "kernel/timer.h"
#include "kernel/types.h"

static u32 log_block = 1;   // skip MBR
static u8  log_buf[SD_BLOCK_SIZE];
static u32 log_idx = 0;

void log_init(void) {
    sd_init();
    log_flush();   // write empty block
}

void log_can(const can_frame_t* f) {
    log_entry_t* e = (log_entry_t*)(log_buf + log_idx);
    e->magic    = LOG_MAGIC;
    e->timestamp = timer_ticks();
    e->id       = f->id;
    memcpy(e->data, f->data, 8);
    
    log_idx += sizeof(log_entry_t);
    if (log_idx + sizeof(log_entry_t) > SD_BLOCK_SIZE) {
        log_flush();
    }
}

void log_flush(void) {
    if (log_idx) {
        sd_write(log_block++, log_buf);
        log_idx = 0;
    }
}
