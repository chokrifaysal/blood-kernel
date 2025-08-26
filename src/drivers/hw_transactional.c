/*
 * hw_transactional.c – x86 hardware transactional memory (TSX RTM/HLE)
 */

#include "kernel/types.h"

/* CPUID feature bits */
#define CPUID_EBX_RTM			(1 << 11)
#define CPUID_EBX_HLE			(1 << 4)
#define CPUID_EBX_TSX_FORCE_ABORT	(1 << 13)

/* MSR definitions */
#define MSR_IA32_TSX_CTRL		0x122
#define MSR_IA32_TSX_FORCE_ABORT	0x10F

/* TSX Control bits */
#define TSX_CTRL_RTM_DISABLE		(1ULL << 0)
#define TSX_CTRL_CPUID_CLEAR		(1ULL << 1)

/* RTM status values */
#define RTM_STARTED			(~0U)

/* Global state */
static tsx_stats_t tsx_stats;
static u32 tsx_capabilities = 0;
static u8 tsx_debug_mode = 0;
static u8 tsx_force_fallback = 0;
static tsx_context_t global_ctx;
static u64 conflict_addresses[16];
static u8 conflict_count = 0;

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

static inline u64 rdtsc(void) {
	u32 low, high;
	__asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
	return ((u64)high << 32) | low;
}

void hw_transactional_init(void) {
	u32 eax, ebx, ecx, edx;
	
	/* Clear all state */
	for (u32 i = 0; i < sizeof(tsx_stats); i++) {
		((u8*)&tsx_stats)[i] = 0;
	}
	
	/* Detect TSX support (CPUID.7.0:EBX) */
	cpuid(7, 0, &eax, &ebx, &ecx, &edx);
	if (ebx & CPUID_EBX_RTM) tsx_capabilities |= TSX_CAP_RTM;
	if (ebx & CPUID_EBX_HLE) tsx_capabilities |= TSX_CAP_HLE;
	if (ebx & CPUID_EBX_TSX_FORCE_ABORT) tsx_capabilities |= TSX_CAP_FORCE_ABORT;
	
	/* Check if TSX is disabled by microcode or BIOS */
	if (tsx_capabilities & (TSX_CAP_RTM | TSX_CAP_HLE)) {
		u64 tsx_ctrl = rdmsr(MSR_IA32_TSX_CTRL);
		if (tsx_ctrl & TSX_CTRL_RTM_DISABLE) {
			tsx_capabilities &= ~TSX_CAP_RTM;
		}
	}
	
	/* Initialize global context */
	hw_transactional_context_init(&global_ctx, TX_RETRY_ADAPTIVE);
}

u8 hw_transactional_is_supported(void) {
	return tsx_capabilities != 0;
}

u8 hw_transactional_has_rtm(void) {
	return (tsx_capabilities & TSX_CAP_RTM) != 0;
}

u8 hw_transactional_has_hle(void) {
	return (tsx_capabilities & TSX_CAP_HLE) != 0;
}

u32 hw_transactional_get_capabilities(void) {
	return tsx_capabilities;
}

u32 hw_transactional_begin(void) {
	if (!hw_transactional_has_rtm() || tsx_force_fallback) {
		return TX_STATUS_UNSUPPORTED;
	}
	
	u32 status;
	tsx_stats.total_attempts++;
	
	__asm__ volatile(
		".byte 0xC7, 0xF8\n\t"		/* xbegin */
		".long 0\n\t"			/* fallback offset */
		: "=a"(status)
		:
		: "memory"
	);
	
	if (status == RTM_STARTED) {
		global_ctx.nest_level++;
		global_ctx.start_cycles = (u32)rdtsc();
		return RTM_STARTED;
	} else {
		/* Transaction aborted */
		tsx_stats.total_tx_cycles += (u32)rdtsc() - global_ctx.start_cycles;
		
		if (status & RTM_ABORT_EXPLICIT) tsx_stats.explicit_aborts++;
		if (status & RTM_ABORT_RETRY) tsx_stats.retry_aborts++;
		if (status & RTM_ABORT_CONFLICT) tsx_stats.conflict_aborts++;
		if (status & RTM_ABORT_CAPACITY) tsx_stats.capacity_aborts++;
		if (status & RTM_ABORT_DEBUG) tsx_stats.debug_aborts++;
		if (status & RTM_ABORT_NESTED) tsx_stats.nested_aborts++;
		
		global_ctx.abort_code = status;
		return status;
	}
}

