/*
 * virt_ext.c – x86 CPU virtualization extensions (VT-x enhancements)
 */

#include "kernel/types.h"

/* VMX capability MSRs */
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
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS  0x48D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x48E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS      0x48F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS     0x490
#define MSR_IA32_VMX_VMFUNC         0x491

/* VMCS field encodings */
#define VMCS_GUEST_ES_SELECTOR      0x800
#define VMCS_GUEST_CS_SELECTOR      0x802
#define VMCS_GUEST_SS_SELECTOR      0x804
#define VMCS_GUEST_DS_SELECTOR      0x806
#define VMCS_GUEST_FS_SELECTOR      0x808
#define VMCS_GUEST_GS_SELECTOR      0x80A
#define VMCS_GUEST_LDTR_SELECTOR    0x80C
#define VMCS_GUEST_TR_SELECTOR      0x80E
#define VMCS_GUEST_INTR_STATUS      0x810
#define VMCS_GUEST_PML_INDEX        0x812

#define VMCS_HOST_ES_SELECTOR       0xC00
#define VMCS_HOST_CS_SELECTOR       0xC02
#define VMCS_HOST_SS_SELECTOR       0xC04
#define VMCS_HOST_DS_SELECTOR       0xC06
#define VMCS_HOST_FS_SELECTOR       0xC08
#define VMCS_HOST_GS_SELECTOR       0xC0A
#define VMCS_HOST_TR_SELECTOR       0xC0C

/* VMX operation states */
#define VMX_STATE_OFF               0
#define VMX_STATE_ROOT              1
#define VMX_STATE_NON_ROOT          2

/* EPT capabilities */
#define EPT_CAP_MEMORY_TYPE_WB      (1 << 14)
#define EPT_CAP_PAGE_WALK_4         (1 << 6)
#define EPT_CAP_INVEPT              (1 << 20)
#define EPT_CAP_INVVPID             (1 << 32)

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

typedef struct {
    u8 virt_ext_supported;
    u8 vmx_supported;
    u8 svm_supported;
    u8 vmx_enabled;
    u8 vmx_operation_state;
    u8 ept_supported;
    u8 vpid_supported;
    u8 unrestricted_guest_supported;
    u8 vmfunc_supported;
    u8 nested_virtualization_supported;
    vmx_capabilities_t vmx_caps;
    u64 vmxon_region_pa;
    u64 vmcs_region_pa;
    u32 vmcs_revision_id;
    u16 current_vpid;
    u32 vm_exit_count;
    u32 vm_entry_count;
} virt_ext_info_t;

