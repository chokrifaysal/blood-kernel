/*
 * vmx.c – x86 Intel VT-x virtualization support
 */

#include "kernel/types.h"

/* VMX MSRs */
#define MSR_IA32_FEATURE_CONTROL    0x3A
#define MSR_IA32_VMX_BASIC          0x480
#define MSR_IA32_VMX_PINBASED_CTLS  0x481
#define MSR_IA32_VMX_PROCBASED_CTLS 0x482
#define MSR_IA32_VMX_EXIT_CTLS      0x483
#define MSR_IA32_VMX_ENTRY_CTLS     0x484
#define MSR_IA32_VMX_MISC           0x485
#define MSR_IA32_VMX_CR0_FIXED0     0x486
#define MSR_IA32_VMX_CR0_FIXED1     0x487
#define MSR_IA32_VMX_CR4_FIXED0     0x488
#define MSR_IA32_VMX_CR4_FIXED1     0x489
#define MSR_IA32_VMX_VMCS_ENUM      0x48A
#define MSR_IA32_VMX_PROCBASED_CTLS2 0x48B
#define MSR_IA32_VMX_EPT_VPID_CAP   0x48C
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS 0x48D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x48E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS 0x48F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS 0x490

/* Feature Control MSR bits */
#define FEATURE_CONTROL_LOCKED      0x01
#define FEATURE_CONTROL_VMX_INSIDE_SMX 0x02
#define FEATURE_CONTROL_VMX_OUTSIDE_SMX 0x04

/* VMCS fields */
#define VMCS_GUEST_ES_SELECTOR      0x0800
#define VMCS_GUEST_CS_SELECTOR      0x0802
#define VMCS_GUEST_SS_SELECTOR      0x0804
#define VMCS_GUEST_DS_SELECTOR      0x0806
#define VMCS_GUEST_FS_SELECTOR      0x0808
#define VMCS_GUEST_GS_SELECTOR      0x080A
#define VMCS_GUEST_LDTR_SELECTOR    0x080C
#define VMCS_GUEST_TR_SELECTOR      0x080E
#define VMCS_HOST_ES_SELECTOR       0x0C00
#define VMCS_HOST_CS_SELECTOR       0x0C02
#define VMCS_HOST_SS_SELECTOR       0x0C04
#define VMCS_HOST_DS_SELECTOR       0x0C06
#define VMCS_HOST_FS_SELECTOR       0x0C08
#define VMCS_HOST_GS_SELECTOR       0x0C0A
#define VMCS_HOST_TR_SELECTOR       0x0C0C

#define VMCS_PIN_BASED_VM_EXEC_CONTROL 0x4000
#define VMCS_CPU_BASED_VM_EXEC_CONTROL 0x4002
#define VMCS_EXCEPTION_BITMAP       0x4004
#define VMCS_PAGE_FAULT_ERROR_CODE_MASK 0x4006
#define VMCS_PAGE_FAULT_ERROR_CODE_MATCH 0x4008
#define VMCS_CR3_TARGET_COUNT       0x400A
#define VMCS_VM_EXIT_CONTROLS       0x400C
#define VMCS_VM_EXIT_MSR_STORE_COUNT 0x400E
#define VMCS_VM_EXIT_MSR_LOAD_COUNT 0x4010
#define VMCS_VM_ENTRY_CONTROLS      0x4012
#define VMCS_VM_ENTRY_MSR_LOAD_COUNT 0x4014
#define VMCS_VM_ENTRY_INTR_INFO_FIELD 0x4016
#define VMCS_VM_ENTRY_EXCEPTION_ERROR_CODE 0x4018
#define VMCS_VM_ENTRY_INSTRUCTION_LEN 0x401A
#define VMCS_TPR_THRESHOLD          0x401C
#define VMCS_SECONDARY_VM_EXEC_CONTROL 0x401E

#define VMCS_GUEST_CR0              0x6800
#define VMCS_GUEST_CR3              0x6802
#define VMCS_GUEST_CR4              0x6804
#define VMCS_GUEST_ES_BASE          0x6806
#define VMCS_GUEST_CS_BASE          0x6808
#define VMCS_GUEST_SS_BASE          0x680A
#define VMCS_GUEST_DS_BASE          0x680C
#define VMCS_GUEST_FS_BASE          0x680E
#define VMCS_GUEST_GS_BASE          0x6810
#define VMCS_GUEST_LDTR_BASE        0x6812
#define VMCS_GUEST_TR_BASE          0x6814
#define VMCS_GUEST_GDTR_BASE        0x6816
#define VMCS_GUEST_IDTR_BASE        0x6818
#define VMCS_GUEST_DR7              0x681A
#define VMCS_GUEST_RSP              0x681C
#define VMCS_GUEST_RIP              0x681E
#define VMCS_GUEST_RFLAGS           0x6820