void hw_transactional_end(void) {
	if (!hw_transactional_has_rtm() || global_ctx.nest_level == 0) {
		return;
	}
	
	__asm__ volatile(".byte 0x0F, 0x01, 0xD5" ::: "memory"); /* xend */
	
	global_ctx.nest_level--;
	if (global_ctx.nest_level == 0) {
		tsx_stats.successful_commits++;
		tsx_stats.total_tx_cycles += (u32)rdtsc() - global_ctx.start_cycles;
		tsx_stats.avg_tx_cycles = tsx_stats.total_tx_cycles / tsx_stats.successful_commits;
	}
}

void hw_transactional_abort(u8 code) {
	if (!hw_transactional_has_rtm()) return;
	
	__asm__ volatile(
		".byte 0xC6, 0xF8, %0"
		:
		: "i"(code)
		: "memory"
	); /* xabort */
}

u8 hw_transactional_test(void) {
	if (!hw_transactional_has_rtm()) return 0;
	
	u8 result;
	__asm__ volatile(
		".byte 0x0F, 0x01, 0xD6\n\t"	/* xtest */
		"setnz %0"
		: "=r"(result)
		:
		: "cc"
	);
	
	return result;
}

u8 hw_transactional_execute(void (*transaction_func)(void*), void* data,
			    u8 retry_policy, u32 max_retries) {
	if (!transaction_func) return 0;
	
	u32 retry_count = 0;
	
	while (retry_count <= max_retries) {
		u32 status = hw_transactional_begin();
		
		if (status == RTM_STARTED) {
			transaction_func(data);
			hw_transactional_end();
			return 1; /* Success */
		}
		
		/* Transaction aborted */
		if (!hw_transactional_should_retry(status, retry_count)) {
			break;
		}
		
		retry_count++;
		
		/* Adaptive backoff */
		if (retry_policy == TX_RETRY_ADAPTIVE) {
			for (u32 i = 0; i < (1 << retry_count); i++) {
				__asm__ volatile("pause");
			}
		}
	}
	
	tsx_stats.fallback_executions++;
	return 0; /* Failed, fallback needed */
}

u8 hw_transactional_execute_with_fallback(void (*transaction_func)(void*), 
					  void (*fallback_func)(void*), void* data,
					  u8 retry_policy, u32 max_retries) {
	u64 start_cycles = rdtsc();
	
	if (hw_transactional_execute(transaction_func, data, retry_policy, max_retries)) {
		return 1;
	}
	
	/* Execute fallback */
	if (fallback_func) {
		fallback_func(data);
		tsx_stats.total_fallback_cycles += (u32)(rdtsc() - start_cycles);
		tsx_stats.avg_fallback_cycles = tsx_stats.total_fallback_cycles / tsx_stats.fallback_executions;
		return 1;
	}
	
	return 0;
}

void hw_transactional_context_init(tsx_context_t* ctx, u8 retry_policy) {
	if (!ctx) return;
	
	ctx->nest_level = 0;
	ctx->start_cycles = 0;
	ctx->abort_code = 0;
	ctx->retry_count = 0;
	ctx->retry_policy = retry_policy;
	ctx->force_fallback = 0;
}

u8 hw_transactional_context_begin(tsx_context_t* ctx) {
	if (!ctx) return 0;
	
	u32 status = hw_transactional_begin();
	
	if (status == RTM_STARTED) {
		return 1;
	}
	
	ctx->abort_code = status;
	ctx->retry_count++;
	
	return 0;
}