static virt_ext_info_t virt_ext_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void virt_ext_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for VMX support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    virt_ext_info.vmx_supported = (ecx & (1 << 5)) != 0;
    
    /* Check for SVM support (AMD) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    virt_ext_info.svm_supported = (ecx & (1 << 2)) != 0;
    
    if (virt_ext_info.vmx_supported || virt_ext_info.svm_supported) {
        virt_ext_info.virt_ext_supported = 1;
    }
}

static void virt_ext_read_vmx_capabilities(void) {
    if (!virt_ext_info.vmx_supported || !msr_is_supported()) return;
    
    /* Read basic VMX capabilities */
    virt_ext_info.vmx_caps.basic = msr_read(MSR_IA32_VMX_BASIC);
    virt_ext_info.vmcs_revision_id = virt_ext_info.vmx_caps.basic & 0x7FFFFFFF;
    
    /* Read control capabilities */
    u64 pinbased = msr_read(MSR_IA32_VMX_PINBASED_CTLS);
    virt_ext_info.vmx_caps.pinbased_ctls_default0 = pinbased & 0xFFFFFFFF;
    virt_ext_info.vmx_caps.pinbased_ctls_default1 = pinbased >> 32;
    
    u64 procbased = msr_read(MSR_IA32_VMX_PROCBASED_CTLS);
    virt_ext_info.vmx_caps.procbased_ctls_default0 = procbased & 0xFFFFFFFF;
    virt_ext_info.vmx_caps.procbased_ctls_default1 = procbased >> 32;
    
    u64 exit_ctls = msr_read(MSR_IA32_VMX_EXIT_CTLS);
    virt_ext_info.vmx_caps.exit_ctls_default0 = exit_ctls & 0xFFFFFFFF;
    virt_ext_info.vmx_caps.exit_ctls_default1 = exit_ctls >> 32;
    
    u64 entry_ctls = msr_read(MSR_IA32_VMX_ENTRY_CTLS);
    virt_ext_info.vmx_caps.entry_ctls_default0 = entry_ctls & 0xFFFFFFFF;
    virt_ext_info.vmx_caps.entry_ctls_default1 = entry_ctls >> 32;
    
    /* Read additional capabilities */
    virt_ext_info.vmx_caps.misc = msr_read(MSR_IA32_VMX_MISC);
    virt_ext_info.vmx_caps.cr0_fixed0 = msr_read(MSR_IA32_VMX_CR0_FIXED0);
    virt_ext_info.vmx_caps.cr0_fixed1 = msr_read(MSR_IA32_VMX_CR0_FIXED1);
    virt_ext_info.vmx_caps.cr4_fixed0 = msr_read(MSR_IA32_VMX_CR4_FIXED0);
    virt_ext_info.vmx_caps.cr4_fixed1 = msr_read(MSR_IA32_VMX_CR4_FIXED1);
    virt_ext_info.vmx_caps.vmcs_enum = msr_read(MSR_IA32_VMX_VMCS_ENUM);
    
    /* Check for secondary processor-based controls */
    if (virt_ext_info.vmx_caps.procbased_ctls_default1 & (1 << 31)) {
        u64 procbased2 = msr_read(MSR_IA32_VMX_PROCBASED_CTLS2);
        virt_ext_info.vmx_caps.procbased_ctls2_default0 = procbased2 & 0xFFFFFFFF;
        virt_ext_info.vmx_caps.procbased_ctls2_default1 = procbased2 >> 32;
        
        /* Check for EPT and VPID */
        virt_ext_info.ept_supported = (virt_ext_info.vmx_caps.procbased_ctls2_default1 & (1 << 1)) != 0;
        virt_ext_info.vpid_supported = (virt_ext_info.vmx_caps.procbased_ctls2_default1 & (1 << 5)) != 0;
        virt_ext_info.unrestricted_guest_supported = (virt_ext_info.vmx_caps.procbased_ctls2_default1 & (1 << 7)) != 0;
        
        if (virt_ext_info.ept_supported || virt_ext_info.vpid_supported) {
            virt_ext_info.vmx_caps.ept_vpid_cap = msr_read(MSR_IA32_VMX_EPT_VPID_CAP);
        }
    }
    
    /* Check for VMFUNC */
    if (virt_ext_info.vmx_caps.procbased_ctls2_default1 & (1 << 13)) {
        virt_ext_info.vmfunc_supported = 1;
        virt_ext_info.vmx_caps.vmfunc_cap = msr_read(MSR_IA32_VMX_VMFUNC);
    }
    
    /* Check for nested virtualization */
    virt_ext_info.nested_virtualization_supported = (virt_ext_info.vmx_caps.misc & (1ULL << 5)) != 0;
}

void virt_ext_init(void) {
    virt_ext_info.virt_ext_supported = 0;
    virt_ext_info.vmx_supported = 0;
    virt_ext_info.svm_supported = 0;
    virt_ext_info.vmx_enabled = 0;
    virt_ext_info.vmx_operation_state = VMX_STATE_OFF;
    virt_ext_info.ept_supported = 0;
    virt_ext_info.vpid_supported = 0;
    virt_ext_info.unrestricted_guest_supported = 0;
    virt_ext_info.vmfunc_supported = 0;
    virt_ext_info.nested_virtualization_supported = 0;
    virt_ext_info.vmxon_region_pa = 0;
    virt_ext_info.vmcs_region_pa = 0;
    virt_ext_info.vmcs_revision_id = 0;
    virt_ext_info.current_vpid = 1;
    virt_ext_info.vm_exit_count = 0;
    virt_ext_info.vm_entry_count = 0;
    
    virt_ext_detect_capabilities();
    virt_ext_read_vmx_capabilities();
}

