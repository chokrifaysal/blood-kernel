/*
 * ac97.h – x86 AC97 audio codec controller
 */

#ifndef AC97_H
#define AC97_H

#include "kernel/types.h"

/* Core functions */
u8 ac97_init(u16 nambar, u16 nabmbar, u8 irq);
u8 ac97_is_initialized(void);

/* Playback functions */
void ac97_play_buffer(const void* buffer, u32 size);
void ac97_stop_playback(void);
u8 ac97_is_playing(void);

/* Audio control */
void ac97_set_volume(u8 left, u8 right);
void ac97_generate_tone(u16 frequency, u32 duration_ms);

/* Interrupt handler */
void ac97_irq_handler(void);

#endif