u8 hw_transactional_context_end(tsx_context_t* ctx) {
	if (!ctx || ctx->nest_level == 0) return 0;
	
	hw_transactional_end();
	ctx->retry_count = 0;
	return 1;
}

u8 hw_transactional_context_abort(tsx_context_t* ctx, u8 code) {
	if (!ctx) return 0;
	
	hw_transactional_abort(code);
	return 1;
}

void hw_transactional_hle_spinlock_init(hle_spinlock_t* lock) {
	if (!lock) return;
	
	lock->lock = 0;
	lock->hle_acquisitions = 0;
	lock->hle_fallbacks = 0;
	lock->contention_cycles = 0;
}

void hw_transactional_hle_acquire(hle_spinlock_t* lock) {
	if (!lock || !hw_transactional_has_hle()) return;
	
	u64 start_cycles = rdtsc();
	
	/* HLE acquire with XACQUIRE prefix */
	while (1) {
		u32 expected = 0;
		u8 success;
		
		__asm__ volatile(
			".byte 0xF2\n\t"		/* XACQUIRE prefix */
			"lock cmpxchgl %2, %1\n\t"
			"sete %0"
			: "=r"(success), "+m"(lock->lock)
			: "r"(1), "a"(expected)
			: "memory", "cc"
		);
		
		if (success) {
			lock->hle_acquisitions++;
			tsx_stats.hle_attempts++;
			tsx_stats.hle_successful++;
			break;
		}
		
		/* Contention detected */
		while (lock->lock != 0) {
			__asm__ volatile("pause");
		}
		
		lock->contention_cycles += (u32)(rdtsc() - start_cycles);
	}
}

void hw_transactional_hle_release(hle_spinlock_t* lock) {
	if (!lock || !hw_transactional_has_hle()) return;
	
	/* HLE release with XRELEASE prefix */
	__asm__ volatile(
		".byte 0xF3\n\t"		/* XRELEASE prefix */
		"movl $0, %0"
		: "=m"(lock->lock)
		:
		: "memory"
	);
}

u8 hw_transactional_hle_try_acquire(hle_spinlock_t* lock) {
	if (!lock || !hw_transactional_has_hle()) return 0;
	
	u32 expected = 0;
	u8 success;
	
	tsx_stats.hle_attempts++;
	
	__asm__ volatile(
		".byte 0xF2\n\t"		/* XACQUIRE prefix */
		"lock cmpxchgl %2, %1\n\t"
		"sete %0"
		: "=r"(success), "+m"(lock->lock)
		: "r"(1), "a"(expected)
		: "memory", "cc"
	);
	
	if (success) {
		lock->hle_acquisitions++;
		tsx_stats.hle_successful++;
	} else {
		lock->hle_fallbacks++;
	}
	
	return success;
}

u8 hw_transactional_hle_is_locked(hle_spinlock_t* lock) {
	return lock ? (lock->lock != 0) : 0;
}

void hw_transactional_hle_xchg_acquire(volatile u32* lock) {
	if (!lock || !hw_transactional_has_hle()) return;
	
	while (1) {
		u32 result;
		
		__asm__ volatile(
			".byte 0xF2\n\t"		/* XACQUIRE prefix */
			"xchgl %0, %1"
			: "=r"(result), "+m"(*lock)
			: "0"(1)
			: "memory"
		);
		
		if (result == 0) break;
		
		while (*lock != 0) {
			__asm__ volatile("pause");
		}
	}
}

void hw_transactional_hle_xchg_release(volatile u32* lock) {
	if (!lock || !hw_transactional_has_hle()) return;
	
	__asm__ volatile(
		".byte 0xF3\n\t"		/* XRELEASE prefix */
		"movl $0, %0"
		: "=m"(*lock)
		:
		: "memory"
	);
}