#define VMCS_HOST_CR0               0x6C00
#define VMCS_HOST_CR3               0x6C02
#define VMCS_HOST_CR4               0x6C04
#define VMCS_HOST_FS_BASE           0x6C06
#define VMCS_HOST_GS_BASE           0x6C08
#define VMCS_HOST_TR_BASE           0x6C0A
#define VMCS_HOST_GDTR_BASE         0x6C0C
#define VMCS_HOST_IDTR_BASE         0x6C0E
#define VMCS_HOST_RSP               0x6C14
#define VMCS_HOST_RIP               0x6C16

/* VM Exit reasons */
#define EXIT_REASON_EXCEPTION_NMI   0
#define EXIT_REASON_EXTERNAL_INTERRUPT 1
#define EXIT_REASON_TRIPLE_FAULT    2
#define EXIT_REASON_INIT            3
#define EXIT_REASON_SIPI            4
#define EXIT_REASON_IO_SMI          5
#define EXIT_REASON_OTHER_SMI       6
#define EXIT_REASON_PENDING_VIRT_INTR 7
#define EXIT_REASON_PENDING_VIRT_NMI 8
#define EXIT_REASON_TASK_SWITCH     9
#define EXIT_REASON_CPUID           10
#define EXIT_REASON_GETSEC          11
#define EXIT_REASON_HLT             12
#define EXIT_REASON_INVD            13
#define EXIT_REASON_INVLPG          14
#define EXIT_REASON_RDPMC           15
#define EXIT_REASON_RDTSC           16
#define EXIT_REASON_RSM             17
#define EXIT_REASON_VMCALL          18
#define EXIT_REASON_VMCLEAR         19
#define EXIT_REASON_VMLAUNCH        20
#define EXIT_REASON_VMPTRLD         21
#define EXIT_REASON_VMPTRST         22
#define EXIT_REASON_VMREAD          23
#define EXIT_REASON_VMRESUME        24
#define EXIT_REASON_VMWRITE         25
#define EXIT_REASON_VMXOFF          26
#define EXIT_REASON_VMXON           27
#define EXIT_REASON_CR_ACCESS       28
#define EXIT_REASON_DR_ACCESS       29
#define EXIT_REASON_IO_INSTRUCTION  30
#define EXIT_REASON_MSR_READ        31
#define EXIT_REASON_MSR_WRITE       32

typedef struct {
    u8 vmx_supported;
    u8 vmx_enabled;
    u8 ept_supported;
    u8 vpid_supported;
    u8 unrestricted_guest;
    u32 vmcs_revision_id;
    u32 vmcs_size;
    u64 vmx_basic;
    u64 pin_based_ctls;
    u64 proc_based_ctls;
    u64 exit_ctls;
    u64 entry_ctls;
    u64 cr0_fixed0;
    u64 cr0_fixed1;
    u64 cr4_fixed0;
    u64 cr4_fixed1;
    void* vmxon_region;
    void* vmcs_region;
} vmx_info_t;

static vmx_info_t vmx_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 vmx_check_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (ecx & (1 << 5)) != 0;
}

static u8 vmx_check_feature_control(void) {
    u64 feature_control = msr_read(MSR_IA32_FEATURE_CONTROL);
    
    if (!(feature_control & FEATURE_CONTROL_LOCKED)) {
        feature_control |= FEATURE_CONTROL_LOCKED | FEATURE_CONTROL_VMX_OUTSIDE_SMX;
        msr_write(MSR_IA32_FEATURE_CONTROL, feature_control);
    }
    
    return (feature_control & FEATURE_CONTROL_VMX_OUTSIDE_SMX) != 0;
}

static void vmx_read_capabilities(void) {
    vmx_info.vmx_basic = msr_read(MSR_IA32_VMX_BASIC);
    vmx_info.vmcs_revision_id = vmx_info.vmx_basic & 0x7FFFFFFF;
    vmx_info.vmcs_size = (vmx_info.vmx_basic >> 32) & 0x1FFF;
    
    vmx_info.pin_based_ctls = msr_read(MSR_IA32_VMX_PINBASED_CTLS);
    vmx_info.proc_based_ctls = msr_read(MSR_IA32_VMX_PROCBASED_CTLS);
    vmx_info.exit_ctls = msr_read(MSR_IA32_VMX_EXIT_CTLS);
    vmx_info.entry_ctls = msr_read(MSR_IA32_VMX_ENTRY_CTLS);
    
    vmx_info.cr0_fixed0 = msr_read(MSR_IA32_VMX_CR0_FIXED0);
    vmx_info.cr0_fixed1 = msr_read(MSR_IA32_VMX_CR0_FIXED1);
    vmx_info.cr4_fixed0 = msr_read(MSR_IA32_VMX_CR4_FIXED0);
    vmx_info.cr4_fixed1 = msr_read(MSR_IA32_VMX_CR4_FIXED1);
    
    /* Check for EPT and VPID support */
    u64 ept_vpid_cap = msr_read(MSR_IA32_VMX_EPT_VPID_CAP);
    vmx_info.ept_supported = (ept_vpid_cap & (1 << 6)) != 0;
    vmx_info.vpid_supported = (ept_vpid_cap & (1 << 32)) != 0;
    
    /* Check for unrestricted guest support */
    u64 proc_based_ctls2 = msr_read(MSR_IA32_VMX_PROCBASED_CTLS2);
    vmx_info.unrestricted_guest = (proc_based_ctls2 & (1 << 7)) != 0;
}

