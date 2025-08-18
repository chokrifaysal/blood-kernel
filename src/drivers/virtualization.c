/*
 * virtualization.c – x86 CPU virtualization extensions (VMX/SVM)
 */

#include "kernel/types.h"

/* VMX MSRs */
#define MSR_IA32_FEATURE_CONTROL        0x3A
#define MSR_IA32_VMX_BASIC              0x480
#define MSR_IA32_VMX_PINBASED_CTLS      0x481
#define MSR_IA32_VMX_PROCBASED_CTLS     0x482
#define MSR_IA32_VMX_EXIT_CTLS          0x483
#define MSR_IA32_VMX_ENTRY_CTLS         0x484
#define MSR_IA32_VMX_MISC               0x485
#define MSR_IA32_VMX_CR0_FIXED0         0x486
#define MSR_IA32_VMX_CR0_FIXED1         0x487
#define MSR_IA32_VMX_CR4_FIXED0         0x488
#define MSR_IA32_VMX_CR4_FIXED1         0x489
#define MSR_IA32_VMX_VMCS_ENUM          0x48A
#define MSR_IA32_VMX_PROCBASED_CTLS2    0x48B
#define MSR_IA32_VMX_EPT_VPID_CAP       0x48C
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS 0x48D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x48E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS     0x48F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS    0x490
#define MSR_IA32_VMX_VMFUNC             0x491

/* SVM MSRs */
#define MSR_EFER                        0xC0000080
#define MSR_VM_CR                       0xC0010114
#define MSR_VM_HSAVE_PA                 0xC0010117

/* Feature Control bits */
#define FEATURE_CONTROL_LOCKED          (1 << 0)
#define FEATURE_CONTROL_VMX_INSIDE_SMX  (1 << 1)
#define FEATURE_CONTROL_VMX_OUTSIDE_SMX (1 << 2)

/* EFER bits */
#define EFER_SVME                       (1 << 12)

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

typedef struct {
    u32 revision_id;
    u32 vmx_abort_indicator;
    u8 data[4088];
} vmcs_t;

typedef struct {
    u64 intercept_cr_read;
    u64 intercept_cr_write;
    u64 intercept_dr_read;
    u64 intercept_dr_write;
    u32 intercept_exceptions;
    u64 intercept_instr1;
    u64 intercept_instr2;
    u8 reserved1[40];
    u16 pause_filter_threshold;
    u16 pause_filter_count;
    u64 iopm_base_pa;
    u64 msrpm_base_pa;
    u64 tsc_offset;
    u32 guest_asid;
    u8 tlb_control;
    u8 reserved2[3];
    u32 v_intr_control;
    u64 v_intr_vector;
    u64 interrupt_shadow;
    u64 exitcode;
    u64 exitinfo1;
    u64 exitinfo2;
    u64 exitintinfo;
    u64 np_enable;
    u8 reserved3[16];
    u64 avic_apic_bar;
    u8 reserved4[8];
    u64 ghcb_pa;
    u64 eventinj;
    u64 n_cr3;
    u64 lbr_virtualization_enable;
    u32 vmcb_clean_bits;
    u32 reserved5;
    u64 nrip;
    u8 num_of_bytes_fetched;
    u8 guest_instruction_bytes[15];
    u64 avic_apic_backing_page_ptr;
    u8 reserved6[8];
    u64 avic_logical_table_ptr;
    u64 avic_physical_table_ptr;
    u8 reserved7[8];
    u64 vmsa_ptr;
} vmcb_control_area_t;

typedef struct {
    u8 virtualization_supported;
    u8 vmx_supported;
    u8 svm_supported;
    u8 vmx_enabled;
    u8 svm_enabled;
    u8 feature_control_locked;
    u64 vmx_basic;
    u64 vmx_pinbased_ctls;
    u64 vmx_procbased_ctls;
    u64 vmx_exit_ctls;
    u64 vmx_entry_ctls;
    u64 vmx_misc;
    u64 vmx_ept_vpid_cap;
    u32 vmx_capabilities;
    u32 svm_capabilities;
    u32 vmcs_revision_id;
    u32 vmcs_size;
    u64 vmcs_physical_address;
    vmcs_t* vmcs_virtual_address;
    u64 vmcb_physical_address;
    vmcb_control_area_t* vmcb_virtual_address;
    u32 vm_entries;
    u32 vm_exits;
    u32 vm_exit_reasons[32];
    u64 last_vm_entry_time;
    u64 last_vm_exit_time;
    u64 total_vm_time;
} virtualization_info_t;