u8 hw_transactional_decode_abort_code(u32 abort_code) {
	if (abort_code & RTM_ABORT_EXPLICIT) return TX_STATUS_ABORT;
	if (abort_code & RTM_ABORT_CONFLICT) return TX_STATUS_CONFLICT;
	if (abort_code & RTM_ABORT_CAPACITY) return TX_STATUS_CAPACITY;
	if (abort_code & RTM_ABORT_DEBUG) return TX_STATUS_DEBUG;
	if (abort_code & RTM_ABORT_NESTED) return TX_STATUS_NESTED;
	return TX_STATUS_ABORT;
}

const char* hw_transactional_get_abort_reason(u32 abort_code) {
	if (abort_code & RTM_ABORT_EXPLICIT) return "Explicit abort";
	if (abort_code & RTM_ABORT_CONFLICT) return "Memory conflict";
	if (abort_code & RTM_ABORT_CAPACITY) return "Cache capacity exceeded";
	if (abort_code & RTM_ABORT_DEBUG) return "Debug breakpoint";
	if (abort_code & RTM_ABORT_NESTED) return "Nested transaction limit";
	return "Unknown abort reason";
}

u8 hw_transactional_should_retry(u32 abort_code, u32 retry_count) {
	/* Don't retry explicit aborts */
	if (abort_code & RTM_ABORT_EXPLICIT) return 0;
	
	/* Always retry if retry bit is set */
	if (abort_code & RTM_ABORT_RETRY) return 1;
	
	/* Retry conflicts with exponential backoff */
	if (abort_code & RTM_ABORT_CONFLICT) {
		return retry_count < 8;
	}
	
	/* Don't retry capacity or debug aborts */
	if (abort_code & (RTM_ABORT_CAPACITY | RTM_ABORT_DEBUG)) return 0;
	
	/* Retry others with limit */
	return retry_count < 3;
}

const tsx_stats_t* hw_transactional_get_stats(void) {
	return &tsx_stats;
}

void hw_transactional_clear_stats(void) {
	for (u32 i = 0; i < sizeof(tsx_stats); i++) {
		((u8*)&tsx_stats)[i] = 0;
	}
}

u32 hw_transactional_get_success_rate(void) {
	if (tsx_stats.total_attempts == 0) return 0;
	return (tsx_stats.successful_commits * 100) / tsx_stats.total_attempts;
}

u32 hw_transactional_get_abort_rate_by_type(u32 abort_type) {
	u32 total_aborts = tsx_stats.total_attempts - tsx_stats.successful_commits;
	if (total_aborts == 0) return 0;
	
	u32 type_aborts = 0;
	switch (abort_type) {
		case RTM_ABORT_EXPLICIT: type_aborts = tsx_stats.explicit_aborts; break;
		case RTM_ABORT_RETRY: type_aborts = tsx_stats.retry_aborts; break;
		case RTM_ABORT_CONFLICT: type_aborts = tsx_stats.conflict_aborts; break;
		case RTM_ABORT_CAPACITY: type_aborts = tsx_stats.capacity_aborts; break;
		case RTM_ABORT_DEBUG: type_aborts = tsx_stats.debug_aborts; break;
		case RTM_ABORT_NESTED: type_aborts = tsx_stats.nested_aborts; break;
	}
	
	return (type_aborts * 100) / total_aborts;
}

u32 hw_transactional_benchmark_simple_tx(u32 iterations) {
	if (!hw_transactional_has_rtm()) return 0;
	
	volatile u32 shared_counter = 0;
	u64 start_cycles = rdtsc();
	
	for (u32 i = 0; i < iterations; i++) {
		u32 status = hw_transactional_begin();
		if (status == RTM_STARTED) {
			shared_counter++;
			hw_transactional_end();
		} else {
			shared_counter++; /* Fallback */
		}
	}
	
	u64 total_cycles = rdtsc() - start_cycles;
	return (u32)(total_cycles / iterations);
}

