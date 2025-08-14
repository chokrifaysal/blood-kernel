/*
 * smbios.h – System Management BIOS
 */

#ifndef SMBIOS_H
#define SMBIOS_H

#include "kernel/types.h"

/* Core functions */
void smbios_init(void);
u8 smbios_is_available(void);
u8 smbios_get_version_major(void);
u8 smbios_get_version_minor(void);

/* BIOS information */
const char* smbios_get_bios_vendor(void);
const char* smbios_get_bios_version(void);

/* System information */
const char* smbios_get_system_manufacturer(void);
const char* smbios_get_system_product(void);
const char* smbios_get_system_version(void);
const char* smbios_get_system_serial(void);
void smbios_get_system_uuid(u8* uuid);

/* Processor information */
u16 smbios_get_processor_speed(u8 processor);
const char* smbios_get_processor_manufacturer(u8 processor);
const char* smbios_get_processor_version(u8 processor);
u8 smbios_get_processor_core_count(u8 processor);
u8 smbios_get_processor_thread_count(u8 processor);

/* Low-level access */
typedef struct {
    u8 type;
    u8 length;
    u16 handle;
} smbios_header_t;

smbios_header_t* smbios_find_structure(u8 type, u8 instance);

#endif
