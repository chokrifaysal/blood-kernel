/*
 * memory_subsys.c – x86 memory subsystem control (DRAM/ECC/bandwidth monitoring)
 */

#include "kernel/types.h"

/* Intel Memory Controller MSRs */
#define MSR_PLATFORM_INFO		0xCE
#define MSR_TURBO_RATIO_LIMIT		0x1AD
#define MSR_ENERGY_PERF_BIAS		0x1B0

/* Intel RDT (Resource Director Technology) MSRs */
#define MSR_IA32_PQR_ASSOC		0xC8F
#define MSR_IA32_QM_EVTSEL		0xC8D
#define MSR_IA32_QM_CTR			0xC8E
#define MSR_IA32_L3_QOS_CFG		0xC81

/* Memory controller registers (PCIe config space) */
#define INTEL_IMC_DEV			0x3C00
#define INTEL_IMC_FUNC			0
#define AMD_UMC_DEV			0x1460

/* DRAM timing register offsets */
#define MC_TIMING_1			0x80
#define MC_TIMING_2			0x84
#define MC_TIMING_3			0x88
#define MC_CHANNEL_MAPPER		0x60
#define MC_ECC_INJECT			0xF0
#define MC_ECC_STATUS			0xF4

/* ECC status bits */
#define ECC_CORRECTABLE_ERROR		(1 << 0)
#define ECC_UNCORRECTABLE_ERROR		(1 << 1)
#define ECC_SCRUB_ENABLED		(1 << 8)
#define ECC_PATROL_ENABLED		(1 << 9)

/* MBM event encoding */
#define MBM_LOCAL_BW			0x02
#define MBM_TOTAL_BW			0x03

/* CMT event encoding */
#define CMT_LLC_OCCUPANCY		0x01

/* Prefetch control MSRs */
#define MSR_MISC_FEATURE_CONTROL	0x1A4
#define MSR_PREFETCH_CONTROL		0x1A4

/* Global state */
static memory_config_t mem_config;
static ecc_status_t ecc_status;
static memory_bandwidth_t bandwidth_stats[16];
static cache_monitoring_t cache_stats[16];
static u32 mem_capabilities = 0;
static u8 controller_detected = 0;
static u32 total_transactions = 0;
static u32 total_errors = 0;
static u64 init_timestamp = 0;

static inline void cpuid(u32 leaf, u32 subleaf, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx) {
	__asm__ volatile("cpuid"
		: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "a"(leaf), "c"(subleaf));
}

static inline u64 rdmsr(u32 msr) {
	u32 low, high;
	__asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
	return ((u64)high << 32) | low;
}

