/*
 * memory_subsys.h – x86 memory subsystem control (DRAM/ECC/bandwidth monitoring)
 */

#ifndef MEMORY_SUBSYS_H
#define MEMORY_SUBSYS_H

#include "kernel/types.h"

/* Memory controller types */
#define MEM_CTRL_INTEL_IMC		0
#define MEM_CTRL_AMD_UMC		1
#define MEM_CTRL_GENERIC		2

/* DRAM types */
#define DRAM_TYPE_DDR3			0
#define DRAM_TYPE_DDR4			1
#define DRAM_TYPE_DDR5			2
#define DRAM_TYPE_LPDDR4		3
#define DRAM_TYPE_LPDDR5		4

/* ECC types */
#define ECC_TYPE_NONE			0
#define ECC_TYPE_SECDED			1
#define ECC_TYPE_CHIPKILL		2
#define ECC_TYPE_ADDDC			3

/* Memory bandwidth monitoring events */
#define MBM_EVENT_LOCAL			0
#define MBM_EVENT_TOTAL			1
#define MBM_EVENT_READS			2
#define MBM_EVENT_WRITES		3

/* Cache monitoring technology events */
#define CMT_EVENT_LLC_OCCUPANCY		0
#define CMT_EVENT_MISS_RATE		1

/* Memory subsystem capabilities */
#define MEMSYS_CAP_ECC			(1 << 0)
#define MEMSYS_CAP_SCRUBBING		(1 << 1)
#define MEMSYS_CAP_PATROL		(1 << 2)
#define MEMSYS_CAP_DEMAND		(1 << 3)
#define MEMSYS_CAP_MBM			(1 << 4)
#define MEMSYS_CAP_CMT			(1 << 5)
#define MEMSYS_CAP_PREFETCH_CTRL	(1 << 6)
#define MEMSYS_CAP_THERMAL_THROTTLE	(1 << 7)

typedef struct {
	u8 controller_type;
	u8 channels;
	u8 dimms_per_channel;
	u8 ranks_per_dimm;
	u32 channel_width;
	u32 bus_width;
	u64 total_capacity;
	u32 frequency;
	u8 dram_type;
	u8 ecc_type;
	u32 capabilities;
} memory_config_t;

typedef struct {
	u16 cl;		/* CAS Latency */
	u16 trcd;	/* RAS to CAS Delay */
	u16 trp;	/* Row Precharge */
	u16 tras;	/* Active to Precharge */
	u16 trc;	/* Row Cycle Time */
	u16 trfc;	/* Refresh Cycle Time */
	u16 twtr;	/* Write to Read */
	u16 trtp;	/* Read to Precharge */
	u16 tfaw;	/* Four Bank Activate Window */
	u16 tcwl;	/* CAS Write Latency */
	u16 tcke;	/* Clock Enable */
	u16 txp;	/* Exit Power Down */
} dram_timing_t;

typedef struct {
	u32 single_bit_errors;
	u32 double_bit_errors;
	u32 correctable_errors;
	u32 uncorrectable_errors;
	u64 total_errors;
	u32 scrub_rate;
	u32 patrol_rate;
	u64 last_error_addr;
	u32 error_syndrome;
	u8 channel_errors[8];
	u8 dimm_errors[32];
} ecc_status_t;

typedef struct {
	u64 read_bandwidth;
	u64 write_bandwidth;
	u64 total_bandwidth;
	u32 utilization_percent;
	u64 transactions;
	u32 avg_latency_ns;
	u32 queue_depth;
	u32 page_hits;
	u32 page_misses;
	u32 bank_conflicts;
	u64 refresh_cycles;
} memory_bandwidth_t;

typedef struct {
	u32 llc_occupancy;
	u32 llc_misses;
	u32 llc_hits;
	u32 miss_rate_percent;
	u64 total_accesses;
	u32 evictions;
} cache_monitoring_t;

/* Core functions */
void memory_subsys_init(void);