u8 virt_ext_is_supported(void) {
    return virt_ext_info.virt_ext_supported;
}

u8 virt_ext_is_vmx_supported(void) {
    return virt_ext_info.vmx_supported;
}

u8 virt_ext_is_svm_supported(void) {
    return virt_ext_info.svm_supported;
}

u8 virt_ext_is_ept_supported(void) {
    return virt_ext_info.ept_supported;
}

u8 virt_ext_is_vpid_supported(void) {
    return virt_ext_info.vpid_supported;
}

u8 virt_ext_is_unrestricted_guest_supported(void) {
    return virt_ext_info.unrestricted_guest_supported;
}

u8 virt_ext_is_vmfunc_supported(void) {
    return virt_ext_info.vmfunc_supported;
}

u8 virt_ext_is_nested_virtualization_supported(void) {
    return virt_ext_info.nested_virtualization_supported;
}

u8 virt_ext_enable_vmx(void) {
    if (!virt_ext_info.vmx_supported) return 0;
    
    extern u8 cpu_features_enable_vmx(void);
    if (!cpu_features_enable_vmx()) return 0;
    
    /* Set CR4.VMXE */
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 13); /* VMXE bit */
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    virt_ext_info.vmx_enabled = 1;
    return 1;
}

u8 virt_ext_is_vmx_enabled(void) {
    return virt_ext_info.vmx_enabled;
}

u8 virt_ext_vmxon(u64 vmxon_region_pa) {
    if (!virt_ext_info.vmx_enabled) return 0;
    
    virt_ext_info.vmxon_region_pa = vmxon_region_pa;
    
    /* Write VMCS revision ID to VMXON region */
    *(u32*)vmxon_region_pa = virt_ext_info.vmcs_revision_id;
    
    u8 result;
    __asm__ volatile(
        "vmxon %1\n\t"
        "setna %0"
        : "=q"(result)
        : "m"(vmxon_region_pa)
        : "cc", "memory"
    );
    
    if (result == 0) {
        virt_ext_info.vmx_operation_state = VMX_STATE_ROOT;
        return 1;
    }
    
    return 0;
}

u8 virt_ext_vmxoff(void) {
    if (virt_ext_info.vmx_operation_state != VMX_STATE_ROOT) return 0;
    
    u8 result;
    __asm__ volatile(
        "vmxoff\n\t"
        "setna %0"
        : "=q"(result)
        :
        : "cc", "memory"
    );
    
    if (result == 0) {
        virt_ext_info.vmx_operation_state = VMX_STATE_OFF;
        return 1;
    }
    
    return 0;
}

u8 virt_ext_vmptrld(u64 vmcs_pa) {
    if (virt_ext_info.vmx_operation_state != VMX_STATE_ROOT) return 0;
    
    virt_ext_info.vmcs_region_pa = vmcs_pa;
    
    /* Write VMCS revision ID to VMCS region */
    *(u32*)vmcs_pa = virt_ext_info.vmcs_revision_id;
    
    u8 result;
    __asm__ volatile(
        "vmptrld %1\n\t"
        "setna %0"
        : "=q"(result)
        : "m"(vmcs_pa)
        : "cc", "memory"
    );
    
    return result == 0;
}

u8 virt_ext_vmclear(u64 vmcs_pa) {
    if (virt_ext_info.vmx_operation_state != VMX_STATE_ROOT) return 0;
    
    u8 result;
    __asm__ volatile(
        "vmclear %1\n\t"
        "setna %0"
        : "=q"(result)
        : "m"(vmcs_pa)
        : "cc", "memory"
    );
    
    return result == 0;
}

u8 virt_ext_vmread(u64 field, u64* value) {
    if (virt_ext_info.vmx_operation_state != VMX_STATE_ROOT) return 0;
    
    u8 result;
    __asm__ volatile(
        "vmread %2, %1\n\t"
        "setna %0"
        : "=q"(result), "=r"(*value)
        : "r"(field)
        : "cc"
    );
    
    return result == 0;
}

