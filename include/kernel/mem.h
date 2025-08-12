/*
 * mem.h - memory detection & early mapping
 */

#ifndef _BLOOD_MEM_H
#define _BLOOD_MEM_H

#include "kernel/types.h"

void detect_memory_x86(u32 mbi_addr);
void detect_memory_arm(void);

#endif
