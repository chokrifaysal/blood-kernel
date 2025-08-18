/*
 * thermal_mgmt.c – x86 CPU thermal management (thermal throttling/RAPL)
 */

#include "kernel/types.h"

/* Thermal MSRs */
#define MSR_IA32_THERM_STATUS           0x19C
#define MSR_IA32_THERM_INTERRUPT        0x19B
#define MSR_IA32_PACKAGE_THERM_STATUS   0x1B1
#define MSR_IA32_PACKAGE_THERM_INTERRUPT 0x1B0
#define MSR_IA32_TEMPERATURE_TARGET     0x1A2

/* RAPL (Running Average Power Limit) MSRs */
#define MSR_RAPL_POWER_UNIT             0x606
#define MSR_PKG_POWER_LIMIT             0x610
#define MSR_PKG_ENERGY_STATUS           0x611
#define MSR_PKG_PERF_STATUS             0x613
#define MSR_PKG_POWER_INFO              0x614
#define MSR_DRAM_POWER_LIMIT            0x618
#define MSR_DRAM_ENERGY_STATUS          0x619
#define MSR_DRAM_PERF_STATUS            0x61B
#define MSR_DRAM_POWER_INFO             0x61C
#define MSR_PP0_POWER_LIMIT             0x638
#define MSR_PP0_ENERGY_STATUS           0x639
#define MSR_PP0_POLICY                  0x63A
#define MSR_PP0_PERF_STATUS             0x63B
#define MSR_PP1_POWER_LIMIT             0x640
#define MSR_PP1_ENERGY_STATUS           0x641
#define MSR_PP1_POLICY                  0x642

/* Thermal status bits */
#define THERM_STATUS_THERMAL_STATUS     (1 << 0)
#define THERM_STATUS_THERMAL_STATUS_LOG (1 << 1)
#define THERM_STATUS_PROCHOT            (1 << 2)
#define THERM_STATUS_PROCHOT_LOG        (1 << 3)
#define THERM_STATUS_CRIT_TEMP          (1 << 4)
#define THERM_STATUS_CRIT_TEMP_LOG      (1 << 5)
#define THERM_STATUS_THERMAL1           (1 << 6)
#define THERM_STATUS_THERMAL1_LOG       (1 << 7)
#define THERM_STATUS_THERMAL2           (1 << 8)
#define THERM_STATUS_THERMAL2_LOG       (1 << 9)
#define THERM_STATUS_POWER_LIMIT        (1 << 10)
#define THERM_STATUS_POWER_LIMIT_LOG    (1 << 11)
#define THERM_STATUS_READING_VALID      (1 << 31)

/* Power limit control bits */
#define POWER_LIMIT_ENABLE              (1ULL << 15)
#define POWER_LIMIT_CLAMP               (1ULL << 16)
#define POWER_LIMIT_TIME_WINDOW_MASK    0x7F
#define POWER_LIMIT_TIME_WINDOW_SHIFT   17

/* Thermal event types */
#define THERMAL_EVENT_THRESHOLD         0
#define THERMAL_EVENT_PROCHOT           1
#define THERMAL_EVENT_CRITICAL          2
#define THERMAL_EVENT_POWER_LIMIT       3

typedef struct {
    u8 event_type;
    u64 timestamp;
    u32 temperature;
    u32 cpu_id;
    u32 duration_ms;
} thermal_event_t;

typedef struct {
    u32 power_units;
    u32 energy_units;
    u32 time_units;
    u64 pkg_power_limit;
    u64 pkg_energy_consumed;
    u64 dram_energy_consumed;
    u64 pp0_energy_consumed;
    u64 pp1_energy_consumed;
    u32 pkg_power_limit_1;
    u32 pkg_power_limit_2;
    u32 pkg_time_window_1;
    u32 pkg_time_window_2;
    u8 pkg_power_limit_enabled;
    u8 dram_power_limit_enabled;
} rapl_info_t;

typedef struct {
    u8 thermal_mgmt_supported;
    u8 thermal_monitoring_supported;
    u8 rapl_supported;
    u32 current_temperature;
    u32 max_temperature;
    u32 critical_temperature;
    u32 thermal_target;
    u8 thermal_throttling_active;
    u8 prochot_active;
    u8 critical_temp_reached;
    u32 thermal_events_count;
    thermal_event_t thermal_events[32];
    u32 thermal_event_index;
    rapl_info_t rapl;
    u32 throttle_count;
    u64 total_throttle_time;
    u64 last_thermal_event;
    u32 thermal_interrupts;
    u32 power_limit_violations;
} thermal_mgmt_info_t;

