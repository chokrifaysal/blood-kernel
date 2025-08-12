/*
 * context.h - arch-specific context switch
 */

#ifndef _BLOOD_CONTEXT_H
#define _BLOOD_CONTEXT_H

#include "kernel/types.h"

void context_switch(u32** old_sp, u32* new_sp);

#endif