static u8 vmx_setup_vmxon_region(void) {
    extern void* paging_alloc_pages(u32 count);
    
    vmx_info.vmxon_region = paging_alloc_pages(1);
    if (!vmx_info.vmxon_region) return 0;
    
    /* Clear VMXON region */
    u32* vmxon = (u32*)vmx_info.vmxon_region;
    for (u32 i = 0; i < 1024; i++) {
        vmxon[i] = 0;
    }
    
    /* Set VMCS revision ID */
    vmxon[0] = vmx_info.vmcs_revision_id;
    
    return 1;
}

static u8 vmx_setup_vmcs_region(void) {
    extern void* paging_alloc_pages(u32 count);
    
    vmx_info.vmcs_region = paging_alloc_pages(1);
    if (!vmx_info.vmcs_region) return 0;
    
    /* Clear VMCS region */
    u32* vmcs = (u32*)vmx_info.vmcs_region;
    for (u32 i = 0; i < 1024; i++) {
        vmcs[i] = 0;
    }
    
    /* Set VMCS revision ID */
    vmcs[0] = vmx_info.vmcs_revision_id;
    
    return 1;
}

static u8 vmx_enable_vmx_operation(void) {
    /* Set CR4.VMXE */
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 13);
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    /* Execute VMXON */
    u64 vmxon_phys = (u64)vmx_info.vmxon_region;
    u8 result;
    
    __asm__ volatile(
        "vmxon %1\n\t"
        "setna %0"
        : "=r"(result)
        : "m"(vmxon_phys)
        : "cc", "memory"
    );
    
    return result == 0;
}

void vmx_init(void) {
    if (!msr_is_supported()) return;
    
    vmx_info.vmx_supported = vmx_check_support();
    if (!vmx_info.vmx_supported) return;
    
    if (!vmx_check_feature_control()) return;
    
    vmx_read_capabilities();
    
    if (!vmx_setup_vmxon_region()) return;
    if (!vmx_setup_vmcs_region()) return;
    
    vmx_info.vmx_enabled = vmx_enable_vmx_operation();
}

u8 vmx_is_supported(void) {
    return vmx_info.vmx_supported;
}

u8 vmx_is_enabled(void) {
    return vmx_info.vmx_enabled;
}

u8 vmx_is_ept_supported(void) {
    return vmx_info.ept_supported;
}

u8 vmx_is_vpid_supported(void) {
    return vmx_info.vpid_supported;
}

u8 vmx_is_unrestricted_guest_supported(void) {
    return vmx_info.unrestricted_guest;
}

u32 vmx_get_vmcs_revision_id(void) {
    return vmx_info.vmcs_revision_id;
}

u32 vmx_get_vmcs_size(void) {
    return vmx_info.vmcs_size;
}

u64 vmx_get_pin_based_controls(void) {
    return vmx_info.pin_based_ctls;
}

u64 vmx_get_proc_based_controls(void) {
    return vmx_info.proc_based_ctls;
}

u64 vmx_get_exit_controls(void) {
    return vmx_info.exit_ctls;
}

u64 vmx_get_entry_controls(void) {
    return vmx_info.entry_ctls;
}

u8 vmx_vmclear(void* vmcs) {
    u64 vmcs_phys = (u64)vmcs;
    u8 result;
    
    __asm__ volatile(
        "vmclear %1\n\t"
        "setna %0"
        : "=r"(result)
        : "m"(vmcs_phys)
        : "cc", "memory"
    );
    
    return result == 0;
}

u8 vmx_vmptrld(void* vmcs) {
    u64 vmcs_phys = (u64)vmcs;
    u8 result;
    
    __asm__ volatile(
        "vmptrld %1\n\t"
        "setna %0"
        : "=r"(result)
        : "m"(vmcs_phys)
        : "cc", "memory"
    );
    
    return result == 0;
}

u8 vmx_vmread(u32 field, u64* value) {
    u8 result;
    
    __asm__ volatile(
        "vmread %2, %1\n\t"
        "setna %0"
        : "=r"(result), "=r"(*value)
        : "r"((u64)field)
        : "cc"
    );
    
    return result == 0;
}

u8 vmx_vmwrite(u32 field, u64 value) {
    u8 result;
    
    __asm__ volatile(
        "vmwrite %1, %2\n\t"
        "setna %0"
        : "=r"(result)
        : "r"(value), "r"((u64)field)
        : "cc"
    );
    
    return result == 0;
}

void vmx_disable(void) {
    if (!vmx_info.vmx_enabled) return;
    
    __asm__ volatile("vmxoff" : : : "cc");
    
    /* Clear CR4.VMXE */
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 &= ~(1 << 13);
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    vmx_info.vmx_enabled = 0;
}
