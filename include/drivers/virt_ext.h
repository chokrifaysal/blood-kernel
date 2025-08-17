/*
 * virt_ext.h – x86 CPU virtualization extensions (VT-x enhancements)
 */

#ifndef VIRT_EXT_H
#define VIRT_EXT_H

#include "kernel/types.h"

/* VMX operation states */
#define VMX_STATE_OFF               0
#define VMX_STATE_ROOT              1
#define VMX_STATE_NON_ROOT          2

typedef struct {
    u64 basic;
    u32 pinbased_ctls_default0;
    u32 pinbased_ctls_default1;
    u32 procbased_ctls_default0;
    u32 procbased_ctls_default1;
    u32 exit_ctls_default0;
    u32 exit_ctls_default1;
    u32 entry_ctls_default0;
    u32 entry_ctls_default1;
    u64 misc;
    u64 cr0_fixed0;
    u64 cr0_fixed1;
    u64 cr4_fixed0;
    u64 cr4_fixed1;
    u64 vmcs_enum;
    u32 procbased_ctls2_default0;
    u32 procbased_ctls2_default1;
    u64 ept_vpid_cap;
    u64 vmfunc_cap;
} vmx_capabilities_t;

/* Core functions */
void virt_ext_init(void);

/* Support detection */
u8 virt_ext_is_supported(void);
u8 virt_ext_is_vmx_supported(void);
u8 virt_ext_is_svm_supported(void);
u8 virt_ext_is_ept_supported(void);
u8 virt_ext_is_vpid_supported(void);
u8 virt_ext_is_unrestricted_guest_supported(void);
u8 virt_ext_is_vmfunc_supported(void);
u8 virt_ext_is_nested_virtualization_supported(void);

/* VMX control */
u8 virt_ext_enable_vmx(void);
u8 virt_ext_is_vmx_enabled(void);
u8 virt_ext_vmxon(u64 vmxon_region_pa);
u8 virt_ext_vmxoff(void);

/* VMCS operations */
u8 virt_ext_vmptrld(u64 vmcs_pa);
u8 virt_ext_vmclear(u64 vmcs_pa);
u8 virt_ext_vmread(u64 field, u64* value);
u8 virt_ext_vmwrite(u64 field, u64 value);

/* VM execution */
u8 virt_ext_vmlaunch(void);
u8 virt_ext_vmresume(void);
void virt_ext_handle_vmexit(void);

/* Statistics */
u32 virt_ext_get_vm_exit_count(void);
u32 virt_ext_get_vm_entry_count(void);
u8 virt_ext_get_vmx_operation_state(void);

/* Capabilities */
const vmx_capabilities_t* virt_ext_get_vmx_capabilities(void);

/* VPID management */
u16 virt_ext_allocate_vpid(void);

/* EPT/VPID invalidation */
u8 virt_ext_invept(u32 type, u64 eptp);
u8 virt_ext_invvpid(u32 type, u16 vpid, u64 linear_address);

#endif