static virtualization_info_t virtualization_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);
extern void* kmalloc(u32 size);
extern void kfree(void* ptr);
extern u64 virt_to_phys(void* virt);

static void virtualization_detect_vmx(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for VMX support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    virtualization_info.vmx_supported = (ecx & (1 << 5)) != 0;
    
    if (!virtualization_info.vmx_supported || !msr_is_supported()) return;
    
    /* Read VMX capabilities */
    virtualization_info.vmx_basic = msr_read(MSR_IA32_VMX_BASIC);
    virtualization_info.vmx_pinbased_ctls = msr_read(MSR_IA32_VMX_PINBASED_CTLS);
    virtualization_info.vmx_procbased_ctls = msr_read(MSR_IA32_VMX_PROCBASED_CTLS);
    virtualization_info.vmx_exit_ctls = msr_read(MSR_IA32_VMX_EXIT_CTLS);
    virtualization_info.vmx_entry_ctls = msr_read(MSR_IA32_VMX_ENTRY_CTLS);
    virtualization_info.vmx_misc = msr_read(MSR_IA32_VMX_MISC);
    
    /* Extract VMCS information */
    virtualization_info.vmcs_revision_id = virtualization_info.vmx_basic & 0x7FFFFFFF;
    virtualization_info.vmcs_size = (virtualization_info.vmx_basic >> 32) & 0x1FFF;
    
    /* Check for advanced features */
    if (msr_read(MSR_IA32_VMX_PROCBASED_CTLS) & (1ULL << 63)) {
        virtualization_info.vmx_ept_vpid_cap = msr_read(MSR_IA32_VMX_EPT_VPID_CAP);
        
        if (virtualization_info.vmx_ept_vpid_cap & (1 << 0)) {
            virtualization_info.vmx_capabilities |= VMX_CAP_EPT;
        }
        if (virtualization_info.vmx_ept_vpid_cap & (1 << 32)) {
            virtualization_info.vmx_capabilities |= VMX_CAP_VPID;
        }
        if (virtualization_info.vmx_ept_vpid_cap & (1 << 5)) {
            virtualization_info.vmx_capabilities |= VMX_CAP_UNRESTRICTED_GUEST;
        }
    }
    
    /* Check feature control */
    u64 feature_control = msr_read(MSR_IA32_FEATURE_CONTROL);
    virtualization_info.feature_control_locked = (feature_control & FEATURE_CONTROL_LOCKED) != 0;
}

static void virtualization_detect_svm(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for SVM support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    virtualization_info.svm_supported = (ecx & (1 << 2)) != 0;
    
    if (!virtualization_info.svm_supported) return;
    
    /* Check SVM features */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x8000000A));
    
    if (edx & (1 << 0)) virtualization_info.svm_capabilities |= SVM_CAP_NPT;
    if (edx & (1 << 1)) virtualization_info.svm_capabilities |= SVM_CAP_LBR_VIRT;
    if (edx & (1 << 2)) virtualization_info.svm_capabilities |= SVM_CAP_SVM_LOCK;
    if (edx & (1 << 3)) virtualization_info.svm_capabilities |= SVM_CAP_NRIP_SAVE;
}

