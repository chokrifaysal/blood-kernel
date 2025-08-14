/*
 * smbios.c – System Management BIOS
 */

#include "kernel/types.h"

#define SMBIOS_ANCHOR "_SM_"
#define SMBIOS3_ANCHOR "_SM3_"

/* SMBIOS structure types */
#define SMBIOS_TYPE_BIOS        0
#define SMBIOS_TYPE_SYSTEM      1
#define SMBIOS_TYPE_BASEBOARD   2
#define SMBIOS_TYPE_CHASSIS     3
#define SMBIOS_TYPE_PROCESSOR   4
#define SMBIOS_TYPE_MEMORY_CTRL 5
#define SMBIOS_TYPE_MEMORY_MOD  6
#define SMBIOS_TYPE_CACHE       7
#define SMBIOS_TYPE_PORT_CONN   8
#define SMBIOS_TYPE_SYSTEM_SLOT 9
#define SMBIOS_TYPE_OEM_STRINGS 11
#define SMBIOS_TYPE_SYS_CONFIG  12
#define SMBIOS_TYPE_BIOS_LANG   13
#define SMBIOS_TYPE_GROUP_ASSOC 14
#define SMBIOS_TYPE_SYS_EVENT   15
#define SMBIOS_TYPE_PHYS_MEM    16
#define SMBIOS_TYPE_MEM_DEVICE  17
#define SMBIOS_TYPE_MEM_ERROR   18
#define SMBIOS_TYPE_MEM_ARRAY_MAP 19
#define SMBIOS_TYPE_MEM_DEV_MAP 20
#define SMBIOS_TYPE_BUILTIN_POINT 21
#define SMBIOS_TYPE_PORTABLE_BAT 22
#define SMBIOS_TYPE_SYS_RESET   23
#define SMBIOS_TYPE_HW_SECURITY 24
#define SMBIOS_TYPE_SYS_POWER   25
#define SMBIOS_TYPE_VOLTAGE     26
#define SMBIOS_TYPE_COOLING     27
#define SMBIOS_TYPE_TEMP_PROBE  28
#define SMBIOS_TYPE_CURRENT     29
#define SMBIOS_TYPE_OOB_REMOTE  30
#define SMBIOS_TYPE_BIS_ENTRY   31
#define SMBIOS_TYPE_SYS_BOOT    32
#define SMBIOS_TYPE_MGMT_DEV    34
#define SMBIOS_TYPE_MGMT_DEV_COMP 35
#define SMBIOS_TYPE_MGMT_DEV_THRESH 36
#define SMBIOS_TYPE_MEM_CHANNEL 37
#define SMBIOS_TYPE_IPMI_DEV    38
#define SMBIOS_TYPE_SYS_POWER_SUPPLY 39
#define SMBIOS_TYPE_ADDITIONAL  40
#define SMBIOS_TYPE_ONBOARD_DEV 41
#define SMBIOS_TYPE_MGMT_CTRL   42
#define SMBIOS_TYPE_MGMT_CTRL_HOST 43
#define SMBIOS_TYPE_TPM_DEV     43
#define SMBIOS_TYPE_INACTIVE    126
#define SMBIOS_TYPE_END_OF_TABLE 127

typedef struct {
    char anchor[4];
    u8 checksum;
    u8 length;
    u8 major_version;
    u8 minor_version;
    u16 max_structure_size;
    u8 entry_point_revision;
    u8 formatted_area[5];
    char intermediate_anchor[5];
    u8 intermediate_checksum;
    u16 structure_table_length;
    u32 structure_table_address;
    u16 number_of_structures;
    u8 bcd_revision;
} __attribute__((packed)) smbios_entry_point_t;

typedef struct {
    char anchor[5];
    u8 checksum;
    u8 length;
    u8 major_version;
    u8 minor_version;
    u8 docrev;
    u8 entry_point_revision;
    u8 reserved;
    u32 structure_table_max_size;
    u64 structure_table_address;
} __attribute__((packed)) smbios3_entry_point_t;

typedef struct {
    u8 type;
    u8 length;
    u16 handle;
} __attribute__((packed)) smbios_header_t;

typedef struct {
    smbios_header_t header;
    u8 vendor;
    u8 bios_version;
    u16 bios_starting_segment;
    u8 bios_release_date;
    u8 bios_rom_size;
    u64 bios_characteristics;
    u8 bios_characteristics_ext[2];
    u8 system_bios_major_release;
    u8 system_bios_minor_release;
    u8 embedded_controller_major;
    u8 embedded_controller_minor;
} __attribute__((packed)) smbios_bios_info_t;

