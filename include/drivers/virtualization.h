/*
 * virtualization.h – x86 CPU virtualization extensions (VMX/SVM)
 */

#ifndef VIRTUALIZATION_H
#define VIRTUALIZATION_H

#include "kernel/types.h"

/* VMX capabilities */
#define VMX_CAP_EPT                     (1 << 0)
#define VMX_CAP_VPID                    (1 << 1)
#define VMX_CAP_UNRESTRICTED_GUEST      (1 << 2)
#define VMX_CAP_VMFUNC                  (1 << 3)

/* SVM capabilities */
#define SVM_CAP_NPT                     (1 << 0)
#define SVM_CAP_LBR_VIRT                (1 << 1)
#define SVM_CAP_SVM_LOCK                (1 << 2)
#define SVM_CAP_NRIP_SAVE               (1 << 3)

/* Core functions */
void virtualization_init(void);

/* Support detection */
u8 virtualization_is_supported(void);
u8 virtualization_is_vmx_supported(void);
u8 virtualization_is_svm_supported(void);

/* VMX operations */
u8 virtualization_enable_vmx(void);
u8 virtualization_is_vmx_enabled(void);
u8 virtualization_allocate_vmcs(void);
u8 virtualization_vmlaunch(void);

/* SVM operations */
u8 virtualization_enable_svm(void);
u8 virtualization_is_svm_enabled(void);
u8 virtualization_allocate_vmcb(void);
u8 virtualization_vmrun(void);

/* VM exit handling */
void virtualization_handle_vmexit(u32 exit_reason);

/* Capability information */
u64 virtualization_get_vmx_basic(void);
u32 virtualization_get_vmx_capabilities(void);
u32 virtualization_get_svm_capabilities(void);
u32 virtualization_get_vmcs_revision_id(void);
u32 virtualization_get_vmcs_size(void);

/* Feature checks */
u8 virtualization_has_ept(void);
u8 virtualization_has_vpid(void);
u8 virtualization_has_unrestricted_guest(void);
u8 virtualization_has_npt(void);
u8 virtualization_is_feature_control_locked(void);

/* Statistics */
u32 virtualization_get_vm_entries(void);
u32 virtualization_get_vm_exits(void);
u64 virtualization_get_total_vm_time(void);
u64 virtualization_get_last_vm_entry_time(void);
u64 virtualization_get_last_vm_exit_time(void);
const u32* virtualization_get_vm_exit_reasons(void);

/* Cleanup */
void virtualization_cleanup(void);
void virtualization_clear_statistics(void);

#endif
