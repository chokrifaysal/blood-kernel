# x86-64 PC Support

**Intel/AMD x86-64 PC with standard hardware**

## Features
- **IDT**: 256-entry Interrupt Descriptor Table with exception handling
- **PIC**: 8259A Programmable Interrupt Controller with IRQ management
- **MMU**: 4KB page-based memory management with demand paging
- **CPUID**: CPU identification and feature detection
- **CPU Instruction Extensions**: AVX-512, AMX, and APX instruction sets
- **System Management Mode**: SMM/SMI handling and SMRAM management
- **Thermal Management**: RAPL power limits and thermal throttling
- **CPU Security**: CET, MPX, SMEP, SMAP, and PKU security features
- **Memory Protection**: Advanced memory protection with domains and keys
- **CPU Debug Extensions**: Intel PT, LBR, and BTS debugging support
- **Virtualization**: VMX/SVM hardware virtualization support
- **IOMMU**: Intel VT-d and AMD-Vi I/O memory management
- **Performance Monitoring**: PMC/PMU architectural performance counters
- **CPU Topology**: Multi-core/thread topology detection and NUMA awareness
- **Interrupt Routing**: I/O APIC, MSI, and MSI-X interrupt routing
- **CPU Errata**: Microcode management and errata workaround application
- **Cache Coherency**: MESI/MOESI cache coherency protocols and monitoring
- **Memory Ordering**: Memory barriers, fencing, and atomic operations
- **Power States**: C-states/P-states power management and thermal control
- **Microarch**: CPU microarchitecture control and speculation mitigation
- **APIC Ext**: Advanced interrupt handling with x2APIC and IPI support
- **Timing Sync**: TSC/HPET timing and CPU synchronization primitives
- **Freq Scaling**: P-states/Turbo frequency scaling and power management
- **System Ctrl**: SMM/ACPI system control and power state management
- **CPU Ext**: AVX-512/AMX instruction set extensions and XSAVE
- **Exception Mgmt**: CPU fault injection and recovery management
- **Memory Adv**: PCID/INVPCID advanced memory management
- **Security Ext**: MPX/CET security extensions and enforcement
- **Debug HW**: Hardware breakpoints/watchpoints and tracing
- **PMU Ext**: Advanced performance monitoring unit extensions
- **Trace HW**: Intel PT/BTS/LBR instruction tracing
- **Power Mgmt**: Advanced C-states/P-states deep control
- **Memory Protection**: SMEP/SMAP/CET/PKU security extensions
- **Virtualization Ext**: VT-x/EPT/VPID virtualization enhancements
- **SIMD**: SSE/AVX/AVX-512 instruction set optimization
- **Interrupt Mgmt**: NMI/SMI/MCE advanced interrupt handling
- **Atomic**: CPU synchronization primitives and operations
- **Cache Mgmt**: L1/L2/L3 cache control and optimization
- **Memory Mgmt**: PAT/MTRR advanced memory management
- **CPU Features**: Model-specific feature control and configuration
- **CPUFreq**: SpeedStep/Cool'n'Quiet frequency scaling
- **IOAPIC**: I/O Advanced Programmable Interrupt Controller
- **CPUID Ext**: Extended feature detection and management
- **Security**: CET/SMEP/SMAP/MPX/PKU security features
- **Debug**: LBR/BTS branch tracing and debugging
- **Errata**: CPU bug detection and workarounds
- **Topology**: CPU/NUMA topology detection and enumeration
- **XSAVE**: Extended state management for AVX/AVX-512/MPX
- **NUMA**: Memory affinity and allocation policies
- **LongMode**: x86-64 64-bit support with PAE and NX
- **Microcode**: CPU firmware update support for Intel/AMD
- **x2APIC**: Advanced interrupt controller with MSR interface
- **Cache**: MTRR/PAT memory type and cache control
- **VMX**: Intel VT-x virtualization with VMCS support
- **PerfMon**: CPU performance counters and profiling
- **Thermal**: CPU temperature monitoring with digital sensors
- **Power**: P-states/C-states CPU power management
- **IOMMU**: Intel VT-d/AMD-Vi memory translation
- **HPET**: High Precision Event Timer with nanosecond accuracy
- **MSR**: Model Specific Registers for CPU control and monitoring
- **SMBIOS**: System Management BIOS for hardware information
- **DMA**: 8237A Direct Memory Access controller with 8 channels
- **AC97**: Audio Codec '97 sound card with PCM playback
- **RTL8139**: Realtek Fast Ethernet 10/100 network controller
- **ACPI**: Advanced Configuration and Power Interface with RSDP/RSDT
- **APIC**: Advanced Programmable Interrupt Controller with IPI support
- **USB**: UHCI Universal Host Controller Interface for USB 1.1
- **RTC**: MC146818 Real-Time Clock with CMOS memory access
- **Serial**: 16550 UART COM1/COM2/COM3/COM4 with interrupt buffering
- **Floppy**: 82077AA controller for 1.44MB/720KB/1.2MB/360KB drives
- **VGA Text Mode**: 80x25 color display @ 0xB8000
- **PS/2 Keyboard**: Full scan code translation with modifiers
- **PCI Bus**: Device enumeration and configuration
- **PIT Timer**: 8253/8254 programmable interval timer @ 1 kHz
- **ATA/IDE**: Hard disk and CD-ROM support
- **PC Speaker**: Beep generation via PIT channel 2

