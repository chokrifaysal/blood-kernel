/*
 * isotp.c - ISO-TP single-frame & multi-frame
 */

#include "kernel/isotp.h"
#include "kernel/can.h"
#include "kernel/timer.h"
#include "kernel/types.h"

typedef struct {
    u8 buf[ISOTP_MAX_PAYLOAD];
    u16 idx;
    u16 total;
    u32 id;
    u8  state;   // 0=idle 1=rx 2=tx
    u32 last_ts;
} isotp_ctx_t;

static isotp_ctx_t ctx = {0};

static void send_sf(u32 id, const u8* data, u8 len) {
    can_frame_t f = {.id = id, .len = len + 1};
    f.data[0] = len;
    memcpy(&f.data[1], data, len);
    can_send(&f);
}

static void send_fc(u32 id, u8 bs, u8 st) {
    can_frame_t f = {.id = id + 1, .len = 3};
    f.data[0] = 0x30;   // flow control
    f.data[1] = bs;
    f.data[2] = st;
    can_send(&f);
}

void isotp_init(void) {
    ctx.state = 0;
}

u8 isotp_send(const isotp_msg_t* msg) {
    if (msg->len <= 7) {   // single frame
        send_sf(msg->id, msg->data, msg->len);
        return 0;
    }
    // TODO: multi-frame
    return 1;
}

u8 isotp_recv(isotp_msg_t* msg) {
    can_frame_t f;
    if (can_recv(&f) != 0) return 1;
    
    u8 type = f.data[0] >> 4;
    if (type == 0) {   // SF
        u8 len = f.data[0] & 0x0F;
        if (len > 7) return 1;
        memcpy(msg->data, &f.data[1], len);
        msg->len = len;
        msg->id  = f.id;
        return 0;
    }
    return 1;
}
