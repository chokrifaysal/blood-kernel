# x86 interrupt stubs - low-level handlers
.code32
.section .text

# Exception handlers (no error code)
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    push $0          # dummy error code
    push $\num       # interrupt number
    jmp isr_common_stub
.endm

# Exception handlers (with error code)
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    push $\num       # interrupt number
    jmp isr_common_stub
.endm

# IRQ handlers
.macro IRQ num, irq_num
.global irq\num
irq\num:
    cli
    push $0          # dummy error code
    push $\irq_num   # IRQ number
    jmp irq_common_stub
.endm

# Exception stubs
ISR_NOERRCODE 0   # Division by zero
ISR_NOERRCODE 1   # Debug
ISR_NOERRCODE 2   # NMI
ISR_NOERRCODE 3   # Breakpoint
ISR_NOERRCODE 4   # Overflow
ISR_NOERRCODE 5   # Bound range exceeded
ISR_NOERRCODE 6   # Invalid opcode
ISR_NOERRCODE 7   # Device not available
ISR_ERRCODE   8   # Double fault
ISR_ERRCODE   10  # Invalid TSS
ISR_ERRCODE   11  # Segment not present
ISR_ERRCODE   12  # Stack fault
ISR_ERRCODE   13  # General protection fault
ISR_ERRCODE   14  # Page fault
ISR_NOERRCODE 16  # x87 FPU error
ISR_ERRCODE   17  # Alignment check
ISR_NOERRCODE 18  # Machine check
ISR_NOERRCODE 19  # SIMD FPU error

# IRQ stubs
IRQ 0,  32   # PIT timer
IRQ 1,  33   # Keyboard
IRQ 2,  34   # Cascade
IRQ 3,  35   # COM2
IRQ 4,  36   # COM1
IRQ 5,  37   # LPT2
IRQ 6,  38   # Floppy
IRQ 7,  39   # LPT1
IRQ 8,  40   # RTC
IRQ 9,  41   # Free
IRQ 10, 42   # Free
IRQ 11, 43   # Free
IRQ 12, 44   # PS/2 mouse
IRQ 13, 45   # FPU
IRQ 14, 46   # Primary ATA
IRQ 15, 47   # Secondary ATA

# Common exception handler
isr_common_stub:
    pusha            # Save all general purpose registers
    
    mov %ds, %ax     # Save data segment
    push %eax
    
    mov $0x10, %ax   # Load kernel data segment
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    call isr_handler # Call C handler
    
    pop %eax         # Restore data segment
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    popa             # Restore registers
    add $8, %esp     # Clean up error code and interrupt number
    sti
    iret             # Return from interrupt

# Common IRQ handler
irq_common_stub:
    pusha            # Save all general purpose registers
    
    mov %ds, %ax     # Save data segment
    push %eax
    
    mov $0x10, %ax   # Load kernel data segment
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    call irq_handler # Call C handler
    
    pop %eax         # Restore data segment
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    popa             # Restore registers
    add $8, %esp     # Clean up error code and IRQ number
    sti
    iret             # Return from interrupt

# System call handler (INT 0x80)
.global syscall_handler
syscall_handler:
    cli
    pusha
    
    mov %ds, %ax
    push %eax
    
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    # System call number in EAX
    # Parameters in EBX, ECX, EDX, ESI, EDI
    push %edi
    push %esi
    push %edx
    push %ecx
    push %ebx
    push %eax
    
    call syscall_dispatch
    
    add $24, %esp    # Clean up parameters
    
    pop %ebx         # Restore data segment
    mov %bx, %ds
    mov %bx, %es
    mov %bx, %fs
    mov %bx, %gs
    
    popa
    sti
    iret

# Enable interrupts
.global enable_interrupts
enable_interrupts:
    sti
    ret

# Disable interrupts
.global disable_interrupts
disable_interrupts:
    cli
    ret

# Get interrupt flag
.global get_interrupt_flag
get_interrupt_flag:
    pushf
    pop %eax
    and $0x200, %eax  # IF flag
    shr $9, %eax
    ret

# Atomic operations
.global atomic_inc
atomic_inc:
    mov 4(%esp), %eax
    lock incl (%eax)
    ret

.global atomic_dec
atomic_dec:
    mov 4(%esp), %eax
    lock decl (%eax)
    ret

.global atomic_add
atomic_add:
    mov 4(%esp), %eax  # ptr
    mov 8(%esp), %edx  # value
    lock addl %edx, (%eax)
    ret

.global atomic_sub
atomic_sub:
    mov 4(%esp), %eax  # ptr
    mov 8(%esp), %edx  # value
    lock subl %edx, (%eax)
    ret

.global atomic_xchg
atomic_xchg:
    mov 4(%esp), %eax  # ptr
    mov 8(%esp), %edx  # new value
    lock xchgl %edx, (%eax)
    mov %edx, %eax     # return old value
    ret

.global atomic_cmpxchg
atomic_cmpxchg:
    mov 4(%esp), %edx  # ptr
    mov 8(%esp), %eax  # expected
    mov 12(%esp), %ecx # new value
    lock cmpxchgl %ecx, (%edx)
    ret

# Memory barriers
.global memory_barrier
memory_barrier:
    mfence
    ret

.global read_barrier
read_barrier:
    lfence
    ret

.global write_barrier
write_barrier:
    sfence
    ret

# CPU identification
.global cpuid_supported
cpuid_supported:
    pushf
    pop %eax
    mov %eax, %ecx
    xor $0x200000, %eax  # Flip ID bit
    push %eax
    popf
    pushf
    pop %eax
    xor %ecx, %eax       # Check if bit changed
    shr $21, %eax
    and $1, %eax
    ret

.global get_cpuid
get_cpuid:
    push %ebx
    push %ecx
    push %edx
    
    mov 20(%esp), %eax   # function
    cpuid
    
    mov 24(%esp), %esi   # output array
    mov %eax, 0(%esi)
    mov %ebx, 4(%esi)
    mov %ecx, 8(%esi)
    mov %edx, 12(%esi)
    
    pop %edx
    pop %ecx
    pop %ebx
    ret
