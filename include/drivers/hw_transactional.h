/*
 * hw_transactional.h – x86 hardware transactional memory (TSX RTM/HLE)
 */

#ifndef HW_TRANSACTIONAL_H
#define HW_TRANSACTIONAL_H

#include "kernel/types.h"

/* Transaction status codes */
#define TX_STATUS_SUCCESS		0
#define TX_STATUS_ABORT			1
#define TX_STATUS_RETRY			2
#define TX_STATUS_CONFLICT		3
#define TX_STATUS_CAPACITY		4
#define TX_STATUS_DEBUG			5
#define TX_STATUS_NESTED		6
#define TX_STATUS_UNSUPPORTED		7

/* RTM abort codes (EAX register after xabort) */
#define RTM_ABORT_EXPLICIT		(1 << 0)
#define RTM_ABORT_RETRY			(1 << 1)
#define RTM_ABORT_CONFLICT		(1 << 2)
#define RTM_ABORT_CAPACITY		(1 << 3)
#define RTM_ABORT_DEBUG			(1 << 4)
#define RTM_ABORT_NESTED		(1 << 5)

/* HLE prefixes */
#define HLE_ACQUIRE			0xF2
#define HLE_RELEASE			0xF3

/* TSX capabilities */
#define TSX_CAP_RTM			(1 << 0)
#define TSX_CAP_HLE			(1 << 1)
#define TSX_CAP_FORCE_ABORT		(1 << 2)
#define TSX_CAP_DEBUG_INTERFACE		(1 << 3)

/* Transaction retry policies */
#define TX_RETRY_NEVER			0
#define TX_RETRY_ONCE			1
#define TX_RETRY_ADAPTIVE		2
#define TX_RETRY_AGGRESSIVE		3

/* Transaction nesting limits */
#define TSX_MAX_NEST_DEPTH		7

typedef struct {
	u32 total_attempts;
	u32 successful_commits;
	u32 explicit_aborts;
	u32 retry_aborts;
	u32 conflict_aborts;
	u32 capacity_aborts;
	u32 debug_aborts;
	u32 nested_aborts;
	u32 fallback_executions;
	u32 hle_attempts;
	u32 hle_successful;
	u64 total_tx_cycles;
	u64 total_fallback_cycles;
	u32 avg_tx_cycles;
	u32 avg_fallback_cycles;
	u32 max_nest_depth_reached;
} tsx_stats_t;

typedef struct {
	u8 nest_level;
	u32 start_cycles;
	u32 abort_code;
	u8 retry_count;
	u8 retry_policy;
	u8 force_fallback;
} tsx_context_t;

typedef struct {
	volatile u32 lock;
	u32 hle_acquisitions;
	u32 hle_fallbacks;
	u32 contention_cycles;
} hle_spinlock_t;

/* Core functions */
void hw_transactional_init(void);

/* Capability detection */
u8 hw_transactional_is_supported(void);
u8 hw_transactional_has_rtm(void);
u8 hw_transactional_has_hle(void);
u32 hw_transactional_get_capabilities(void);

/* RTM (Restricted Transactional Memory) */
u32 hw_transactional_begin(void);
void hw_transactional_end(void);
void hw_transactional_abort(u8 code);
u8 hw_transactional_test(void);

/* High-level RTM interface */
u8 hw_transactional_execute(void (*transaction_func)(void*), void* data,
			    u8 retry_policy, u32 max_retries);
u8 hw_transactional_execute_with_fallback(void (*transaction_func)(void*), 
					  void (*fallback_func)(void*), void* data,
					  u8 retry_policy, u32 max_retries);

/* RTM context management */
void hw_transactional_context_init(tsx_context_t* ctx, u8 retry_policy);
u8 hw_transactional_context_begin(tsx_context_t* ctx);
u8 hw_transactional_context_end(tsx_context_t* ctx);
u8 hw_transactional_context_abort(tsx_context_t* ctx, u8 code);

/* HLE (Hardware Lock Elision) */
void hw_transactional_hle_spinlock_init(hle_spinlock_t* lock);
void hw_transactional_hle_acquire(hle_spinlock_t* lock);
void hw_transactional_hle_release(hle_spinlock_t* lock);
u8 hw_transactional_hle_try_acquire(hle_spinlock_t* lock);
u8 hw_transactional_hle_is_locked(hle_spinlock_t* lock);

/* HLE with standard operations */
void hw_transactional_hle_xchg_acquire(volatile u32* lock);
void hw_transactional_hle_xchg_release(volatile u32* lock);
void hw_transactional_hle_cmpxchg_acquire(volatile u32* lock, u32 expected, u32 new_val);
void hw_transactional_hle_cmpxchg_release(volatile u32* lock, u32 expected, u32 new_val);

/* Transaction analysis */
u8 hw_transactional_decode_abort_code(u32 abort_code);
const char* hw_transactional_get_abort_reason(u32 abort_code);
u8 hw_transactional_should_retry(u32 abort_code, u32 retry_count);

/* Performance monitoring */
const tsx_stats_t* hw_transactional_get_stats(void);
void hw_transactional_clear_stats(void);
u32 hw_transactional_get_success_rate(void);
u32 hw_transactional_get_abort_rate_by_type(u32 abort_type);

/* Benchmarking */
u32 hw_transactional_benchmark_simple_tx(u32 iterations);
u32 hw_transactional_benchmark_contended_tx(u32 iterations, u32 threads);
u32 hw_transactional_benchmark_hle_lock(u32 iterations);

/* Advanced features */
u8 hw_transactional_force_fallback_mode(u8 enable);
u8 hw_transactional_set_debug_mode(u8 enable);
u32 hw_transactional_get_max_nest_depth(void);

/* Memory operations inside transactions */
u8 hw_transactional_memcpy(void* dest, const void* src, u32 size);
u8 hw_transactional_memset(void* ptr, u8 value, u32 size);
u8 hw_transactional_atomic_inc(volatile u32* ptr);
u8 hw_transactional_atomic_dec(volatile u32* ptr);
u8 hw_transactional_atomic_add(volatile u32* ptr, u32 value);

/* Transaction conflict detection */
void hw_transactional_add_conflict_address(u64 address);
void hw_transactional_remove_conflict_address(u64 address);
u8 hw_transactional_is_conflict_address(u64 address);
void hw_transactional_clear_conflict_addresses(void);

/* Utilities */
const char* hw_transactional_get_capability_name(u32 cap_bit);
const char* hw_transactional_get_retry_policy_name(u8 policy);
u8 hw_transactional_self_test(void);
void hw_transactional_print_stats(void);

/* Macros for convenient RTM usage */
#define TSX_BEGIN() \
    do { \
        u32 __tx_status = hw_transactional_begin(); \
        if (__tx_status == ~0U) {

#define TSX_END() \
            hw_transactional_end(); \
        } else { \
            /* Transaction aborted, handle in fallback */ \
        } \
    } while(0)

#define TSX_ABORT(code) hw_transactional_abort(code)

#define TSX_WITH_FALLBACK(tx_code, fallback_code) \
    do { \
        u32 __tx_status = hw_transactional_begin(); \
        if (__tx_status == ~0U) { \
            tx_code; \
            hw_transactional_end(); \
        } else { \
            fallback_code; \
        } \
    } while(0)

#define HLE_SPINLOCK_ACQUIRE(lock) hw_transactional_hle_acquire(lock)
#define HLE_SPINLOCK_RELEASE(lock) hw_transactional_hle_release(lock)

#endif