/* Detection and capability */
u8 memory_subsys_is_supported(void);
u32 memory_subsys_get_capabilities(void);
const memory_config_t* memory_subsys_get_config(void);

/* Memory controller access */
u8 memory_subsys_detect_controller(void);
u8 memory_subsys_get_controller_count(void);
const char* memory_subsys_get_controller_name(u8 controller);

/* DRAM configuration */
u8 memory_subsys_detect_dram(void);
const dram_timing_t* memory_subsys_get_dram_timings(u8 channel);
u8 memory_subsys_set_dram_timings(u8 channel, const dram_timing_t* timings);
u32 memory_subsys_get_dram_frequency(u8 channel);
u8 memory_subsys_set_dram_frequency(u8 channel, u32 frequency);

/* ECC control */
u8 memory_subsys_is_ecc_enabled(void);
u8 memory_subsys_enable_ecc(u8 enable);
const ecc_status_t* memory_subsys_get_ecc_status(void);
u8 memory_subsys_clear_ecc_errors(void);
u8 memory_subsys_set_scrub_rate(u32 rate);
u8 memory_subsys_force_scrub(u64 start_addr, u64 size);

/* Memory bandwidth monitoring (MBM) */
u8 memory_subsys_is_mbm_supported(void);
u8 memory_subsys_start_mbm_monitoring(u32 rmid, u8 event);
u8 memory_subsys_stop_mbm_monitoring(u32 rmid);
const memory_bandwidth_t* memory_subsys_get_bandwidth_stats(u32 rmid);
u64 memory_subsys_read_mbm_counter(u32 rmid, u8 event);

/* Cache monitoring technology (CMT) */
u8 memory_subsys_is_cmt_supported(void);
u8 memory_subsys_start_cmt_monitoring(u32 rmid);
u8 memory_subsys_stop_cmt_monitoring(u32 rmid);
const cache_monitoring_t* memory_subsys_get_cache_stats(u32 rmid);
u32 memory_subsys_read_llc_occupancy(u32 rmid);

/* Memory prefetching control */
u8 memory_subsys_is_prefetch_supported(void);
u8 memory_subsys_enable_hw_prefetch(u8 enable);
u8 memory_subsys_enable_adjacent_line_prefetch(u8 enable);
u8 memory_subsys_enable_stream_prefetch(u8 enable);
u8 memory_subsys_set_prefetch_distance(u8 distance);

/* Memory thermal management */
u8 memory_subsys_get_dram_temperature(u8 channel);
u8 memory_subsys_is_thermal_throttling_active(void);
u8 memory_subsys_set_thermal_limits(u8 channel, u8 warning_temp, u8 critical_temp);

/* Memory training and calibration */
u8 memory_subsys_run_memory_training(u8 channel);
u8 memory_subsys_calibrate_write_leveling(u8 channel);
u8 memory_subsys_calibrate_read_training(u8 channel);
u8 memory_subsys_save_training_data(u8 channel);
u8 memory_subsys_restore_training_data(u8 channel);

/* Performance optimization */
u8 memory_subsys_set_page_policy(u8 policy);
u8 memory_subsys_set_refresh_mode(u8 mode);
u8 memory_subsys_enable_rank_interleaving(u8 enable);
u8 memory_subsys_enable_channel_interleaving(u8 enable);
u8 memory_subsys_set_command_rate(u8 channel, u8 rate);

/* Statistics and monitoring */
void memory_subsys_clear_stats(void);
u32 memory_subsys_get_total_transactions(void);
u32 memory_subsys_get_error_count(void);
u64 memory_subsys_get_uptime_seconds(void);

/* Utilities */
const char* memory_subsys_get_dram_type_name(u8 type);
const char* memory_subsys_get_ecc_type_name(u8 type);
const char* memory_subsys_get_capability_name(u32 cap_bit);
u8 memory_subsys_self_test(void);
void memory_subsys_print_config(void);

#endif