void virtualization_init(void) {
    virtualization_info.virtualization_supported = 0;
    virtualization_info.vmx_supported = 0;
    virtualization_info.svm_supported = 0;
    virtualization_info.vmx_enabled = 0;
    virtualization_info.svm_enabled = 0;
    virtualization_info.feature_control_locked = 0;
    virtualization_info.vmx_basic = 0;
    virtualization_info.vmx_pinbased_ctls = 0;
    virtualization_info.vmx_procbased_ctls = 0;
    virtualization_info.vmx_exit_ctls = 0;
    virtualization_info.vmx_entry_ctls = 0;
    virtualization_info.vmx_misc = 0;
    virtualization_info.vmx_ept_vpid_cap = 0;
    virtualization_info.vmx_capabilities = 0;
    virtualization_info.svm_capabilities = 0;
    virtualization_info.vmcs_revision_id = 0;
    virtualization_info.vmcs_size = 0;
    virtualization_info.vmcs_physical_address = 0;
    virtualization_info.vmcs_virtual_address = 0;
    virtualization_info.vmcb_physical_address = 0;
    virtualization_info.vmcb_virtual_address = 0;
    virtualization_info.vm_entries = 0;
    virtualization_info.vm_exits = 0;
    virtualization_info.last_vm_entry_time = 0;
    virtualization_info.last_vm_exit_time = 0;
    virtualization_info.total_vm_time = 0;
    
    for (u32 i = 0; i < 32; i++) {
        virtualization_info.vm_exit_reasons[i] = 0;
    }
    
    virtualization_detect_vmx();
    virtualization_detect_svm();
    
    if (virtualization_info.vmx_supported || virtualization_info.svm_supported) {
        virtualization_info.virtualization_supported = 1;
    }
}

u8 virtualization_is_supported(void) {
    return virtualization_info.virtualization_supported;
}

u8 virtualization_is_vmx_supported(void) {
    return virtualization_info.vmx_supported;
}

u8 virtualization_is_svm_supported(void) {
    return virtualization_info.svm_supported;
}

u8 virtualization_enable_vmx(void) {
    if (!virtualization_info.vmx_supported || !msr_is_supported()) return 0;
    
    /* Check if VMX is already enabled */
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    if (cr4 & (1 << 13)) {
        virtualization_info.vmx_enabled = 1;
        return 1;
    }
    
    /* Enable VMX in feature control if not locked */
    u64 feature_control = msr_read(MSR_IA32_FEATURE_CONTROL);
    if (!(feature_control & FEATURE_CONTROL_LOCKED)) {
        feature_control |= FEATURE_CONTROL_VMX_OUTSIDE_SMX | FEATURE_CONTROL_LOCKED;
        msr_write(MSR_IA32_FEATURE_CONTROL, feature_control);
    }
    
    /* Enable VMX in CR4 */
    cr4 |= (1 << 13);
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    virtualization_info.vmx_enabled = 1;
    return 1;
}

u8 virtualization_enable_svm(void) {
    if (!virtualization_info.svm_supported || !msr_is_supported()) return 0;
    
    /* Enable SVM in EFER */
    u64 efer = msr_read(MSR_EFER);
    efer |= EFER_SVME;
    msr_write(MSR_EFER, efer);
    
    virtualization_info.svm_enabled = 1;
    return 1;
}

u8 virtualization_allocate_vmcs(void) {
    if (!virtualization_info.vmx_enabled) return 0;
    
    /* Allocate VMCS region */
    virtualization_info.vmcs_virtual_address = (vmcs_t*)kmalloc(4096);
    if (!virtualization_info.vmcs_virtual_address) return 0;
    
    virtualization_info.vmcs_physical_address = virt_to_phys(virtualization_info.vmcs_virtual_address);
    
    /* Initialize VMCS */
    virtualization_info.vmcs_virtual_address->revision_id = virtualization_info.vmcs_revision_id;
    virtualization_info.vmcs_virtual_address->vmx_abort_indicator = 0;
    
    return 1;
}

u8 virtualization_allocate_vmcb(void) {
    if (!virtualization_info.svm_enabled) return 0;
    
    /* Allocate VMCB region */
    virtualization_info.vmcb_virtual_address = (vmcb_control_area_t*)kmalloc(4096);
    if (!virtualization_info.vmcb_virtual_address) return 0;
    
    virtualization_info.vmcb_physical_address = virt_to_phys(virtualization_info.vmcb_virtual_address);
    
    return 1;
}

u8 virtualization_vmlaunch(void) {
    if (!virtualization_info.vmx_enabled || !virtualization_info.vmcs_virtual_address) return 0;
    
    u64 entry_time = timer_get_ticks();
    
    /* Execute VMLAUNCH */
    u8 result;
    __asm__ volatile(
        "vmlaunch\n\t"
        "setna %0"
        : "=q"(result)
        :
        : "memory"
    );
    
    if (result == 0) {
        virtualization_info.vm_entries++;
        virtualization_info.last_vm_entry_time = entry_time;
    }
    
    return result == 0;
}