## Memory Map
- **Kernel**: 0x00100000-0x00400000 (3 MB)
- **Page Directory**: 4 KB aligned
- **Page Tables**: 4 KB each
- **Heap**: 0xD0000000-0xE0000000 (256 MB)
- **Local APIC**: 0xFEE00000 (4 kB)
- **I/O APIC**: 0xFEC00000 (4 kB)
- **VGA Buffer**: 0xB8000 (32 kB)
- **DMA**: 0x00-0x0F, 0xC0-0xDF
- **CMOS/RTC**: 0x70/0x71
- **COM1**: 0x3F8-0x3FF (IRQ4)
- **COM2**: 0x2F8-0x2FF (IRQ3)
- **Floppy**: 0x3F0-0x3F7 (IRQ6, DMA2)
- **PIC**: 0x20/0x21, 0xA0/0xA1
- **PCI Config**: 0xCF8/0xCFC
- **PIT**: 0x40-0x43
- **PS/2**: 0x60/0x64
- **PC Speaker**: 0x61

## Build & Boot
```bash
make ARCH=x86
qemu-system-x86_64 -kernel build/kernel.bin
```

## Hardware Drivers

### Interrupt System
- IDT with 256 entries for exceptions and IRQs
- Exception handlers for all x86 faults
- IRQ routing through 8259A PIC
- Atomic operations and memory barriers

### Memory Management
- 4KB page-based virtual memory
- Identity mapping for kernel space
- Higher-half kernel at 0xC0000000
- Demand paging for heap allocation
- Physical page bitmap allocator

### CPU Identification
- CPUID instruction support detection
- Vendor string (Intel, AMD, etc.)
- CPU brand string and model info
- Feature flags (SSE, AVX, etc.)
- TSC and RDRAND support

### High Precision Timer
- HPET with femtosecond resolution
- Multiple timer comparators
- Periodic and one-shot modes
- TSC frequency calibration
- Nanosecond delay functions

### Model Specific Registers
- MSR read/write operations
- TSC frequency detection
- APIC base address management
- MTRR memory type configuration
- Performance counter setup

### Thermal Management
- Digital temperature sensors
- TjMax detection and monitoring
- Thermal throttling detection
- Critical temperature protection
- Thermal event logging

### Power Management
- P-state frequency scaling
- C-state idle management
- Turbo boost control
- Clock modulation throttling
- Energy performance bias

### Cache Management
- MTRR variable and fixed range registers
- PAT (Page Attribute Table) support
- Cache line flushing and invalidation
- Memory type configuration (UC/WC/WT/WB)
- Cache size detection and prefetch control

### Virtualization Support
- Intel VT-x (VMX) implementation
- VMCS (Virtual Machine Control Structure)
- EPT (Extended Page Tables) support
- VPID (Virtual Processor ID) support
- VM entry/exit control and monitoring