typedef struct {
    smbios_header_t header;
    u8 manufacturer;
    u8 product_name;
    u8 version;
    u8 serial_number;
    u8 uuid[16];
    u8 wake_up_type;
    u8 sku_number;
    u8 family;
} __attribute__((packed)) smbios_system_info_t;

typedef struct {
    smbios_header_t header;
    u8 socket_designation;
    u8 processor_type;
    u8 processor_family;
    u8 processor_manufacturer;
    u64 processor_id;
    u8 processor_version;
    u8 voltage;
    u16 external_clock;
    u16 max_speed;
    u16 current_speed;
    u8 status;
    u8 processor_upgrade;
    u16 l1_cache_handle;
    u16 l2_cache_handle;
    u16 l3_cache_handle;
    u8 serial_number;
    u8 asset_tag;
    u8 part_number;
    u8 core_count;
    u8 core_enabled;
    u8 thread_count;
    u16 processor_characteristics;
    u16 processor_family2;
} __attribute__((packed)) smbios_processor_info_t;

static smbios_entry_point_t* smbios_entry = 0;
static smbios3_entry_point_t* smbios3_entry = 0;
static u8* smbios_table = 0;
static u16 smbios_table_length = 0;
static u8 smbios_version_major = 0;
static u8 smbios_version_minor = 0;

