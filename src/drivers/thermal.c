/*
 * thermal.c – x86 thermal management and monitoring
 */

#include "kernel/types.h"

/* Thermal MSRs */
#define MSR_THERM_INTERRUPT     0x19B
#define MSR_THERM_STATUS        0x19C
#define MSR_THERM2_CTL          0x19D
#define MSR_MISC_ENABLE         0x1A0
#define MSR_TEMPERATURE_TARGET  0x1A2
#define MSR_PACKAGE_THERM_STATUS 0x1B1
#define MSR_PACKAGE_THERM_INTERRUPT 0x1B2

/* Thermal status bits */
#define THERM_STATUS_THERMAL_TRIP    0x01
#define THERM_STATUS_THERMAL_TRIP_LOG 0x02
#define THERM_STATUS_PROCHOT         0x04
#define THERM_STATUS_PROCHOT_LOG     0x08
#define THERM_STATUS_CRIT_TEMP       0x10
#define THERM_STATUS_CRIT_TEMP_LOG   0x20
#define THERM_STATUS_TEMP1           0x40
#define THERM_STATUS_TEMP1_LOG       0x80
#define THERM_STATUS_TEMP2           0x100
#define THERM_STATUS_TEMP2_LOG       0x200
#define THERM_STATUS_POWER_LIMIT     0x400
#define THERM_STATUS_POWER_LIMIT_LOG 0x800
#define THERM_STATUS_READING_VALID   0x80000000

/* Thermal interrupt bits */
#define THERM_INT_HIGH_TEMP_ENABLE   0x01
#define THERM_INT_LOW_TEMP_ENABLE    0x02
#define THERM_INT_PROCHOT_ENABLE     0x04
#define THERM_INT_FORCEPR_ENABLE     0x08
#define THERM_INT_CRIT_TEMP_ENABLE   0x10
#define THERM_INT_TEMP1_ENABLE       0x100
#define THERM_INT_TEMP2_ENABLE       0x200
#define THERM_INT_POWER_LIMIT_ENABLE 0x400

typedef struct {
    u8 supported;
    u8 digital_readout;
    u8 package_thermal;
    u8 interrupt_enabled;
    u32 tjmax;
    u32 temp_threshold1;
    u32 temp_threshold2;
    u32 last_temperature;
    u32 max_temperature;
    u32 thermal_events;
} thermal_info_t;

static thermal_info_t thermal_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 thermal_check_support(void) {
    if (!msr_is_supported()) return 0;
    
    /* Check CPUID for thermal monitoring */
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (edx & (1 << 22)) != 0; /* Thermal monitoring */
}

static u8 thermal_check_digital_readout(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(6));
    
    return (eax & (1 << 0)) != 0; /* Digital temperature sensor */
}

static u32 thermal_get_tjmax(void) {
    if (!msr_is_supported()) return 100; /* Default fallback */
    
    u64 temp_target = msr_read(MSR_TEMPERATURE_TARGET);
    u32 tjmax = (temp_target >> 16) & 0xFF;
    
    if (tjmax == 0) {
        /* Fallback values for common processors */
        u32 eax, ebx, ecx, edx;
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
        
        u32 model = (eax >> 4) & 0xF;
        u32 family = (eax >> 8) & 0xF;
        
        if (family == 6) {
            switch (model) {
                case 0x1A: case 0x1E: case 0x1F: /* Nehalem */
                case 0x25: case 0x2C: case 0x2F: /* Westmere */
                    tjmax = 100;
                    break;
                case 0x2A: case 0x2D: /* Sandy Bridge */
                case 0x3A: case 0x3E: /* Ivy Bridge */
                    tjmax = 105;
                    break;
                default:
                    tjmax = 100;
                    break;
            }
        } else {
            tjmax = 100;
        }
    }
    
    return tjmax;
}

void thermal_init(void) {
    thermal_info.supported = thermal_check_support();
    if (!thermal_info.supported) return;
    
    thermal_info.digital_readout = thermal_check_digital_readout();
    thermal_info.tjmax = thermal_get_tjmax();
    thermal_info.temp_threshold1 = thermal_info.tjmax - 10; /* 10°C below TjMax */
    thermal_info.temp_threshold2 = thermal_info.tjmax - 20; /* 20°C below TjMax */
    thermal_info.last_temperature = 0;
    thermal_info.max_temperature = 0;
    thermal_info.thermal_events = 0;
    
    /* Check for package thermal monitoring */
    u64 pkg_status = msr_read(MSR_PACKAGE_THERM_STATUS);
    thermal_info.package_thermal = (pkg_status != 0);
    
    /* Enable thermal interrupts */
    if (thermal_info.digital_readout) {
        u64 therm_int = THERM_INT_HIGH_TEMP_ENABLE | THERM_INT_CRIT_TEMP_ENABLE;
        msr_write(MSR_THERM_INTERRUPT, therm_int);
        thermal_info.interrupt_enabled = 1;
    }
}

u32 thermal_get_temperature(void) {
    if (!thermal_info.supported || !thermal_info.digital_readout) {
        return 0;
    }
    
    u64 therm_status = msr_read(MSR_THERM_STATUS);
    
    if (!(therm_status & THERM_STATUS_READING_VALID)) {
        return thermal_info.last_temperature;
    }
    
    /* Extract digital readout */
    u32 digital_readout = (therm_status >> 16) & 0x7F;
    u32 temperature = thermal_info.tjmax - digital_readout;
    
    thermal_info.last_temperature = temperature;
    
    if (temperature > thermal_info.max_temperature) {
        thermal_info.max_temperature = temperature;
    }
    
    return temperature;
}

