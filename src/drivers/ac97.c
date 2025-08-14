/*
 * ac97.c – x86 AC97 audio codec controller
 */

#include "kernel/types.h"

/* AC97 mixer registers */
#define AC97_RESET              0x00
#define AC97_MASTER_VOLUME      0x02
#define AC97_HEADPHONE_VOLUME   0x04
#define AC97_MASTER_VOLUME_MONO 0x06
#define AC97_MASTER_TONE        0x08
#define AC97_PC_BEEP_VOLUME     0x0A
#define AC97_PHONE_VOLUME       0x0C
#define AC97_MIC_VOLUME         0x0E
#define AC97_LINE_IN_VOLUME     0x10
#define AC97_CD_VOLUME          0x12
#define AC97_VIDEO_VOLUME       0x14
#define AC97_AUX_VOLUME         0x16
#define AC97_PCM_OUT_VOLUME     0x18
#define AC97_RECORD_SELECT      0x1A
#define AC97_RECORD_GAIN        0x1C
#define AC97_RECORD_GAIN_MIC    0x1E
#define AC97_GENERAL_PURPOSE    0x20
#define AC97_3D_CONTROL         0x22
#define AC97_POWERDOWN          0x26
#define AC97_EXTENDED_AUDIO_ID  0x28
#define AC97_EXTENDED_AUDIO_CTRL 0x2A
#define AC97_PCM_FRONT_DAC_RATE 0x2C
#define AC97_PCM_SURR_DAC_RATE  0x2E
#define AC97_PCM_LFE_DAC_RATE   0x30
#define AC97_PCM_LR_ADC_RATE    0x32
#define AC97_PCM_MIC_ADC_RATE   0x34
#define AC97_VENDOR_ID1         0x7C
#define AC97_VENDOR_ID2         0x7E

/* Intel ICH AC97 controller registers */
#define ICH_NABMBAR_OFFSET      0x14
#define ICH_NAMBAR_OFFSET       0x10

/* Bus master registers */
#define ICH_PCM_OUT_BDBAR       0x10  /* Buffer Descriptor Base Address */
#define ICH_PCM_OUT_CIV         0x14  /* Current Index Value */
#define ICH_PCM_OUT_LVI         0x15  /* Last Valid Index */
#define ICH_PCM_OUT_SR          0x16  /* Status Register */
#define ICH_PCM_OUT_PICB        0x18  /* Position in Current Buffer */
#define ICH_PCM_OUT_PIV         0x1A  /* Prefetched Index Value */
#define ICH_PCM_OUT_CR          0x1B  /* Control Register */

#define ICH_PCM_IN_BDBAR        0x00
#define ICH_PCM_IN_CIV          0x04
#define ICH_PCM_IN_LVI          0x05
#define ICH_PCM_IN_SR           0x06
#define ICH_PCM_IN_PICB         0x08
#define ICH_PCM_IN_PIV          0x0A
#define ICH_PCM_IN_CR           0x0B

#define ICH_MIC_BDBAR           0x20
#define ICH_MIC_CIV             0x24
#define ICH_MIC_LVI             0x25
#define ICH_MIC_SR              0x26
#define ICH_MIC_PICB            0x28
#define ICH_MIC_PIV             0x2A
#define ICH_MIC_CR              0x2B

/* Control register bits */
#define ICH_CR_RPBM             0x01  /* Run/Pause Bus Master */
#define ICH_CR_RR               0x02  /* Reset Registers */
#define ICH_CR_LVBIE            0x04  /* Last Valid Buffer Interrupt Enable */
#define ICH_CR_FEIE             0x08  /* FIFO Error Interrupt Enable */
#define ICH_CR_IOCE             0x10  /* Interrupt On Completion Enable */

/* Status register bits */
#define ICH_SR_DCH              0x01  /* DMA Controller Halted */
#define ICH_SR_CELV             0x02  /* Current Equals Last Valid */
#define ICH_SR_LVBCI            0x04  /* Last Valid Buffer Completion Interrupt */
#define ICH_SR_BCIS             0x08  /* Buffer Completion Interrupt Status */
#define ICH_SR_FIFOE            0x10  /* FIFO Error */

