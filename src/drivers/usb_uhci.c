/*
 * usb_uhci.c – x86 USB Universal Host Controller Interface
 */

#include "kernel/types.h"

/* UHCI registers */
#define UHCI_USBCMD       0x00  /* USB Command */
#define UHCI_USBSTS       0x02  /* USB Status */
#define UHCI_USBINTR      0x04  /* USB Interrupt Enable */
#define UHCI_FRNUM        0x06  /* Frame Number */
#define UHCI_FRBASEADD    0x08  /* Frame List Base Address */
#define UHCI_SOFMOD       0x0C  /* Start of Frame Modify */
#define UHCI_PORTSC1      0x10  /* Port 1 Status/Control */
#define UHCI_PORTSC2      0x12  /* Port 2 Status/Control */

/* USB Command Register bits */
#define USBCMD_RS         0x0001  /* Run/Stop */
#define USBCMD_HCRESET    0x0002  /* Host Controller Reset */
#define USBCMD_GRESET     0x0004  /* Global Reset */
#define USBCMD_EGSM       0x0008  /* Enter Global Suspend Mode */
#define USBCMD_FGR        0x0010  /* Force Global Resume */
#define USBCMD_SWDBG      0x0020  /* Software Debug */
#define USBCMD_CF         0x0040  /* Configure Flag */
#define USBCMD_MAXP       0x0080  /* Max Packet */

/* USB Status Register bits */
#define USBSTS_USBINT     0x0001  /* USB Interrupt */
#define USBSTS_ERROR      0x0002  /* USB Error Interrupt */
#define USBSTS_RD         0x0004  /* Resume Detect */
#define USBSTS_HSE        0x0008  /* Host System Error */
#define USBSTS_HCPE       0x0010  /* Host Controller Process Error */
#define USBSTS_HCH        0x0020  /* HC Halted */

/* Port Status/Control Register bits */
#define PORTSC_CCS        0x0001  /* Current Connect Status */
#define PORTSC_CSC        0x0002  /* Connect Status Change */
#define PORTSC_PE         0x0004  /* Port Enable */
#define PORTSC_PEDC       0x0008  /* Port Enable/Disable Change */
#define PORTSC_LS         0x0030  /* Line Status */
#define PORTSC_RD         0x0040  /* Resume Detect */
#define PORTSC_LSDA       0x0100  /* Low Speed Device Attached */
#define PORTSC_PR         0x0200  /* Port Reset */
#define PORTSC_SUSP       0x1000  /* Suspend */

/* Transfer Descriptor */
typedef struct {
    u32 link_ptr;
    u32 control_status;
    u32 token;
    u32 buffer_ptr;
    u32 reserved[4];
} __attribute__((packed)) uhci_td_t;

/* Queue Head */
typedef struct {
    u32 head_link_ptr;
    u32 element_link_ptr;
    u32 reserved[6];
} __attribute__((packed)) uhci_qh_t;

/* TD Control/Status bits */
#define TD_CTRL_SPD       0x20000000  /* Short Packet Detect */
#define TD_CTRL_C_ERR     0x18000000  /* Error Counter */
#define TD_CTRL_LS        0x04000000  /* Low Speed */
#define TD_CTRL_IOS       0x02000000  /* Isochronous Select */
#define TD_CTRL_IOC       0x01000000  /* Interrupt on Complete */
#define TD_CTRL_ACTIVE    0x00800000  /* Active */
#define TD_CTRL_STALLED   0x00400000  /* Stalled */
#define TD_CTRL_DBUFERR   0x00200000  /* Data Buffer Error */
#define TD_CTRL_BABBLE    0x00100000  /* Babble Detected */
#define TD_CTRL_NAK       0x00080000  /* NAK Received */
#define TD_CTRL_CRCTOUT   0x00040000  /* CRC/Timeout Error */
#define TD_CTRL_BITSTUFF  0x00020000  /* Bitstuff Error */
#define TD_CTRL_ACTLEN    0x000007FF  /* Actual Length */

/* TD Token bits */
#define TD_TOKEN_PID      0x000000FF  /* Packet ID */
#define TD_TOKEN_DEVADDR  0x00007F00  /* Device Address */
#define TD_TOKEN_ENDPT    0x00078000  /* Endpoint */
#define TD_TOKEN_DT       0x00080000  /* Data Toggle */
#define TD_TOKEN_MAXLEN   0x7FF00000  /* Maximum Length */

/* PID values */
#define PID_SETUP         0x2D
#define PID_IN            0x69
#define PID_OUT           0xE1