static thermal_mgmt_info_t thermal_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);
extern u32 cpu_get_current_id(void);

static void thermal_mgmt_detect_thermal_monitoring(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for thermal monitoring support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    thermal_info.thermal_monitoring_supported = (edx & (1 << 22)) != 0;
    
    if (thermal_info.thermal_monitoring_supported && msr_is_supported()) {
        /* Read thermal target */
        u64 temp_target = msr_read(MSR_IA32_TEMPERATURE_TARGET);
        thermal_info.thermal_target = (temp_target >> 16) & 0xFF;
        thermal_info.critical_temperature = thermal_info.thermal_target + 20;  /* Estimate */
        thermal_info.max_temperature = thermal_info.thermal_target + 10;
    }
}

static void thermal_mgmt_detect_rapl(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for RAPL support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(6));
    
    thermal_info.rapl_supported = (eax & (1 << 14)) != 0;
    
    if (thermal_info.rapl_supported && msr_is_supported()) {
        /* Read RAPL power units */
        u64 power_unit = msr_read(MSR_RAPL_POWER_UNIT);
        thermal_info.rapl.power_units = 1 << (power_unit & 0xF);
        thermal_info.rapl.energy_units = 1 << ((power_unit >> 8) & 0x1F);
        thermal_info.rapl.time_units = 1 << ((power_unit >> 16) & 0xF);
        
        /* Read current power limits */
        thermal_info.rapl.pkg_power_limit = msr_read(MSR_PKG_POWER_LIMIT);
        thermal_info.rapl.pkg_power_limit_1 = thermal_info.rapl.pkg_power_limit & 0x7FFF;
        thermal_info.rapl.pkg_power_limit_2 = (thermal_info.rapl.pkg_power_limit >> 32) & 0x7FFF;
        thermal_info.rapl.pkg_power_limit_enabled = (thermal_info.rapl.pkg_power_limit & POWER_LIMIT_ENABLE) != 0;
        
        /* Read initial energy values */
        thermal_info.rapl.pkg_energy_consumed = msr_read(MSR_PKG_ENERGY_STATUS);
        thermal_info.rapl.dram_energy_consumed = msr_read(MSR_DRAM_ENERGY_STATUS);
        thermal_info.rapl.pp0_energy_consumed = msr_read(MSR_PP0_ENERGY_STATUS);
        thermal_info.rapl.pp1_energy_consumed = msr_read(MSR_PP1_ENERGY_STATUS);
    }
}

void thermal_mgmt_init(void) {
    thermal_info.thermal_mgmt_supported = 0;
    thermal_info.thermal_monitoring_supported = 0;
    thermal_info.rapl_supported = 0;
    thermal_info.current_temperature = 0;
    thermal_info.max_temperature = 0;
    thermal_info.critical_temperature = 0;
    thermal_info.thermal_target = 0;
    thermal_info.thermal_throttling_active = 0;
    thermal_info.prochot_active = 0;
    thermal_info.critical_temp_reached = 0;
    thermal_info.thermal_events_count = 0;
    thermal_info.thermal_event_index = 0;
    thermal_info.throttle_count = 0;
    thermal_info.total_throttle_time = 0;
    thermal_info.last_thermal_event = 0;
    thermal_info.thermal_interrupts = 0;
    thermal_info.power_limit_violations = 0;
    
    for (u32 i = 0; i < 32; i++) {
        thermal_info.thermal_events[i].event_type = 0;
        thermal_info.thermal_events[i].timestamp = 0;
        thermal_info.thermal_events[i].temperature = 0;
        thermal_info.thermal_events[i].cpu_id = 0;
        thermal_info.thermal_events[i].duration_ms = 0;
    }
    
    /* Initialize RAPL */
    thermal_info.rapl.power_units = 0;
    thermal_info.rapl.energy_units = 0;
    thermal_info.rapl.time_units = 0;
    thermal_info.rapl.pkg_power_limit = 0;
    thermal_info.rapl.pkg_energy_consumed = 0;
    thermal_info.rapl.dram_energy_consumed = 0;
    thermal_info.rapl.pp0_energy_consumed = 0;
    thermal_info.rapl.pp1_energy_consumed = 0;
    thermal_info.rapl.pkg_power_limit_1 = 0;
    thermal_info.rapl.pkg_power_limit_2 = 0;
    thermal_info.rapl.pkg_time_window_1 = 0;
    thermal_info.rapl.pkg_time_window_2 = 0;
    thermal_info.rapl.pkg_power_limit_enabled = 0;
    thermal_info.rapl.dram_power_limit_enabled = 0;
    
    thermal_mgmt_detect_thermal_monitoring();
    thermal_mgmt_detect_rapl();
    
    if (thermal_info.thermal_monitoring_supported || thermal_info.rapl_supported) {
        thermal_info.thermal_mgmt_supported = 1;
    }
}