/* Buffer descriptor */
typedef struct {
    u32 buffer_ptr;
    u16 length;
    u16 flags;
} __attribute__((packed)) ac97_buffer_desc_t;

#define BDL_IOC                 0x8000  /* Interrupt on Completion */
#define BDL_BUP                 0x4000  /* Buffer Underrun Policy */

typedef struct {
    u16 nambar;  /* Native Audio Mixer Base Address */
    u16 nabmbar; /* Native Audio Bus Master Base Address */
    u8 irq;
    ac97_buffer_desc_t* bdl_out;
    ac97_buffer_desc_t* bdl_in;
    u32* audio_buffer;
    u16 buffer_size;
    u8 initialized;
} ac97_controller_t;

static ac97_controller_t ac97_ctrl;

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

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

static void ac97_codec_write(u16 reg, u16 value) {
    u32 timeout = 1000000;
    
    /* Wait for codec ready */
    while (timeout-- && (inl(ac97_ctrl.nabmbar + 0x34) & 0x01));
    
    /* Write to codec */
    outw(ac97_ctrl.nambar + reg, value);
}

static u16 ac97_codec_read(u16 reg) {
    u32 timeout = 1000000;
    
    /* Wait for codec ready */
    while (timeout-- && (inl(ac97_ctrl.nabmbar + 0x34) & 0x01));
    
    return inw(ac97_ctrl.nambar + reg);
}

static void ac97_reset_controller(void) {
    /* Reset bus master */
    outl(ac97_ctrl.nabmbar + 0x2C, 0x02);
    
    extern void timer_delay(u32 ms);
    timer_delay(10);
    
    /* Clear reset */
    outl(ac97_ctrl.nabmbar + 0x2C, 0x00);
    timer_delay(10);
}

u8 ac97_init(u16 nambar, u16 nabmbar, u8 irq) {
    ac97_ctrl.nambar = nambar;
    ac97_ctrl.nabmbar = nabmbar;
    ac97_ctrl.irq = irq;
    
    /* Reset controller */
    ac97_reset_controller();
    
    /* Reset codec */
    ac97_codec_write(AC97_RESET, 0);
    
    extern void timer_delay(u32 ms);
    timer_delay(100);
    
    /* Check if codec is present */
    u16 vendor_id = ac97_codec_read(AC97_VENDOR_ID1);
    if (vendor_id == 0x0000 || vendor_id == 0xFFFF) {
        return 0;
    }
    
    /* Allocate buffer descriptor lists */
    extern void* paging_alloc_pages(u32 count);
    ac97_ctrl.bdl_out = (ac97_buffer_desc_t*)paging_alloc_pages(1);
    ac97_ctrl.bdl_in = (ac97_buffer_desc_t*)paging_alloc_pages(1);
    ac97_ctrl.audio_buffer = (u32*)paging_alloc_pages(4); /* 16KB buffer */
    
    if (!ac97_ctrl.bdl_out || !ac97_ctrl.bdl_in || !ac97_ctrl.audio_buffer) {
        return 0;
    }
    
    ac97_ctrl.buffer_size = 16384;
    
    /* Setup buffer descriptor list */
    for (u8 i = 0; i < 32; i++) {
        ac97_ctrl.bdl_out[i].buffer_ptr = (u32)ac97_ctrl.audio_buffer + (i * 512);
        ac97_ctrl.bdl_out[i].length = 256; /* 256 samples */
        ac97_ctrl.bdl_out[i].flags = BDL_IOC;
    }
    
    /* Set buffer descriptor base address */
    outl(ac97_ctrl.nabmbar + ICH_PCM_OUT_BDBAR, (u32)ac97_ctrl.bdl_out);
    
    /* Configure codec */
    ac97_codec_write(AC97_MASTER_VOLUME, 0x0000);      /* Max volume */
    ac97_codec_write(AC97_PCM_OUT_VOLUME, 0x0808);     /* PCM volume */
    ac97_codec_write(AC97_POWERDOWN, 0x0000);          /* Power up all */
    
    /* Set sample rate to 44.1 kHz */
    u16 ext_audio = ac97_codec_read(AC97_EXTENDED_AUDIO_ID);
    if (ext_audio & 0x01) {
        ac97_codec_write(AC97_EXTENDED_AUDIO_CTRL, 0x01);
        ac97_codec_write(AC97_PCM_FRONT_DAC_RATE, 44100);
    }
    
    ac97_ctrl.initialized = 1;
    return 1;
}

