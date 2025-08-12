/*
 * Multiboot header - follows spec exactly
 */

#ifndef _BLOOD_MULTIBOOT_H
#define _BLOOD_MULTIBOOT_H

#define MULTIBOOT_MAGIC 0x1BADB002
#define MULTIBOOT_FLAGS 0x00000003

struct multiboot_header {
    u32 magic;
    u32 flags;
    u32 checksum;
};

// This goes in .multiboot section
__attribute__((section(".multiboot")))
struct multiboot_header blood_multiboot = {
    .magic = MULTIBOOT_MAGIC,
    .flags = MULTIBOOT_FLAGS,
    .checksum = -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
};

#endif