u32 thermal_get_package_temperature(void) {
    if (!thermal_info.supported || !thermal_info.package_thermal) {
        return 0;
    }
    
    u64 pkg_status = msr_read(MSR_PACKAGE_THERM_STATUS);
    
    if (!(pkg_status & THERM_STATUS_READING_VALID)) {
        return 0;
    }
    
    u32 digital_readout = (pkg_status >> 16) & 0x7F;
    return thermal_info.tjmax - digital_readout;
}

u8 thermal_is_supported(void) {
    return thermal_info.supported;
}

u8 thermal_has_digital_sensor(void) {
    return thermal_info.digital_readout;
}

u32 thermal_get_tjmax(void) {
    return thermal_info.tjmax;
}

u32 thermal_get_max_temperature(void) {
    return thermal_info.max_temperature;
}

u8 thermal_is_throttling(void) {
    if (!thermal_info.supported) return 0;
    
    u64 therm_status = msr_read(MSR_THERM_STATUS);
    return (therm_status & THERM_STATUS_PROCHOT) != 0;
}

u8 thermal_is_critical(void) {
    if (!thermal_info.supported) return 0;
    
    u64 therm_status = msr_read(MSR_THERM_STATUS);
    return (therm_status & THERM_STATUS_CRIT_TEMP) != 0;
}

void thermal_set_threshold(u8 threshold_num, u32 temperature) {
    if (!thermal_info.supported || !thermal_info.digital_readout) return;
    
    if (temperature > thermal_info.tjmax) {
        temperature = thermal_info.tjmax;
    }
    
    u32 digital_value = thermal_info.tjmax - temperature;
    
    u64 therm_int = msr_read(MSR_THERM_INTERRUPT);
    
    if (threshold_num == 1) {
        therm_int = (therm_int & ~(0x7F << 8)) | ((digital_value & 0x7F) << 8);
        therm_int |= THERM_INT_TEMP1_ENABLE;
        thermal_info.temp_threshold1 = temperature;
    } else if (threshold_num == 2) {
        therm_int = (therm_int & ~(0x7F << 16)) | ((digital_value & 0x7F) << 16);
        therm_int |= THERM_INT_TEMP2_ENABLE;
        thermal_info.temp_threshold2 = temperature;
    }
    
    msr_write(MSR_THERM_INTERRUPT, therm_int);
}

u32 thermal_get_threshold(u8 threshold_num) {
    if (threshold_num == 1) {
        return thermal_info.temp_threshold1;
    } else if (threshold_num == 2) {
        return thermal_info.temp_threshold2;
    }
    return 0;
}

void thermal_clear_log_bits(void) {
    if (!thermal_info.supported) return;
    
    u64 therm_status = msr_read(MSR_THERM_STATUS);
    
    /* Clear log bits by writing 1 to them */
    therm_status |= THERM_STATUS_THERMAL_TRIP_LOG | THERM_STATUS_PROCHOT_LOG |
                   THERM_STATUS_CRIT_TEMP_LOG | THERM_STATUS_TEMP1_LOG |
                   THERM_STATUS_TEMP2_LOG | THERM_STATUS_POWER_LIMIT_LOG;
    
    msr_write(MSR_THERM_STATUS, therm_status);
    
    if (thermal_info.package_thermal) {
        u64 pkg_status = msr_read(MSR_PACKAGE_THERM_STATUS);
        pkg_status |= THERM_STATUS_THERMAL_TRIP_LOG | THERM_STATUS_PROCHOT_LOG |
                     THERM_STATUS_CRIT_TEMP_LOG | THERM_STATUS_TEMP1_LOG |
                     THERM_STATUS_TEMP2_LOG | THERM_STATUS_POWER_LIMIT_LOG;
        msr_write(MSR_PACKAGE_THERM_STATUS, pkg_status);
    }
}

void thermal_irq_handler(void) {
    if (!thermal_info.supported) return;
    
    thermal_info.thermal_events++;
    
    u64 therm_status = msr_read(MSR_THERM_STATUS);
    
    if (therm_status & THERM_STATUS_CRIT_TEMP) {
        /* Critical temperature reached - emergency shutdown */
        extern void acpi_power_off(void);
        acpi_power_off();
    }
    
    if (therm_status & THERM_STATUS_THERMAL_TRIP) {
        /* Thermal trip point reached */
    }
    
    if (therm_status & THERM_STATUS_PROCHOT) {
        /* Processor hot - throttling active */
    }
    
    /* Clear interrupt by reading status */
    thermal_clear_log_bits();
}

u32 thermal_get_event_count(void) {
    return thermal_info.thermal_events;
}

void thermal_enable_monitoring(u8 enable) {
    if (!thermal_info.supported || !thermal_info.digital_readout) return;
    
    u64 therm_int = msr_read(MSR_THERM_INTERRUPT);
    
    if (enable) {
        therm_int |= THERM_INT_HIGH_TEMP_ENABLE | THERM_INT_CRIT_TEMP_ENABLE;
    } else {
        therm_int &= ~(THERM_INT_HIGH_TEMP_ENABLE | THERM_INT_CRIT_TEMP_ENABLE);
    }
    
    msr_write(MSR_THERM_INTERRUPT, therm_int);
    thermal_info.interrupt_enabled = enable;
}

u8 thermal_is_monitoring_enabled(void) {
    return thermal_info.interrupt_enabled;
}