### Long Mode Support
- x86-64 64-bit architecture support
- 4-level page table management (PML4/PDPT/PD/PT)
- NX (No-Execute) bit support
- SYSCALL/SYSRET fast system calls
- SMEP/SMAP security features

### Microcode Updates
- Intel and AMD microcode support
- Microcode validation and verification
- Runtime microcode application
- Processor signature matching
- Revision tracking and management

### CPU Topology Detection
- CPUID-based topology enumeration
- Package/die/core/thread hierarchy
- Cache sharing level detection
- NUMA node assignment
- x2APIC ID extraction and parsing

### Extended State Management
- XSAVE/XRSTOR instruction support
- AVX/AVX-512 state preservation
- MPX bound register management
- Feature-specific state areas
- Supervisor state handling

### CPU Security Features
- SMEP/SMAP supervisor mode protection
- CET (Control Flow Enforcement Technology)
- Shadow stack and indirect branch tracking
- MPX (Memory Protection Extensions)
- PKU (Protection Key for Userspace)

### Advanced Debugging
- LBR (Last Branch Record) stack
- BTS (Branch Trace Store) buffer
- Branch filtering and profiling
- Single-step branch debugging
- PMI freeze on debug events

### CPU Frequency Scaling
- Intel Enhanced SpeedStep support
- AMD Cool'n'Quiet technology
- P-state enumeration and control
- Turbo boost management
- Dynamic frequency adjustment

### IOAPIC Management
- Multiple IOAPIC controller support
- ACPI MADT table parsing
- GSI (Global System Interrupt) routing
- Redirection table configuration
- Legacy PIC replacement

### Cache Management
- L1/L2/L3 cache hierarchy control
- Cache line flushing and writeback
- CLFLUSH/CLFLUSHOPT/CLWB instructions
- Cache prefetching and optimization
- Cache allocation technology support

### Advanced Memory Management
- PAT (Page Attribute Table) configuration
- MTRR (Memory Type Range Register) control
- Variable and fixed MTRR support
- Memory type optimization (WB/WT/WC/UC)
- TLB management and invalidation

### SIMD Optimization
- SSE/SSE2/SSE3/SSSE3/SSE4 instruction sets
- AVX/AVX2/AVX-512 vector processing
- FMA (Fused Multiply-Add) operations
- Optimized memcpy/memset/strcmp functions
- SIMD state save/restore management

### Advanced Interrupt Management
- NMI (Non-Maskable Interrupt) control
- SMI (System Management Interrupt) handling
- MCE (Machine Check Exception) processing
- Thermal interrupt management
- Interrupt nesting and statistics

### Advanced Power Management
- C-states (C0-C8) deep sleep control
- Package C-state coordination
- Energy monitoring and RAPL
- Power limiting and TDP management
- MWAIT-based idle optimization

### Memory Protection Extensions
- SMEP (Supervisor Mode Execution Prevention)
- SMAP (Supervisor Mode Access Prevention)
- PKU (Protection Keys for Userspace)
- CET (Control-flow Enforcement Technology)
- Shadow stack and indirect branch tracking

### Hardware Debugging
- Hardware breakpoints and watchpoints
- Debug register (DR0-DR7) management
- Single-step execution control
- Branch tracing and general detect
- I/O breakpoint support

### PMU Extensions
- Advanced performance monitoring
- General-purpose and fixed counters
- PEBS (Precise Event-Based Sampling)
- LBR (Last Branch Record) stack
- Performance monitoring interrupts

### Exception Management
- CPU fault injection and recovery
- Exception logging and statistics
- Recoverable exception handling
- Double fault and machine check tracking
- Configurable exception modes

### Advanced Memory Management
- PCID (Process Context Identifiers)
- INVPCID instruction support
- PAT (Page Attribute Table) control
- MTRR (Memory Type Range Registers)
- TLB management and optimization

### Frequency Scaling
- P-states and Turbo Boost control
- Intel SpeedStep and AMD Cool'n'Quiet
- Energy performance bias configuration
- Dynamic frequency adjustment
- Power consumption optimization

### System Control
- SMM (System Management Mode) support
- ACPI power state management (S0-S5)
- System reset and shutdown control
- Wake event management
- Uptime and reset cause tracking

