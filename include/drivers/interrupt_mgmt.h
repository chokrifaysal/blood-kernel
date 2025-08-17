/*
 * interrupt_mgmt.h – x86 advanced interrupt handling (NMI/SMI/MCE)
 */

#ifndef INTERRUPT_MGMT_H
#define INTERRUPT_MGMT_H

#include "kernel/types.h"

/* Interrupt types */
#define INT_TYPE_EXTERNAL       0
#define INT_TYPE_NMI            1
#define INT_TYPE_SMI            2
#define INT_TYPE_MCE            3
#define INT_TYPE_THERMAL        4

typedef struct {
    u64 status;
    u64 addr;
    u64 misc;
    u32 bank;
    u8 valid;
} mce_record_t;

/* Core functions */
void interrupt_mgmt_init(void);

/* Support detection */
u8 interrupt_mgmt_is_nmi_supported(void);
u8 interrupt_mgmt_is_smi_supported(void);
u8 interrupt_mgmt_is_mce_supported(void);
u8 interrupt_mgmt_is_thermal_interrupt_supported(void);

/* NMI control */
void interrupt_mgmt_enable_nmi(void);
void interrupt_mgmt_disable_nmi(void);
u8 interrupt_mgmt_is_nmi_enabled(void);
void interrupt_mgmt_trigger_nmi(void);

/* Interrupt statistics */
u32 interrupt_mgmt_get_nmi_count(void);
u32 interrupt_mgmt_get_smi_count(void);
u32 interrupt_mgmt_get_mce_count(void);

/* MCE management */
u32 interrupt_mgmt_get_mce_bank_count(void);
const mce_record_t* interrupt_mgmt_get_mce_record(u32 index);
void interrupt_mgmt_clear_mce_records(void);
u8 interrupt_mgmt_is_mce_bank_valid(u32 bank);
u64 interrupt_mgmt_read_mce_bank_status(u32 bank);
void interrupt_mgmt_clear_mce_bank_status(u32 bank);
void interrupt_mgmt_enable_mce_bank(u32 bank);
void interrupt_mgmt_disable_mce_bank(u32 bank);

/* Interrupt handlers */
void interrupt_mgmt_handle_nmi(void);
void interrupt_mgmt_handle_mce(void);

/* Interrupt nesting */
void interrupt_mgmt_enter_interrupt(u32 vector);
void interrupt_mgmt_exit_interrupt(void);
u8 interrupt_mgmt_get_nesting_level(void);
u32 interrupt_mgmt_get_last_vector(void);
u8 interrupt_mgmt_in_interrupt(void);

/* Interrupt control */
void interrupt_mgmt_mask_all_interrupts(void);
void interrupt_mgmt_unmask_all_interrupts(void);
u8 interrupt_mgmt_are_interrupts_enabled(void);
void interrupt_mgmt_save_and_disable_interrupts(u32* flags);
void interrupt_mgmt_restore_interrupts(u32 flags);

/* Thermal interrupt support */
void interrupt_mgmt_setup_thermal_interrupt(void);
void interrupt_mgmt_handle_thermal_interrupt(void);

#endif