static u8 smbios_checksum(void* ptr, u32 length) {
    u8* data = (u8*)ptr;
    u8 sum = 0;
    for (u32 i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

static smbios_entry_point_t* smbios_find_entry_point(void) {
    /* Search in EBDA */
    u16 ebda = *(u16*)0x40E;
    if (ebda) {
        ebda <<= 4;
        for (u32 addr = ebda; addr < ebda + 1024; addr += 16) {
            if (*(u32*)addr == *(u32*)SMBIOS_ANCHOR) {
                smbios_entry_point_t* entry = (smbios_entry_point_t*)addr;
                if (smbios_checksum(entry, entry->length) == 0) {
                    return entry;
                }
            }
        }
    }
    
    /* Search in BIOS area */
    for (u32 addr = 0xF0000; addr < 0x100000; addr += 16) {
        if (*(u32*)addr == *(u32*)SMBIOS_ANCHOR) {
            smbios_entry_point_t* entry = (smbios_entry_point_t*)addr;
            if (entry->length >= 0x1F && smbios_checksum(entry, entry->length) == 0) {
                return entry;
            }
        }
        
        /* Check for SMBIOS 3.0 */
        if (*(u32*)addr == *(u32*)SMBIOS3_ANCHOR && *(u8*)(addr + 4) == '_') {
            smbios3_entry_point_t* entry3 = (smbios3_entry_point_t*)addr;
            if (smbios_checksum(entry3, entry3->length) == 0) {
                smbios3_entry = entry3;
            }
        }
    }
    
    return 0;
}

void smbios_init(void) {
    smbios_entry = smbios_find_entry_point();
    
    if (smbios_entry) {
        smbios_table = (u8*)smbios_entry->structure_table_address;
        smbios_table_length = smbios_entry->structure_table_length;
        smbios_version_major = smbios_entry->major_version;
        smbios_version_minor = smbios_entry->minor_version;
    } else if (smbios3_entry) {
        smbios_table = (u8*)smbios3_entry->structure_table_address;
        smbios_table_length = smbios3_entry->structure_table_max_size;
        smbios_version_major = smbios3_entry->major_version;
        smbios_version_minor = smbios3_entry->minor_version;
    }
}

static const char* smbios_get_string(smbios_header_t* header, u8 string_number) {
    if (string_number == 0) return "";
    
    u8* strings = (u8*)header + header->length;
    u8 current_string = 1;
    
    while (*strings != 0) {
        if (current_string == string_number) {
            return (const char*)strings;
        }
        
        while (*strings != 0) strings++;
        strings++;
        current_string++;
    }
    
    return "";
}

smbios_header_t* smbios_find_structure(u8 type, u8 instance) {
    if (!smbios_table) return 0;
    
    u8* current = smbios_table;
    u8* end = smbios_table + smbios_table_length;
    u8 found_instances = 0;
    
    while (current < end) {
        smbios_header_t* header = (smbios_header_t*)current;
        
        if (header->type == type) {
            if (found_instances == instance) {
                return header;
            }
            found_instances++;
        }
        
        if (header->type == SMBIOS_TYPE_END_OF_TABLE) {
            break;
        }
        
        /* Skip to next structure */
        current += header->length;
        
        /* Skip strings */
        while (current < end && (*current != 0 || *(current + 1) != 0)) {
            current++;
        }
        current += 2; /* Skip double null terminator */
    }
    
    return 0;
}

const char* smbios_get_bios_vendor(void) {
    smbios_bios_info_t* bios = (smbios_bios_info_t*)smbios_find_structure(SMBIOS_TYPE_BIOS, 0);
    if (!bios) return "";
    
    return smbios_get_string(&bios->header, bios->vendor);
}

const char* smbios_get_bios_version(void) {
    smbios_bios_info_t* bios = (smbios_bios_info_t*)smbios_find_structure(SMBIOS_TYPE_BIOS, 0);
    if (!bios) return "";
    
    return smbios_get_string(&bios->header, bios->bios_version);
}

const char* smbios_get_system_manufacturer(void) {
    smbios_system_info_t* system = (smbios_system_info_t*)smbios_find_structure(SMBIOS_TYPE_SYSTEM, 0);
    if (!system) return "";
    
    return smbios_get_string(&system->header, system->manufacturer);
}

const char* smbios_get_system_product(void) {
    smbios_system_info_t* system = (smbios_system_info_t*)smbios_find_structure(SMBIOS_TYPE_SYSTEM, 0);
    if (!system) return "";
    
    return smbios_get_string(&system->header, system->product_name);
}

const char* smbios_get_system_version(void) {
    smbios_system_info_t* system = (smbios_system_info_t*)smbios_find_structure(SMBIOS_TYPE_SYSTEM, 0);
    if (!system) return "";
    
    return smbios_get_string(&system->header, system->version);
}

const char* smbios_get_system_serial(void) {
    smbios_system_info_t* system = (smbios_system_info_t*)smbios_find_structure(SMBIOS_TYPE_SYSTEM, 0);
    if (!system) return "";
    
    return smbios_get_string(&system->header, system->serial_number);
}

void smbios_get_system_uuid(u8* uuid) {
    smbios_system_info_t* system = (smbios_system_info_t*)smbios_find_structure(SMBIOS_TYPE_SYSTEM, 0);
    if (!system || !uuid) return;
    
    for (u8 i = 0; i < 16; i++) {
        uuid[i] = system->uuid[i];
    }
}

u16 smbios_get_processor_speed(u8 processor) {
    smbios_processor_info_t* proc = (smbios_processor_info_t*)smbios_find_structure(SMBIOS_TYPE_PROCESSOR, processor);
    if (!proc) return 0;
    
    return proc->current_speed;
}

const char* smbios_get_processor_manufacturer(u8 processor) {
    smbios_processor_info_t* proc = (smbios_processor_info_t*)smbios_find_structure(SMBIOS_TYPE_PROCESSOR, processor);
    if (!proc) return "";
    
    return smbios_get_string(&proc->header, proc->processor_manufacturer);
}

const char* smbios_get_processor_version(u8 processor) {
    smbios_processor_info_t* proc = (smbios_processor_info_t*)smbios_find_structure(SMBIOS_TYPE_PROCESSOR, processor);
    if (!proc) return "";
    
    return smbios_get_string(&proc->header, proc->processor_version);
}

u8 smbios_get_processor_core_count(u8 processor) {
    smbios_processor_info_t* proc = (smbios_processor_info_t*)smbios_find_structure(SMBIOS_TYPE_PROCESSOR, processor);
    if (!proc) return 0;
    
    return proc->core_count;
}

u8 smbios_get_processor_thread_count(u8 processor) {
    smbios_processor_info_t* proc = (smbios_processor_info_t*)smbios_find_structure(SMBIOS_TYPE_PROCESSOR, processor);
    if (!proc) return 0;
    
    return proc->thread_count;
}

u8 smbios_is_available(void) {
    return (smbios_entry != 0) || (smbios3_entry != 0);
}

u8 smbios_get_version_major(void) {
    return smbios_version_major;
}

u8 smbios_get_version_minor(void) {
    return smbios_version_minor;
}