u8 thermal_mgmt_is_supported(void) {
    return thermal_info.thermal_mgmt_supported;
}

u32 thermal_mgmt_read_temperature(void) {
    if (!thermal_info.thermal_monitoring_supported || !msr_is_supported()) return 0;
    
    u64 therm_status = msr_read(MSR_IA32_THERM_STATUS);
    
    if (therm_status & THERM_STATUS_READING_VALID) {
        u32 digital_readout = (therm_status >> 16) & 0x7F;
        thermal_info.current_temperature = thermal_info.thermal_target - digital_readout;
        
        /* Update thermal status flags */
        thermal_info.thermal_throttling_active = (therm_status & THERM_STATUS_THERMAL_STATUS) != 0;
        thermal_info.prochot_active = (therm_status & THERM_STATUS_PROCHOT) != 0;
        thermal_info.critical_temp_reached = (therm_status & THERM_STATUS_CRIT_TEMP) != 0;
        
        return thermal_info.current_temperature;
    }
    
    return 0;
}

u8 thermal_mgmt_enable_thermal_interrupt(u32 threshold_temperature) {
    if (!thermal_info.thermal_monitoring_supported || !msr_is_supported()) return 0;
    
    u64 therm_interrupt = msr_read(MSR_IA32_THERM_INTERRUPT);
    
    /* Enable thermal interrupt */
    therm_interrupt |= (1 << 0);  /* High temperature interrupt enable */
    therm_interrupt |= (1 << 1);  /* Low temperature interrupt enable */
    therm_interrupt |= (1 << 2);  /* PROCHOT interrupt enable */
    therm_interrupt |= (1 << 4);  /* Critical temperature interrupt enable */
    
    /* Set threshold */
    u32 threshold_offset = thermal_info.thermal_target - threshold_temperature;
    therm_interrupt &= ~(0x7F << 8);
    therm_interrupt |= (threshold_offset & 0x7F) << 8;
    
    msr_write(MSR_IA32_THERM_INTERRUPT, therm_interrupt);
    
    return 1;
}

u8 thermal_mgmt_set_power_limit(u32 power_limit_watts, u32 time_window_seconds) {
    if (!thermal_info.rapl_supported || !msr_is_supported()) return 0;
    
    /* Convert power limit to RAPL units */
    u32 power_limit_units = power_limit_watts * thermal_info.rapl.power_units;
    
    /* Convert time window to RAPL units */
    u32 time_window_units = time_window_seconds / thermal_info.rapl.time_units;
    
    /* Build power limit register value */
    u64 power_limit_reg = power_limit_units & 0x7FFF;
    power_limit_reg |= POWER_LIMIT_ENABLE;
    power_limit_reg |= ((u64)time_window_units & POWER_LIMIT_TIME_WINDOW_MASK) << POWER_LIMIT_TIME_WINDOW_SHIFT;
    
    msr_write(MSR_PKG_POWER_LIMIT, power_limit_reg);
    
    thermal_info.rapl.pkg_power_limit = power_limit_reg;
    thermal_info.rapl.pkg_power_limit_1 = power_limit_units;
    thermal_info.rapl.pkg_time_window_1 = time_window_seconds;
    thermal_info.rapl.pkg_power_limit_enabled = 1;
    
    return 1;
}

u64 thermal_mgmt_read_energy_consumption(u8 domain) {
    if (!thermal_info.rapl_supported || !msr_is_supported()) return 0;
    
    u64 energy_raw = 0;
    
    switch (domain) {
        case 0: /* Package */
            energy_raw = msr_read(MSR_PKG_ENERGY_STATUS);
            thermal_info.rapl.pkg_energy_consumed = energy_raw;
            break;
        case 1: /* DRAM */
            energy_raw = msr_read(MSR_DRAM_ENERGY_STATUS);
            thermal_info.rapl.dram_energy_consumed = energy_raw;
            break;
        case 2: /* PP0 (Cores) */
            energy_raw = msr_read(MSR_PP0_ENERGY_STATUS);
            thermal_info.rapl.pp0_energy_consumed = energy_raw;
            break;
        case 3: /* PP1 (Graphics) */
            energy_raw = msr_read(MSR_PP1_ENERGY_STATUS);
            thermal_info.rapl.pp1_energy_consumed = energy_raw;
            break;
        default:
            return 0;
    }
    
    /* Convert to microjoules */
    return energy_raw * thermal_info.rapl.energy_units;
}