u8 virt_ext_vmwrite(u64 field, u64 value) {
    if (virt_ext_info.vmx_operation_state != VMX_STATE_ROOT) return 0;
    
    u8 result;
    __asm__ volatile(
        "vmwrite %2, %1\n\t"
        "setna %0"
        : "=q"(result)
        : "r"(field), "r"(value)
        : "cc"
    );
    
    return result == 0;
}

u8 virt_ext_vmlaunch(void) {
    if (virt_ext_info.vmx_operation_state != VMX_STATE_ROOT) return 0;
    
    u8 result;
    __asm__ volatile(
        "vmlaunch\n\t"
        "setna %0"
        : "=q"(result)
        :
        : "cc", "memory"
    );
    
    if (result == 0) {
        virt_ext_info.vm_entry_count++;
        virt_ext_info.vmx_operation_state = VMX_STATE_NON_ROOT;
    }
    
    return result == 0;
}

u8 virt_ext_vmresume(void) {
    if (virt_ext_info.vmx_operation_state != VMX_STATE_ROOT) return 0;
    
    u8 result;
    __asm__ volatile(
        "vmresume\n\t"
        "setna %0"
        : "=q"(result)
        :
        : "cc", "memory"
    );
    
    if (result == 0) {
        virt_ext_info.vm_entry_count++;
        virt_ext_info.vmx_operation_state = VMX_STATE_NON_ROOT;
    }
    
    return result == 0;
}

void virt_ext_handle_vmexit(void) {
    virt_ext_info.vm_exit_count++;
    virt_ext_info.vmx_operation_state = VMX_STATE_ROOT;
    
    /* Read exit reason and handle accordingly */
    u64 exit_reason;
    if (virt_ext_vmread(0x4402, &exit_reason)) { /* VM_EXIT_REASON */
        /* Handle specific exit reasons here */
        switch (exit_reason & 0xFFFF) {
            case 0: /* Exception or NMI */
                break;
            case 1: /* External interrupt */
                break;
            case 10: /* CPUID */
                break;
            case 12: /* HLT */
                break;
            case 18: /* VMCALL */
                break;
            case 28: /* Control register access */
                break;
            case 30: /* I/O instruction */
                break;
            case 31: /* RDMSR */
                break;
            case 32: /* WRMSR */
                break;
            case 48: /* EPT violation */
                break;
            default:
                break;
        }
    }
}

u32 virt_ext_get_vm_exit_count(void) {
    return virt_ext_info.vm_exit_count;
}

u32 virt_ext_get_vm_entry_count(void) {
    return virt_ext_info.vm_entry_count;
}

u8 virt_ext_get_vmx_operation_state(void) {
    return virt_ext_info.vmx_operation_state;
}

const vmx_capabilities_t* virt_ext_get_vmx_capabilities(void) {
    return &virt_ext_info.vmx_caps;
}

u16 virt_ext_allocate_vpid(void) {
    if (!virt_ext_info.vpid_supported) return 0;
    
    return virt_ext_info.current_vpid++;
}

u8 virt_ext_invept(u32 type, u64 eptp) {
    if (!virt_ext_info.ept_supported) return 0;
    
    struct {
        u64 eptp;
        u64 reserved;
    } descriptor = { eptp, 0 };
    
    u8 result;
    __asm__ volatile(
        "invept %1, %2\n\t"
        "setna %0"
        : "=q"(result)
        : "r"((u64)type), "m"(descriptor)
        : "cc"
    );
    
    return result == 0;
}

u8 virt_ext_invvpid(u32 type, u16 vpid, u64 linear_address) {
    if (!virt_ext_info.vpid_supported) return 0;
    
    struct {
        u16 vpid;
        u16 reserved1;
        u32 reserved2;
        u64 linear_address;
    } descriptor = { vpid, 0, 0, linear_address };
    
    u8 result;
    __asm__ volatile(
        "invvpid %1, %2\n\t"
        "setna %0"
        : "=q"(result)
        : "r"((u64)type), "m"(descriptor)
        : "cc"
    );
    
    return result == 0;
}