u8 virtualization_vmrun(void) {
    if (!virtualization_info.svm_enabled || !virtualization_info.vmcb_virtual_address) return 0;
    
    u64 entry_time = timer_get_ticks();
    
    /* Execute VMRUN */
    __asm__ volatile(
        "vmrun"
        :
        : "a"(virtualization_info.vmcb_physical_address)
        : "memory"
    );
    
    virtualization_info.vm_entries++;
    virtualization_info.last_vm_entry_time = entry_time;
    
    return 1;
}

void virtualization_handle_vmexit(u32 exit_reason) {
    u64 exit_time = timer_get_ticks();
    
    virtualization_info.vm_exits++;
    virtualization_info.last_vm_exit_time = exit_time;
    
    if (virtualization_info.last_vm_entry_time > 0) {
        virtualization_info.total_vm_time += (exit_time - virtualization_info.last_vm_entry_time);
    }
    
    /* Record exit reason */
    if (exit_reason < 32) {
        virtualization_info.vm_exit_reasons[exit_reason]++;
    }
}

u8 virtualization_is_vmx_enabled(void) {
    return virtualization_info.vmx_enabled;
}

u8 virtualization_is_svm_enabled(void) {
    return virtualization_info.svm_enabled;
}

u64 virtualization_get_vmx_basic(void) {
    return virtualization_info.vmx_basic;
}

u32 virtualization_get_vmx_capabilities(void) {
    return virtualization_info.vmx_capabilities;
}

u32 virtualization_get_svm_capabilities(void) {
    return virtualization_info.svm_capabilities;
}

u32 virtualization_get_vmcs_revision_id(void) {
    return virtualization_info.vmcs_revision_id;
}

u32 virtualization_get_vmcs_size(void) {
    return virtualization_info.vmcs_size;
}

u32 virtualization_get_vm_entries(void) {
    return virtualization_info.vm_entries;
}

u32 virtualization_get_vm_exits(void) {
    return virtualization_info.vm_exits;
}

u64 virtualization_get_total_vm_time(void) {
    return virtualization_info.total_vm_time;
}

u64 virtualization_get_last_vm_entry_time(void) {
    return virtualization_info.last_vm_entry_time;
}

u64 virtualization_get_last_vm_exit_time(void) {
    return virtualization_info.last_vm_exit_time;
}

const u32* virtualization_get_vm_exit_reasons(void) {
    return virtualization_info.vm_exit_reasons;
}

u8 virtualization_has_ept(void) {
    return (virtualization_info.vmx_capabilities & VMX_CAP_EPT) != 0;
}

u8 virtualization_has_vpid(void) {
    return (virtualization_info.vmx_capabilities & VMX_CAP_VPID) != 0;
}

u8 virtualization_has_unrestricted_guest(void) {
    return (virtualization_info.vmx_capabilities & VMX_CAP_UNRESTRICTED_GUEST) != 0;
}

u8 virtualization_has_npt(void) {
    return (virtualization_info.svm_capabilities & SVM_CAP_NPT) != 0;
}

u8 virtualization_is_feature_control_locked(void) {
    return virtualization_info.feature_control_locked;
}

void virtualization_cleanup(void) {
    if (virtualization_info.vmcs_virtual_address) {
        kfree(virtualization_info.vmcs_virtual_address);
        virtualization_info.vmcs_virtual_address = 0;
        virtualization_info.vmcs_physical_address = 0;
    }
    
    if (virtualization_info.vmcb_virtual_address) {
        kfree(virtualization_info.vmcb_virtual_address);
        virtualization_info.vmcb_virtual_address = 0;
        virtualization_info.vmcb_physical_address = 0;
    }
}

void virtualization_clear_statistics(void) {
    virtualization_info.vm_entries = 0;
    virtualization_info.vm_exits = 0;
    virtualization_info.total_vm_time = 0;
    
    for (u32 i = 0; i < 32; i++) {
        virtualization_info.vm_exit_reasons[i] = 0;
    }
}