void ac97_play_buffer(const void* buffer, u32 size) {
    if (!ac97_ctrl.initialized) return;
    
    /* Copy audio data to buffer */
    const u32* src = (const u32*)buffer;
    u32 samples = size / 4;
    
    if (samples > ac97_ctrl.buffer_size / 4) {
        samples = ac97_ctrl.buffer_size / 4;
    }
    
    for (u32 i = 0; i < samples; i++) {
        ac97_ctrl.audio_buffer[i] = src[i];
    }
    
    /* Setup buffer descriptors */
    u32 buffers_needed = (samples * 4 + 511) / 512;
    if (buffers_needed > 32) buffers_needed = 32;
    
    for (u32 i = 0; i < buffers_needed; i++) {
        ac97_ctrl.bdl_out[i].length = (i == buffers_needed - 1) ? 
            ((samples * 4) % 512) / 4 : 128;
        if (ac97_ctrl.bdl_out[i].length == 0) {
            ac97_ctrl.bdl_out[i].length = 128;
        }
    }
    
    /* Set last valid index */
    outb(ac97_ctrl.nabmbar + ICH_PCM_OUT_LVI, buffers_needed - 1);
    
    /* Start playback */
    outb(ac97_ctrl.nabmbar + ICH_PCM_OUT_CR, ICH_CR_RPBM | ICH_CR_IOCE);
}

void ac97_stop_playback(void) {
    if (!ac97_ctrl.initialized) return;
    
    /* Stop bus master */
    outb(ac97_ctrl.nabmbar + ICH_PCM_OUT_CR, 0);
    
    /* Reset */
    outb(ac97_ctrl.nabmbar + ICH_PCM_OUT_CR, ICH_CR_RR);
    
    extern void timer_delay(u32 ms);
    timer_delay(1);
    
    outb(ac97_ctrl.nabmbar + ICH_PCM_OUT_CR, 0);
}

void ac97_set_volume(u8 left, u8 right) {
    if (!ac97_ctrl.initialized) return;
    
    /* Volume is inverted (0 = max, 63 = min) */
    left = 63 - (left * 63 / 100);
    right = 63 - (right * 63 / 100);
    
    u16 volume = (left << 8) | right;
    ac97_codec_write(AC97_MASTER_VOLUME, volume);
}

u8 ac97_is_playing(void) {
    if (!ac97_ctrl.initialized) return 0;
    
    u8 status = inb(ac97_ctrl.nabmbar + ICH_PCM_OUT_SR);
    return !(status & ICH_SR_DCH);
}

void ac97_irq_handler(void) {
    if (!ac97_ctrl.initialized) return;
    
    u8 status = inb(ac97_ctrl.nabmbar + ICH_PCM_OUT_SR);
    
    /* Clear interrupt status */
    outb(ac97_ctrl.nabmbar + ICH_PCM_OUT_SR, status);
    
    if (status & ICH_SR_BCIS) {
        /* Buffer completion interrupt */
    }
    
    if (status & ICH_SR_LVBCI) {
        /* Last valid buffer completion */
        ac97_stop_playback();
    }
    
    if (status & ICH_SR_FIFOE) {
        /* FIFO error */
        ac97_stop_playback();
    }
}

void ac97_generate_tone(u16 frequency, u32 duration_ms) {
    if (!ac97_ctrl.initialized) return;
    
    const u32 sample_rate = 44100;
    u32 samples = (sample_rate * duration_ms) / 1000;
    
    if (samples > ac97_ctrl.buffer_size / 4) {
        samples = ac97_ctrl.buffer_size / 4;
    }
    
    /* Generate sine wave */
    for (u32 i = 0; i < samples; i++) {
        /* Simple square wave approximation */
        u16 sample = ((i * frequency * 2) / sample_rate) % 2 ? 0x7FFF : 0x8000;
        ac97_ctrl.audio_buffer[i] = (sample << 16) | sample; /* Stereo */
    }
    
    ac97_play_buffer(ac97_ctrl.audio_buffer, samples * 4);
}

u8 ac97_is_initialized(void) {
    return ac97_ctrl.initialized;
}