### Microarchitecture Control
- Branch prediction and speculation control
- IBRS/STIBP/SSBD mitigation support
- Indirect branch prediction barrier (IBPB)
- LBR (Last Branch Record) monitoring
- Vulnerability detection and mitigation

### APIC Extensions
- x2APIC mode with MSR-based access
- Inter-processor interrupt (IPI) management
- APIC timer with multiple modes
- Interrupt priority and error handling
- Broadcast and targeted IPI operations

### Cache Coherency
- MESI/MOESI cache coherency protocols
- Cache line state monitoring and tracking
- Cache flush, invalidate, and writeback operations
- Snoop hit/miss detection and handling
- Cache hierarchy enumeration and control

### Memory Ordering
- Memory barriers and fencing (LFENCE/SFENCE/MFENCE)
- Atomic operations with memory ordering semantics
- TSO (Total Store Order) memory model support
- Memory access violation detection
- Compiler and hardware barrier synchronization

### CPU Topology
- Multi-core and multi-thread topology detection
- APIC ID parsing and hierarchy enumeration
- Package/die/core/thread relationship mapping
- Cache sharing level detection and analysis
- NUMA node awareness and CPU affinity
- Hybrid topology support (P-cores + E-cores)

### Interrupt Routing
- I/O APIC redirection table management
- MSI (Message Signaled Interrupts) configuration
- MSI-X advanced interrupt handling
- PCI interrupt routing and delivery modes
- Interrupt load balancing and affinity control

### Virtualization Extensions
- Intel VMX (Virtual Machine Extensions) support
- AMD SVM (Secure Virtual Machine) support
- VMCS (Virtual Machine Control Structure) management
- VMCB (Virtual Machine Control Block) handling
- EPT (Extended Page Tables) and NPT (Nested Page Tables)
- VPID (Virtual Processor Identifier) support

### IOMMU Support
- Intel VT-d (Virtualization Technology for Directed I/O)
- AMD-Vi (AMD I/O Virtualization Technology)
- DMA remapping and translation
- Interrupt remapping and routing
- DMAR (DMA Remapping) unit management
- IOTLB (I/O Translation Lookaside Buffer) control

### Performance Monitoring
- Architectural Performance Monitoring (PMU)
- General-purpose performance counters (PMC)
- Fixed-function performance counters
- Instructions retired and CPU cycles tracking
- Cache miss and branch prediction monitoring
- IPC (Instructions Per Cycle) calculation

### CPU Errata Management
- Intel and AMD errata database and detection
- Microcode revision tracking and updates
- Security vulnerability assessment
- Workaround application and management
- Critical errata identification and mitigation

### Power States
- C-states (C0-C10) power management
- P-states (P0-P7) performance scaling
- MWAIT-based deep sleep states
- Thermal monitoring and throttling detection
- Power transition tracking and statistics

### Timing and Synchronization
- TSC (Time Stamp Counter) with invariant support
- HPET (High Precision Event Timer) control
- CPU synchronization barriers
- Latency measurement and calibration
- RDTSCP with CPU ID support

### CPU Instruction Extensions
- AVX-512 (Advanced Vector Extensions 512-bit)
- AMX (Advanced Matrix Extensions) for AI/ML workloads
- APX (Advanced Performance Extensions)
- XSAVE/XRSTOR context management
- Tile register configuration for matrix operations

### System Management Mode
- SMI (System Management Interrupt) handling
- SMRAM (System Management RAM) management
- SMM handler installation and execution
- SMI source control and monitoring
- SMM state save and restore operations

### Thermal Management
- RAPL (Running Average Power Limit) control
- Thermal monitoring and temperature reading
- Thermal throttling and PROCHOT detection
- Power limit enforcement and violation tracking
- Critical temperature protection

### CPU Extensions
- AVX-512 instruction set support
- AMX (Advanced Matrix Extensions)
- XSAVE/XRSTOR state management
- Tile configuration for AMX
- Feature usage tracking and statistics

### Hardware Cryptography
- AES-NI hardware acceleration for AES encryption/decryption
- SHA-NI extensions for SHA-1 and SHA-256 acceleration
- PCLMULQDQ carry-less multiplication for GCM/GHASH
- RDRAND/RDSEED hardware random number generation
- GFNI Galois Field instructions for crypto operations
- VAES/VPCLMULQDQ vector crypto for AVX-512
- Performance monitoring and benchmarking
- Self-test validation and capability detection

