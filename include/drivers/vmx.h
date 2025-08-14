/*
 * vmx.h – x86 Intel VT-x virtualization support
 */

#ifndef VMX_H
#define VMX_H

#include "kernel/types.h"

/* Core functions */
void vmx_init(void);
void vmx_disable(void);

/* Status functions */
u8 vmx_is_supported(void);
u8 vmx_is_enabled(void);
u8 vmx_is_ept_supported(void);
u8 vmx_is_vpid_supported(void);
u8 vmx_is_unrestricted_guest_supported(void);

/* Capability information */
u32 vmx_get_vmcs_revision_id(void);
u32 vmx_get_vmcs_size(void);
u64 vmx_get_pin_based_controls(void);
u64 vmx_get_proc_based_controls(void);
u64 vmx_get_exit_controls(void);
u64 vmx_get_entry_controls(void);

/* VMCS operations */
u8 vmx_vmclear(void* vmcs);
u8 vmx_vmptrld(void* vmcs);
u8 vmx_vmread(u32 field, u64* value);
u8 vmx_vmwrite(u32 field, u64 value);

#endif