void thermal_mgmt_handle_thermal_event(u8 event_type, u32 temperature) {
    thermal_event_t* event = &thermal_info.thermal_events[thermal_info.thermal_event_index];
    
    event->event_type = event_type;
    event->timestamp = timer_get_ticks();
    event->temperature = temperature;
    event->cpu_id = cpu_get_current_id();
    event->duration_ms = 0;  /* Will be updated when event ends */
    
    thermal_info.thermal_event_index = (thermal_info.thermal_event_index + 1) % 32;
    thermal_info.thermal_events_count++;
    thermal_info.last_thermal_event = event->timestamp;
    
    switch (event_type) {
        case THERMAL_EVENT_THRESHOLD:
            thermal_info.thermal_interrupts++;
            break;
        case THERMAL_EVENT_PROCHOT:
            thermal_info.throttle_count++;
            break;
        case THERMAL_EVENT_CRITICAL:
            thermal_info.critical_temp_reached = 1;
            break;
        case THERMAL_EVENT_POWER_LIMIT:
            thermal_info.power_limit_violations++;
            break;
    }
}

u8 thermal_mgmt_is_thermal_monitoring_supported(void) {
    return thermal_info.thermal_monitoring_supported;
}

u8 thermal_mgmt_is_rapl_supported(void) {
    return thermal_info.rapl_supported;
}

u32 thermal_mgmt_get_current_temperature(void) {
    return thermal_info.current_temperature;
}

u32 thermal_mgmt_get_max_temperature(void) {
    return thermal_info.max_temperature;
}

u32 thermal_mgmt_get_critical_temperature(void) {
    return thermal_info.critical_temperature;
}

u32 thermal_mgmt_get_thermal_target(void) {
    return thermal_info.thermal_target;
}

u8 thermal_mgmt_is_thermal_throttling_active(void) {
    return thermal_info.thermal_throttling_active;
}

u8 thermal_mgmt_is_prochot_active(void) {
    return thermal_info.prochot_active;
}

u8 thermal_mgmt_is_critical_temp_reached(void) {
    return thermal_info.critical_temp_reached;
}

u32 thermal_mgmt_get_thermal_events_count(void) {
    return thermal_info.thermal_events_count;
}

const thermal_event_t* thermal_mgmt_get_thermal_event(u32 index) {
    if (index >= 32) return 0;
    return &thermal_info.thermal_events[index];
}

const rapl_info_t* thermal_mgmt_get_rapl_info(void) {
    return &thermal_info.rapl;
}

u32 thermal_mgmt_get_throttle_count(void) {
    return thermal_info.throttle_count;
}

u64 thermal_mgmt_get_total_throttle_time(void) {
    return thermal_info.total_throttle_time;
}

u64 thermal_mgmt_get_last_thermal_event(void) {
    return thermal_info.last_thermal_event;
}

u32 thermal_mgmt_get_thermal_interrupts(void) {
    return thermal_info.thermal_interrupts;
}

u32 thermal_mgmt_get_power_limit_violations(void) {
    return thermal_info.power_limit_violations;
}

const char* thermal_mgmt_get_thermal_event_name(u8 event_type) {
    switch (event_type) {
        case THERMAL_EVENT_THRESHOLD: return "Threshold";
        case THERMAL_EVENT_PROCHOT: return "PROCHOT";
        case THERMAL_EVENT_CRITICAL: return "Critical";
        case THERMAL_EVENT_POWER_LIMIT: return "Power Limit";
        default: return "Unknown";
    }
}

void thermal_mgmt_clear_statistics(void) {
    thermal_info.thermal_events_count = 0;
    thermal_info.thermal_event_index = 0;
    thermal_info.throttle_count = 0;
    thermal_info.total_throttle_time = 0;
    thermal_info.thermal_interrupts = 0;
    thermal_info.power_limit_violations = 0;
    thermal_info.critical_temp_reached = 0;
}