typedef struct {
    u16 io_base;
    u8 irq;
    u32* frame_list;
    uhci_qh_t* control_qh;
    uhci_td_t* td_pool;
    u8 td_pool_used[32];
    u8 initialized;
} uhci_controller_t;

static uhci_controller_t uhci_controllers[4];
static u8 uhci_controller_count = 0;

static inline void outw(u16 port, u16 val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outl(u16 port, u32 val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline u32 inl(u16 port) {
    u32 val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static uhci_td_t* uhci_alloc_td(uhci_controller_t* ctrl) {
    for (u8 i = 0; i < 32; i++) {
        if (!ctrl->td_pool_used[i]) {
            ctrl->td_pool_used[i] = 1;
            uhci_td_t* td = &ctrl->td_pool[i];
            
            /* Clear TD */
            td->link_ptr = 1; /* Terminate */
            td->control_status = 0;
            td->token = 0;
            td->buffer_ptr = 0;
            
            return td;
        }
    }
    return 0;
}

static void uhci_free_td(uhci_controller_t* ctrl, uhci_td_t* td) {
    u32 index = td - ctrl->td_pool;
    if (index < 32) {
        ctrl->td_pool_used[index] = 0;
    }
}

static void uhci_reset_controller(uhci_controller_t* ctrl) {
    /* Global reset */
    outw(ctrl->io_base + UHCI_USBCMD, USBCMD_GRESET);
    
    extern void timer_delay(u32 ms);
    timer_delay(50);
    
    outw(ctrl->io_base + UHCI_USBCMD, 0);
    timer_delay(10);
    
    /* Host controller reset */
    outw(ctrl->io_base + UHCI_USBCMD, USBCMD_HCRESET);
    
    u32 timeout = 1000;
    while (timeout-- && (inw(ctrl->io_base + UHCI_USBCMD) & USBCMD_HCRESET));
    
    /* Clear status */
    outw(ctrl->io_base + UHCI_USBSTS, 0xFFFF);
}

static void uhci_setup_frame_list(uhci_controller_t* ctrl) {
    extern void* paging_alloc_pages(u32 count);
    
    /* Allocate frame list (1024 entries * 4 bytes = 4KB) */
    ctrl->frame_list = (u32*)paging_alloc_pages(1);
    if (!ctrl->frame_list) return;
    
    /* Allocate control queue head */
    ctrl->control_qh = (uhci_qh_t*)paging_alloc_pages(1);
    if (!ctrl->control_qh) return;
    
    /* Setup control QH */
    ctrl->control_qh->head_link_ptr = 1; /* Terminate */
    ctrl->control_qh->element_link_ptr = 1; /* Terminate */
    
    /* Point all frame list entries to control QH */
    u32 qh_phys = (u32)ctrl->control_qh | 2; /* QH type */
    for (u16 i = 0; i < 1024; i++) {
        ctrl->frame_list[i] = qh_phys;
    }
    
    /* Set frame list base address */
    outl(ctrl->io_base + UHCI_FRBASEADD, (u32)ctrl->frame_list);
}

u8 uhci_init(u16 io_base, u8 irq) {
    if (uhci_controller_count >= 4) return 0;
    
    uhci_controller_t* ctrl = &uhci_controllers[uhci_controller_count];
    ctrl->io_base = io_base;
    ctrl->irq = irq;
    
    /* Allocate TD pool */
    extern void* paging_alloc_pages(u32 count);
    ctrl->td_pool = (uhci_td_t*)paging_alloc_pages(1);
    if (!ctrl->td_pool) return 0;
    
    for (u8 i = 0; i < 32; i++) {
        ctrl->td_pool_used[i] = 0;
    }
    
    /* Reset controller */
    uhci_reset_controller(ctrl);
    
    /* Setup frame list */
    uhci_setup_frame_list(ctrl);
    
    /* Configure controller */
    outw(ctrl->io_base + UHCI_SOFMOD, 64); /* SOF timing */
    outw(ctrl->io_base + UHCI_USBINTR, 0x0F); /* Enable all interrupts */
    
    /* Start controller */
    outw(ctrl->io_base + UHCI_USBCMD, USBCMD_RS | USBCMD_CF);
    
    ctrl->initialized = 1;
    uhci_controller_count++;
    
    return 1;
}

void uhci_port_reset(u8 controller, u8 port) {
    if (controller >= uhci_controller_count) return;
    
    uhci_controller_t* ctrl = &uhci_controllers[controller];
    u16 port_reg = (port == 0) ? UHCI_PORTSC1 : UHCI_PORTSC2;
    
    /* Reset port */
    outw(ctrl->io_base + port_reg, PORTSC_PR);
    
    extern void timer_delay(u32 ms);
    timer_delay(50);
    
    /* Clear reset */
    outw(ctrl->io_base + port_reg, 0);
    timer_delay(10);
    
    /* Enable port */
    u16 status = inw(ctrl->io_base + port_reg);
    if (status & PORTSC_CCS) {
        outw(ctrl->io_base + port_reg, PORTSC_PE);
    }
}

u16 uhci_get_port_status(u8 controller, u8 port) {
    if (controller >= uhci_controller_count) return 0;
    
    uhci_controller_t* ctrl = &uhci_controllers[controller];
    u16 port_reg = (port == 0) ? UHCI_PORTSC1 : UHCI_PORTSC2;
    
    return inw(ctrl->io_base + port_reg);
}

u8 uhci_control_transfer(u8 controller, u8 device_addr, u8 endpoint, 
                        const void* setup_packet, void* data, u16 data_len) {
    if (controller >= uhci_controller_count) return 0;
    
    uhci_controller_t* ctrl = &uhci_controllers[controller];
    
    /* Allocate TDs */
    uhci_td_t* setup_td = uhci_alloc_td(ctrl);
    uhci_td_t* data_td = 0;
    uhci_td_t* status_td = uhci_alloc_td(ctrl);
    
    if (!setup_td || !status_td) {
        if (setup_td) uhci_free_td(ctrl, setup_td);
        if (status_td) uhci_free_td(ctrl, status_td);
        return 0;
    }
    
    if (data_len > 0) {
        data_td = uhci_alloc_td(ctrl);
        if (!data_td) {
            uhci_free_td(ctrl, setup_td);
            uhci_free_td(ctrl, status_td);
            return 0;
        }
    }
    
    /* Setup TD */
    setup_td->control_status = TD_CTRL_ACTIVE | (3 << 27); /* 3 error retries */
    setup_td->token = (device_addr << 8) | (endpoint << 15) | 
                     ((8 - 1) << 21) | PID_SETUP;
    setup_td->buffer_ptr = (u32)setup_packet;
    
    if (data_td) {
        setup_td->link_ptr = (u32)data_td;
        
        /* Data TD */
        data_td->control_status = TD_CTRL_ACTIVE | (3 << 27);
        data_td->token = (device_addr << 8) | (endpoint << 15) | 
                        TD_TOKEN_DT | ((data_len - 1) << 21) | PID_IN;
        data_td->buffer_ptr = (u32)data;
        data_td->link_ptr = (u32)status_td;
    } else {
        setup_td->link_ptr = (u32)status_td;
    }
    
    /* Status TD */
    status_td->control_status = TD_CTRL_ACTIVE | TD_CTRL_IOC | (3 << 27);
    status_td->token = (device_addr << 8) | (endpoint << 15) | 
                      TD_TOKEN_DT | ((0 - 1) << 21) | PID_OUT;
    status_td->buffer_ptr = 0;
    status_td->link_ptr = 1; /* Terminate */
    
    /* Add to control queue */
    ctrl->control_qh->element_link_ptr = (u32)setup_td;
    
    /* Wait for completion */
    u32 timeout = 1000000;
    while (timeout-- && (status_td->control_status & TD_CTRL_ACTIVE));
    
    u8 success = !(status_td->control_status & 
                  (TD_CTRL_STALLED | TD_CTRL_DBUFERR | TD_CTRL_BABBLE | 
                   TD_CTRL_NAK | TD_CTRL_CRCTOUT | TD_CTRL_BITSTUFF));
    
    /* Clean up */
    ctrl->control_qh->element_link_ptr = 1;
    uhci_free_td(ctrl, setup_td);
    if (data_td) uhci_free_td(ctrl, data_td);
    uhci_free_td(ctrl, status_td);
    
    return success;
}

void uhci_irq_handler(u8 controller) {
    if (controller >= uhci_controller_count) return;
    
    uhci_controller_t* ctrl = &uhci_controllers[controller];
    
    u16 status = inw(ctrl->io_base + UHCI_USBSTS);
    
    /* Clear interrupts */
    outw(ctrl->io_base + UHCI_USBSTS, status);
    
    if (status & USBSTS_USBINT) {
        /* USB transaction completed */
    }
    
    if (status & USBSTS_ERROR) {
        /* USB error */
    }
    
    if (status & USBSTS_RD) {
        /* Resume detected */
    }
    
    if (status & USBSTS_HSE) {
        /* Host system error */
    }
    
    if (status & USBSTS_HCPE) {
        /* Host controller process error */
    }
}

u8 uhci_get_controller_count(void) {
    return uhci_controller_count;
}