### Memory Subsystem Control
- Memory controller direct access and configuration (Intel IMC/AMD UMC)
- DRAM timing control and frequency scaling
- ECC error detection, correction, and scrubbing control
- Memory bandwidth monitoring (MBM) with Intel RDT
- Cache monitoring technology (CMT) for LLC occupancy
- Hardware prefetcher control and optimization
- Memory thermal management and throttling detection
- DRAM training, calibration, and parameter optimization
- Memory interleaving and performance tuning

### Hardware Transactional Memory
- Intel TSX (Transactional Synchronization Extensions) support
- RTM (Restricted Transactional Memory) with xbegin/xend/xabort
- HLE (Hardware Lock Elision) with XACQUIRE/XRELEASE prefixes
- Transaction abort handling and retry logic with adaptive backoff
- Nested transaction support up to 7 levels deep
- Transaction conflict detection and capacity monitoring
- Performance benchmarking and statistics collection
- Hardware lock elision for spinlocks and critical sections
- Fallback mechanisms for non-TSX systems and abort scenarios

### CPU Security Features
- Intel CET (Control Flow Enforcement Technology)
- Shadow stack and indirect branch tracking
- Intel MPX (Memory Protection Extensions)
- SMEP (Supervisor Mode Execution Prevention)
- SMAP (Supervisor Mode Access Prevention)
- Protection Keys for Userspace (PKU)

### Advanced Memory Protection
- Memory protection domains and access control
- Protection key-based page protection
- NX bit (No-Execute) enforcement
- Memory access violation tracking
- Domain-based permission management

### Security Extensions
- MPX (Memory Protection Extensions)
- CET (Control-flow Enforcement Technology)
- Shadow stack and indirect branch tracking
- Bounds checking and violation handling
- NX bit and security enforcement

### CPU Debug Extensions
- Intel Processor Trace (PT) for execution tracing
- Last Branch Record (LBR) stack monitoring
- Branch Trace Store (BTS) for branch recording
- Debug interrupt handling and overflow management
- Trace session management and statistics

### Hardware Tracing
- Intel PT (Processor Trace)
- BTS (Branch Trace Store)
- Address filtering and range control
- Cycle-accurate tracing
- Power event and timing packets

### Virtualization Extensions
- VT-x (Intel Virtualization Technology)
- VMCS (Virtual Machine Control Structure)
- EPT (Extended Page Tables)
- VPID (Virtual Processor Identifiers)
- VMFUNC and nested virtualization

### Atomic Operations
- Compare-and-swap primitives
- Memory barriers and ordering
- Spinlock implementations
- Monitor/Mwait synchronization
- SMP-aware atomic arithmetic

### CPU Feature Control
- Intel/AMD model-specific features
- VMX/SMX virtualization control
- SpeedStep/Turbo boost management
- Execute Disable (XD) bit control
- Energy performance bias tuning

### Extended CPUID Features
- Comprehensive feature detection
- AVX/AVX-512/AMX instruction sets
- Security feature enumeration
- Cache topology information
- Brand string and vendor identification

### CPU Errata Handling
- Intel/AMD errata database
- Automatic workaround application
- Spectre/Meltdown mitigations
- L1TF/MDS/SRBDS protections
- Security vulnerability detection

### NUMA Memory Management
- SRAT/SLIT ACPI table parsing
- Memory affinity policies
- Distance-based allocation
- Node-specific memory pools
- Page migration support

### x2APIC Controller
- MSR-based APIC interface
- 32-bit APIC ID support
- IPI (Inter-Processor Interrupt) delivery
- Local timer with calibration
- EOI broadcast suppression

### Performance Monitoring
- Programmable performance counters
- Fixed-function counters (instructions/cycles)
- Event selection and filtering
- User/kernel mode profiling
- Global performance counter control

### IOMMU Support
- Intel VT-d implementation
- Device isolation and protection
- DMA remapping tables
- Context and root table management
- Domain-based memory translation