static inline void wrmsr(u32 msr, u64 value) {
	u32 low = value & 0xFFFFFFFF;
	u32 high = value >> 32;
	__asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline u32 pci_read32(u8 bus, u8 dev, u8 func, u8 offset) {
	u32 address = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | offset;
	__asm__ volatile("outl %0, $0xCF8" : : "a"(address));
	u32 result;
	__asm__ volatile("inl $0xCFC, %0" : "=a"(result));
	return result;
}

static inline void pci_write32(u8 bus, u8 dev, u8 func, u8 offset, u32 value) {
	u32 address = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | offset;
	__asm__ volatile("outl %0, $0xCF8" : : "a"(address));
	__asm__ volatile("outl %0, $0xCFC" : : "a"(value));
}

static inline u64 rdtsc(void) {
	u32 low, high;
	__asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
	return ((u64)high << 32) | low;
}

void memory_subsys_init(void) {
	u32 eax, ebx, ecx, edx;
	
	/* Clear all state */
	for (u32 i = 0; i < sizeof(mem_config); i++) {
		((u8*)&mem_config)[i] = 0;
	}
	for (u32 i = 0; i < sizeof(ecc_status); i++) {
		((u8*)&ecc_status)[i] = 0;
	}
	
	init_timestamp = rdtsc();
	
	/* Detect Intel RDT support (CPUID.7.0:EBX) */
	cpuid(7, 0, &eax, &ebx, &ecx, &edx);
	if (ebx & (1 << 12)) mem_capabilities |= MEMSYS_CAP_MBM;  /* MBM */
	if (ebx & (1 << 15)) mem_capabilities |= MEMSYS_CAP_CMT;  /* CMT */
	
	/* Detect prefetch control */
	cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (ecx & (1 << 18)) mem_capabilities |= MEMSYS_CAP_PREFETCH_CTRL;
	
	/* Detect memory controller */
	memory_subsys_detect_controller();
	
	/* Initialize ECC detection */
	u32 mc_status = pci_read32(0, 0, 0, MC_ECC_STATUS);
	if (mc_status & ECC_SCRUB_ENABLED) {
		mem_capabilities |= MEMSYS_CAP_ECC | MEMSYS_CAP_SCRUBBING;
	}
	
	/* Configure default settings */
	mem_config.channels = 2;
	mem_config.dimms_per_channel = 2;
	mem_config.ranks_per_dimm = 2;
	mem_config.channel_width = 64;
	mem_config.bus_width = 64;
	mem_config.capabilities = mem_capabilities;
}

u8 memory_subsys_is_supported(void) {
	return mem_capabilities != 0;
}

u32 memory_subsys_get_capabilities(void) {
	return mem_capabilities;
}

const memory_config_t* memory_subsys_get_config(void) {
	return &mem_config;
}

u8 memory_subsys_detect_controller(void) {
	u32 vendor_device;
	
	/* Check for Intel IMC */
	vendor_device = pci_read32(0, 0, 0, 0);
	if ((vendor_device & 0xFFFF) == 0x8086) {
		mem_config.controller_type = MEM_CTRL_INTEL_IMC;
		controller_detected = 1;
		
		/* Read memory configuration from IMC */
		u32 mc_channel = pci_read32(0, 0, 0, MC_CHANNEL_MAPPER);
		mem_config.channels = ((mc_channel >> 6) & 0x3) + 1;
		
		/* Detect DRAM type */
		u32 mc_timing = pci_read32(0, 0, 0, MC_TIMING_1);
		if (mc_timing & (1 << 31)) {
			mem_config.dram_type = DRAM_TYPE_DDR4;
		} else {
			mem_config.dram_type = DRAM_TYPE_DDR3;
		}
		
		return 1;
	}
	
	/* Check for AMD UMC */
	for (u8 node = 0; node < 8; node++) {
		vendor_device = pci_read32(0, 24 + node, 0, 0);
		if ((vendor_device & 0xFFFF) == 0x1022) {
			mem_config.controller_type = MEM_CTRL_AMD_UMC;
			controller_detected = 1;
			return 1;
		}
	}
	
	return 0;
}

u8 memory_subsys_get_controller_count(void) {
	if (mem_config.controller_type == MEM_CTRL_INTEL_IMC) {
		return 1; /* Single IMC per socket */
	} else if (mem_config.controller_type == MEM_CTRL_AMD_UMC) {
		return 2; /* Dual UMC per socket */
	}
	return 0;
}

const char* memory_subsys_get_controller_name(u8 controller) {
	switch (mem_config.controller_type) {
		case MEM_CTRL_INTEL_IMC: return "Intel IMC";
		case MEM_CTRL_AMD_UMC: return "AMD UMC";
		case MEM_CTRL_GENERIC: return "Generic";
		default: return "Unknown";
	}
}

u8 memory_subsys_detect_dram(void) {
	if (!controller_detected) return 0;
	
	/* Read DRAM timings from memory controller */
	u32 timing1 = pci_read32(0, 0, 0, MC_TIMING_1);
	u32 timing2 = pci_read32(0, 0, 0, MC_TIMING_2);
	u32 timing3 = pci_read32(0, 0, 0, MC_TIMING_3);
	
	/* Extract timing parameters */
	static dram_timing_t timings;
	timings.cl = (timing1 >> 0) & 0x1F;
	timings.trcd = (timing1 >> 8) & 0x1F;
	timings.trp = (timing1 >> 16) & 0x1F;
	timings.tras = (timing1 >> 24) & 0x3F;
	timings.trc = (timing2 >> 0) & 0x3F;
	timings.trfc = (timing2 >> 8) & 0x3FF;
	timings.twtr = (timing2 >> 20) & 0x1F;
	timings.trtp = (timing2 >> 28) & 0xF;
	timings.tfaw = (timing3 >> 0) & 0x3F;
	timings.tcwl = (timing3 >> 8) & 0x1F;
	
	/* Estimate frequency from timings */
	if (timings.cl <= 11 && timings.trcd <= 11) {
		mem_config.frequency = 1333; /* DDR3-1333 */
	} else if (timings.cl <= 15 && timings.trcd <= 15) {
		mem_config.frequency = 2400; /* DDR4-2400 */
	} else {
		mem_config.frequency = 3200; /* DDR4-3200 or higher */
	}
	
	return 1;
}

const dram_timing_t* memory_subsys_get_dram_timings(u8 channel) {
	if (channel >= mem_config.channels) return 0;
	
	static dram_timing_t timings;
	u32 timing1 = pci_read32(0, 0, 0, MC_TIMING_1 + (channel * 0x10));
	u32 timing2 = pci_read32(0, 0, 0, MC_TIMING_2 + (channel * 0x10));
	
	timings.cl = (timing1 >> 0) & 0x1F;
	timings.trcd = (timing1 >> 8) & 0x1F;
	timings.trp = (timing1 >> 16) & 0x1F;
	timings.tras = (timing1 >> 24) & 0x3F;
	timings.trc = (timing2 >> 0) & 0x3F;
	timings.trfc = (timing2 >> 8) & 0x3FF;
	
	return &timings;
}

u8 memory_subsys_set_dram_timings(u8 channel, const dram_timing_t* timings) {
	if (channel >= mem_config.channels || !timings) return 0;
	
	u32 timing1 = (timings->tras << 24) | (timings->trp << 16) | 
		      (timings->trcd << 8) | timings->cl;
	u32 timing2 = (timings->trfc << 8) | timings->trc;
	
	pci_write32(0, 0, 0, MC_TIMING_1 + (channel * 0x10), timing1);
	pci_write32(0, 0, 0, MC_TIMING_2 + (channel * 0x10), timing2);
	
	return 1;
}

u32 memory_subsys_get_dram_frequency(u8 channel) {
	return mem_config.frequency;
}

u8 memory_subsys_is_ecc_enabled(void) {
	u32 ecc_ctrl = pci_read32(0, 0, 0, MC_ECC_STATUS);
	return (ecc_ctrl & ECC_SCRUB_ENABLED) != 0;
}

u8 memory_subsys_enable_ecc(u8 enable) {
	u32 ecc_ctrl = pci_read32(0, 0, 0, MC_ECC_STATUS);
	
	if (enable) {
		ecc_ctrl |= ECC_SCRUB_ENABLED | ECC_PATROL_ENABLED;
		mem_capabilities |= MEMSYS_CAP_ECC | MEMSYS_CAP_SCRUBBING;
	} else {
		ecc_ctrl &= ~(ECC_SCRUB_ENABLED | ECC_PATROL_ENABLED);
	}
	
	pci_write32(0, 0, 0, MC_ECC_STATUS, ecc_ctrl);
	return 1;
}

const ecc_status_t* memory_subsys_get_ecc_status(void) {
	u32 status = pci_read32(0, 0, 0, MC_ECC_STATUS);
	
	if (status & ECC_CORRECTABLE_ERROR) {
		ecc_status.correctable_errors++;
		ecc_status.single_bit_errors++;
	}
	
	if (status & ECC_UNCORRECTABLE_ERROR) {
		ecc_status.uncorrectable_errors++;
		ecc_status.double_bit_errors++;
	}
	
	ecc_status.total_errors = ecc_status.correctable_errors + 
				  ecc_status.uncorrectable_errors;
	
	/* Read error address if available */
	if (status & (ECC_CORRECTABLE_ERROR | ECC_UNCORRECTABLE_ERROR)) {
		ecc_status.last_error_addr = pci_read32(0, 0, 0, MC_ECC_STATUS + 4);
		ecc_status.error_syndrome = (status >> 16) & 0xFF;
	}
	
	return &ecc_status;
}

u8 memory_subsys_clear_ecc_errors(void) {
	u32 status = pci_read32(0, 0, 0, MC_ECC_STATUS);
	status |= ECC_CORRECTABLE_ERROR | ECC_UNCORRECTABLE_ERROR; /* W1C bits */
	pci_write32(0, 0, 0, MC_ECC_STATUS, status);
	return 1;
}

u8 memory_subsys_is_mbm_supported(void) {
	return (mem_capabilities & MEMSYS_CAP_MBM) != 0;
}

u8 memory_subsys_start_mbm_monitoring(u32 rmid, u8 event) {
	if (!memory_subsys_is_mbm_supported() || rmid >= 16) return 0;
	
	/* Configure PQR_ASSOC with RMID */
	wrmsr(MSR_IA32_PQR_ASSOC, rmid);
	
	/* Select event in QM_EVTSEL */
	u32 evtsel = event;
	if (event == MBM_EVENT_LOCAL) evtsel = MBM_LOCAL_BW;
	if (event == MBM_EVENT_TOTAL) evtsel = MBM_TOTAL_BW;
	
	wrmsr(MSR_IA32_QM_EVTSEL, (u64)rmid << 32 | evtsel);
	
	return 1;
}

u8 memory_subsys_stop_mbm_monitoring(u32 rmid) {
	if (rmid >= 16) return 0;
	
	wrmsr(MSR_IA32_PQR_ASSOC, 0);
	return 1;
}

const memory_bandwidth_t* memory_subsys_get_bandwidth_stats(u32 rmid) {
	if (rmid >= 16) return 0;
	
	/* Read bandwidth counters */
	u64 local_bw = memory_subsys_read_mbm_counter(rmid, MBM_EVENT_LOCAL);
	u64 total_bw = memory_subsys_read_mbm_counter(rmid, MBM_EVENT_TOTAL);
	
	bandwidth_stats[rmid].read_bandwidth = local_bw * 64; /* Convert to bytes */
	bandwidth_stats[rmid].total_bandwidth = total_bw * 64;
	bandwidth_stats[rmid].write_bandwidth = bandwidth_stats[rmid].total_bandwidth - 
						 bandwidth_stats[rmid].read_bandwidth;
	
	/* Calculate utilization (simplified) */
	u64 max_bw = mem_config.frequency * mem_config.channels * 8; /* MB/s */
	if (max_bw > 0) {
		bandwidth_stats[rmid].utilization_percent = 
			(u32)((bandwidth_stats[rmid].total_bandwidth * 100) / max_bw);
	}
	
	return &bandwidth_stats[rmid];
}

u64 memory_subsys_read_mbm_counter(u32 rmid, u8 event) {
	if (!memory_subsys_is_mbm_supported() || rmid >= 16) return 0;
	
	u32 evtsel = event;
	if (event == MBM_EVENT_LOCAL) evtsel = MBM_LOCAL_BW;
	if (event == MBM_EVENT_TOTAL) evtsel = MBM_TOTAL_BW;
	
	wrmsr(MSR_IA32_QM_EVTSEL, (u64)rmid << 32 | evtsel);
	
	return rdmsr(MSR_IA32_QM_CTR);
}

u8 memory_subsys_is_cmt_supported(void) {
	return (mem_capabilities & MEMSYS_CAP_CMT) != 0;
}

u8 memory_subsys_start_cmt_monitoring(u32 rmid) {
	if (!memory_subsys_is_cmt_supported() || rmid >= 16) return 0;
	
	wrmsr(MSR_IA32_PQR_ASSOC, rmid);
	wrmsr(MSR_IA32_QM_EVTSEL, (u64)rmid << 32 | CMT_LLC_OCCUPANCY);
	
	return 1;
}

const cache_monitoring_t* memory_subsys_get_cache_stats(u32 rmid) {
	if (rmid >= 16) return 0;
	
	cache_stats[rmid].llc_occupancy = memory_subsys_read_llc_occupancy(rmid);
	
	/* Estimate other metrics based on occupancy changes */
	static u32 prev_occupancy[16] = {0};
	if (cache_stats[rmid].llc_occupancy < prev_occupancy[rmid]) {
		cache_stats[rmid].evictions++;
	}
	prev_occupancy[rmid] = cache_stats[rmid].llc_occupancy;
	
	cache_stats[rmid].total_accesses++;
	
	return &cache_stats[rmid];
}

u32 memory_subsys_read_llc_occupancy(u32 rmid) {
	if (!memory_subsys_is_cmt_supported() || rmid >= 16) return 0;
	
	wrmsr(MSR_IA32_QM_EVTSEL, (u64)rmid << 32 | CMT_LLC_OCCUPANCY);
	u64 occupancy = rdmsr(MSR_IA32_QM_CTR);
	
	return (u32)(occupancy * 1024); /* Convert to bytes */
}

u8 memory_subsys_is_prefetch_supported(void) {
	return (mem_capabilities & MEMSYS_CAP_PREFETCH_CTRL) != 0;
}

u8 memory_subsys_enable_hw_prefetch(u8 enable) {
	if (!memory_subsys_is_prefetch_supported()) return 0;
	
	u64 misc_features = rdmsr(MSR_MISC_FEATURE_CONTROL);
	
	if (enable) {
		misc_features &= ~(1ULL << 0); /* Enable L2 HW prefetch */
		misc_features &= ~(1ULL << 1); /* Enable L2 adjacent line prefetch */
	} else {
		misc_features |= (1ULL << 0);  /* Disable L2 HW prefetch */
		misc_features |= (1ULL << 1);  /* Disable L2 adjacent line prefetch */
	}
	
	wrmsr(MSR_MISC_FEATURE_CONTROL, misc_features);
	return 1;
}

u8 memory_subsys_enable_adjacent_line_prefetch(u8 enable) {
	if (!memory_subsys_is_prefetch_supported()) return 0;
	
	u64 misc_features = rdmsr(MSR_MISC_FEATURE_CONTROL);
	
	if (enable) {
		misc_features &= ~(1ULL << 1);
	} else {
		misc_features |= (1ULL << 1);
	}
	
	wrmsr(MSR_MISC_FEATURE_CONTROL, misc_features);
	return 1;
}

u8 memory_subsys_get_dram_temperature(u8 channel) {
	/* Read thermal sensor if available */
	u32 thermal_status = pci_read32(0, 0, 0, 0x1A4);
	return (thermal_status >> (channel * 8)) & 0xFF;
}

u8 memory_subsys_is_thermal_throttling_active(void) {
	u32 thermal_status = pci_read32(0, 0, 0, 0x1A8);
	return (thermal_status & (1 << 0)) != 0;
}

void memory_subsys_clear_stats(void) {
	total_transactions = 0;
	total_errors = 0;
	
	for (u8 i = 0; i < 16; i++) {
		for (u32 j = 0; j < sizeof(bandwidth_stats[i]); j++) {
			((u8*)&bandwidth_stats[i])[j] = 0;
		}
		for (u32 j = 0; j < sizeof(cache_stats[i]); j++) {
			((u8*)&cache_stats[i])[j] = 0;
		}
	}
}

u32 memory_subsys_get_total_transactions(void) {
	return total_transactions;
}

u32 memory_subsys_get_error_count(void) {
	return ecc_status.total_errors;
}

u64 memory_subsys_get_uptime_seconds(void) {
	u64 current_tsc = rdtsc();
	u64 tsc_freq = 3000000000ULL; /* Assume 3GHz TSC */
	return (current_tsc - init_timestamp) / tsc_freq;
}

const char* memory_subsys_get_dram_type_name(u8 type) {
	switch (type) {
		case DRAM_TYPE_DDR3: return "DDR3";
		case DRAM_TYPE_DDR4: return "DDR4";
		case DRAM_TYPE_DDR5: return "DDR5";
		case DRAM_TYPE_LPDDR4: return "LPDDR4";
		case DRAM_TYPE_LPDDR5: return "LPDDR5";
		default: return "Unknown";
	}
}

const char* memory_subsys_get_ecc_type_name(u8 type) {
	switch (type) {
		case ECC_TYPE_NONE: return "None";
		case ECC_TYPE_SECDED: return "SECDED";
		case ECC_TYPE_CHIPKILL: return "Chipkill";
		case ECC_TYPE_ADDDC: return "ADDDC";
		default: return "Unknown";
	}
}

const char* memory_subsys_get_capability_name(u32 cap_bit) {
	switch (cap_bit) {
		case 0: return "ECC";
		case 1: return "Scrubbing";
		case 2: return "Patrol";
		case 3: return "Demand";
		case 4: return "MBM";
		case 5: return "CMT";
		case 6: return "Prefetch Control";
		case 7: return "Thermal Throttle";
		default: return "Unknown";
	}
}

u8 memory_subsys_self_test(void) {
	if (!memory_subsys_is_supported()) return 0;
	
	/* Test MBM if supported */
	if (memory_subsys_is_mbm_supported()) {
		if (!memory_subsys_start_mbm_monitoring(0, MBM_EVENT_TOTAL)) return 0;
		u64 counter = memory_subsys_read_mbm_counter(0, MBM_EVENT_TOTAL);
		memory_subsys_stop_mbm_monitoring(0);
		
		if (counter == 0) return 0; /* No activity detected */
	}
	
	/* Test CMT if supported */
	if (memory_subsys_is_cmt_supported()) {
		if (!memory_subsys_start_cmt_monitoring(0)) return 0;
		u32 occupancy = memory_subsys_read_llc_occupancy(0);
		memory_subsys_stop_cmt_monitoring(0);
		
		if (occupancy == 0) return 0; /* No LLC usage */
	}
	
	/* Test prefetch control if supported */
	if (memory_subsys_is_prefetch_supported()) {
		if (!memory_subsys_enable_hw_prefetch(0)) return 0;
		if (!memory_subsys_enable_hw_prefetch(1)) return 0;
	}
	
	return 1;
}

void memory_subsys_print_config(void) {
	/* This would integrate with kernel's print system */
	/* For now, just update transaction counter to indicate activity */
	total_transactions++;
}