u32 hw_transactional_benchmark_hle_lock(u32 iterations) {
	if (!hw_transactional_has_hle()) return 0;
	
	hle_spinlock_t lock;
	hw_transactional_hle_spinlock_init(&lock);
	
	u64 start_cycles = rdtsc();
	
	for (u32 i = 0; i < iterations; i++) {
		hw_transactional_hle_acquire(&lock);
		/* Critical section */
		hw_transactional_hle_release(&lock);
	}
	
	u64 total_cycles = rdtsc() - start_cycles;
	return (u32)(total_cycles / iterations);
}

u8 hw_transactional_force_fallback_mode(u8 enable) {
	tsx_force_fallback = enable;
	return 1;
}

u8 hw_transactional_set_debug_mode(u8 enable) {
	tsx_debug_mode = enable;
	if (enable && (tsx_capabilities & TSX_CAP_FORCE_ABORT)) {
		wrmsr(MSR_IA32_TSX_FORCE_ABORT, 1);
	}
	return 1;
}

u32 hw_transactional_get_max_nest_depth(void) {
	return TSX_MAX_NEST_DEPTH;
}

u8 hw_transactional_memcpy(void* dest, const void* src, u32 size) {
	if (!dest || !src || size == 0) return 0;
	if (!hw_transactional_test()) return 0; /* Not in transaction */
	
	u8* d = (u8*)dest;
	const u8* s = (const u8*)src;
	
	for (u32 i = 0; i < size; i++) {
		d[i] = s[i];
	}
	
	return 1;
}

u8 hw_transactional_atomic_inc(volatile u32* ptr) {
	if (!ptr || !hw_transactional_test()) return 0;
	
	(*ptr)++;
	return 1;
}

u8 hw_transactional_atomic_dec(volatile u32* ptr) {
	if (!ptr || !hw_transactional_test()) return 0;
	
	(*ptr)--;
	return 1;
}

u8 hw_transactional_atomic_add(volatile u32* ptr, u32 value) {
	if (!ptr || !hw_transactional_test()) return 0;
	
	*ptr += value;
	return 1;
}

void hw_transactional_add_conflict_address(u64 address) {
	if (conflict_count < 16) {
		conflict_addresses[conflict_count++] = address;
	}
}

u8 hw_transactional_is_conflict_address(u64 address) {
	for (u8 i = 0; i < conflict_count; i++) {
		if (conflict_addresses[i] == address) return 1;
	}
	return 0;
}

void hw_transactional_clear_conflict_addresses(void) {
	conflict_count = 0;
}

const char* hw_transactional_get_capability_name(u32 cap_bit) {
	switch (cap_bit) {
		case 0: return "RTM";
		case 1: return "HLE";
		case 2: return "Force Abort";
		case 3: return "Debug Interface";
		default: return "Unknown";
	}
}

const char* hw_transactional_get_retry_policy_name(u8 policy) {
	switch (policy) {
		case TX_RETRY_NEVER: return "Never";
		case TX_RETRY_ONCE: return "Once";
		case TX_RETRY_ADAPTIVE: return "Adaptive";
		case TX_RETRY_AGGRESSIVE: return "Aggressive";
		default: return "Unknown";
	}
}

u8 hw_transactional_self_test(void) {
	if (!hw_transactional_is_supported()) return 0;
	
	/* Test RTM */
	if (hw_transactional_has_rtm()) {
		volatile u32 test_var = 0;
		
		u32 status = hw_transactional_begin();
		if (status == RTM_STARTED) {
			test_var = 42;
			hw_transactional_end();
			
			if (test_var != 42) return 0;
		}
	}
	
	/* Test HLE */
	if (hw_transactional_has_hle()) {
		hle_spinlock_t test_lock;
		hw_transactional_hle_spinlock_init(&test_lock);
		
		if (!hw_transactional_hle_try_acquire(&test_lock)) return 0;
		hw_transactional_hle_release(&test_lock);
		
		if (hw_transactional_hle_is_locked(&test_lock)) return 0;
	}
	
	return 1;
}

void hw_transactional_print_stats(void) {
	/* This would integrate with kernel's print system */
	/* For now, just update a counter to indicate activity */
	tsx_stats.total_attempts++;
}