### System Management BIOS
- SMBIOS table parsing
- Hardware inventory information
- System manufacturer and model
- Processor specifications
- BIOS version and features

### ACPI Support
- RSDP/RSDT table parsing
- FADT and MADT enumeration
- CPU and I/O APIC detection
- Power management interface
- System shutdown and reboot

### APIC System
- Local APIC initialization and control
- I/O APIC redirection table setup
- Inter-processor interrupts (IPI)
- APIC timer with calibration
- Error status monitoring

### DMA Controller
- 8237A dual-controller support (8 channels)
- Memory-to-device and device-to-memory transfers
- Single, block, and demand transfer modes
- Channel allocation and management
- Transfer completion monitoring

### Audio System
- AC97 audio codec support
- PCM audio playback with DMA
- Volume control and mixing
- Tone generation for system sounds
- Interrupt-driven buffer management

### Network Interface
- RTL8139 Fast Ethernet controller
- 10/100 Mbps operation
- Packet transmission and reception
- MAC address configuration
- Link status monitoring

### USB Host Controller
- UHCI (USB 1.1) controller support
- Frame list and queue head management
- Control transfer implementation
- Port status monitoring and reset
- Device enumeration framework

### Real-Time Clock
- MC146818 RTC with battery backup
- Date/time reading and setting
- Alarm functionality with interrupts
- CMOS memory access (128 bytes)
- System configuration storage

### Serial Ports
- 16550 UART with FIFO buffers
- COM1/COM2/COM3/COM4 support
- Configurable baud rates up to 115200
- Hardware flow control (RTS/CTS/DTR)
- Interrupt-driven I/O with buffering

### Floppy Disk Controller
- 82077AA compatible controller
- Support for 1.44MB, 720KB, 1.2MB, 360KB
- DMA-based sector read/write
- Drive detection via CMOS
- CHS addressing with seek optimization

### VGA Text Mode
- 80x25 character display
- 16 foreground/background colors
- Hardware cursor control
- Box drawing and formatting

### PS/2 Keyboard
- Scan code set 1 translation
- Modifier key tracking (Shift, Ctrl, Alt, Caps)
- LED control (Caps, Num, Scroll)
- Interrupt-driven input buffer

### PCI Bus
- Configuration space access
- Device enumeration
- Vendor/device ID lookup
- Class code identification
- BAR (Base Address Register) parsing

### PIT Timer
- 1.193182 MHz base frequency
- Channel 0: System tick @ 1 kHz
- Channel 2: PC speaker control
- Microsecond precision delays
- Uptime tracking

### ATA/IDE
- Primary/secondary controllers
- Master/slave drives
- LBA28 addressing
- Sector read/write operations
- Drive identification (model, serial, capacity)

## API Examples

### VGA Display
```c
vga_init();
vga_set_color(VGA_WHITE, VGA_BLUE);
vga_puts("Hello World!");
vga_draw_box(10, 10, 20, 5, VGA_YELLOW);
```

### Keyboard Input
```c
ps2_kbd_init();
char c = ps2_kbd_getchar_blocking();
u8 mods = ps2_kbd_get_modifiers();
```

### PCI Enumeration
```c
pci_init();
pci_device_t* vga = pci_find_class(0x03, 0x00);
pci_enable_device(vga);
```

### Disk I/O
```c
ata_init();
u8 buffer[512];
ata_read_sectors(0, 0, 1, buffer); // Read MBR
```

### Timer Functions
```c
pit_init(1000); // 1 kHz
pit_delay(1000); // 1 second
pit_beep(440, 500); // A4 note for 500ms
```

## Demo Tasks
- **VGA Demo**: Color test and UI elements
- **Keyboard Test**: Interactive input with modifier display
- **PCI Scanner**: Lists all detected PCI devices
- **ATA Detection**: Shows connected drives and capacity
- **Timer Display**: Real-time uptime counter
- **System Info**: Live system statistics

## Performance
- **VGA**: Direct memory access, no GPU acceleration
- **Keyboard**: ~100 Hz scan rate, interrupt-driven
- **PCI**: 33 MHz bus, configuration cycles
- **Timer**: 1 MHz resolution, 32-bit counters
- **ATA**: PIO mode, ~16 MB/s transfer rate
