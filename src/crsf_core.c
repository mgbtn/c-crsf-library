#include <string.h>
#include "crsf.h"

static crsf_ctx_t g_ctx;
static crsf_pack_t g_pack;

#define STATE_SYNC      0
#define STATE_LEN       1
#define STATE_PAYLOAD   2

static inline size_t ringbuf_avail_write(void) {
    return CRSF_RINGBUF_SIZE - g_ctx.size;
}

static inline size_t ringbuf_avail_read(void) {
    return g_ctx.size;
}

static inline void ringbuf_push(uint8_t byte) {
    if (g_ctx.size >= CRSF_RINGBUF_SIZE) {
        g_ctx.tail = (g_ctx.tail + 1) % CRSF_RINGBUF_SIZE;
        g_ctx.size--;
        CRSF_ERR("RINGBUF: overflow, dropped byte");
    }
    g_ctx.buffer[g_ctx.head] = byte;
    g_ctx.head = (g_ctx.head + 1) % CRSF_RINGBUF_SIZE;
    g_ctx.size++;
}

static inline uint8_t ringbuf_pop(void) {
    if (g_ctx.size == 0) return 0;
    uint8_t b = g_ctx.buffer[g_ctx.tail];
    g_ctx.tail = (g_ctx.tail + 1) % CRSF_RINGBUF_SIZE;
    g_ctx.size--;
    return b;
}

static inline uint8_t ringbuf_peek(size_t off) {
    if (off >= g_ctx.size) return 0;
    return g_ctx.buffer[(g_ctx.tail + off) % CRSF_RINGBUF_SIZE];
}

void crsf_init(void) {
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.parse_state = STATE_SYNC;
    CRSF_LOG("CRSF: initialized, buf=%d", CRSF_RINGBUF_SIZE);
}

void crsf_reset(void) {
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.parse_state = STATE_SYNC;
    CRSF_LOG("CRSF: reset");
}

void crsf_addstr(const uint8_t *data, size_t len) {
    if (!data || len == 0) return;
    for (size_t i = 0; i < len; i++) {
        ringbuf_push(data[i]);
    }
}

crsf_pack_t* crsf_checkBuff(void) {
   
    while (ringbuf_avail_read() > 0) {
        
        if (g_ctx.parse_state == STATE_SYNC) {
            uint8_t b = ringbuf_peek(0);
            if (b == 0xC8 || b == 0xEE || b == 0x00) {
                g_ctx.packet_buf[0] = ringbuf_pop();
                g_ctx.packet_len = 1;
                g_ctx.parse_state = STATE_LEN;
                CRSF_LOG("PARSE: SYNC=0x%02X", b);
            } else {
                ringbuf_pop();
            }
            continue;
        }
        
        if (g_ctx.parse_state == STATE_LEN) {
            if (ringbuf_avail_read() < 1) break;
            uint8_t len = ringbuf_peek(0);
            if (len < 2 || len > 62) {
                CRSF_ERR("PARSE: bad LEN=%u, resync", len);
                ringbuf_pop();
                g_ctx.parse_state = STATE_SYNC;
                continue;
            }
            g_ctx.packet_buf[1] = ringbuf_pop();
            g_ctx.packet_len = 2;
            g_ctx.parse_state = STATE_PAYLOAD;
            continue;
        }
        
        if (g_ctx.parse_state == STATE_PAYLOAD) {
            uint8_t need = g_ctx.packet_buf[1] + 2;
            if (ringbuf_avail_read() < (need - g_ctx.packet_len)) {
                break;
            }
            while (g_ctx.packet_len < need) {
                g_ctx.packet_buf[g_ctx.packet_len++] = ringbuf_pop();
            }
            
            g_ctx.parse_state = STATE_SYNC;
            
            uint8_t device = g_ctx.packet_buf[0];
            uint8_t length = g_ctx.packet_buf[1];
            uint8_t type   = g_ctx.packet_buf[2];
            uint8_t *payload = (length > 1) ? &g_ctx.packet_buf[3] : NULL;
            uint8_t crc_rx = g_ctx.packet_buf[1 + length];
            
            uint8_t crc_calc = crsf_crc8(&g_ctx.packet_buf[2], length - 1);
            if (crc_calc != crc_rx) {
                CRSF_ERR("PARSE: CRC fail calc=0x%02X rx=0x%02X type=0x%02X",
                         crc_calc, crc_rx, type);
                continue;
            }
            
            g_pack.device  = device;
            g_pack.length  = length;
            g_pack.type    = type;
            g_pack.payload = payload;
            g_pack.crc     = crc_rx;
            g_pack.raw     = g_ctx.packet_buf;
            
            CRSF_LOG("PARSE: OK dev=0x%02X type=0x%02X", device, type);
            return &g_pack;
        }
    }
    return NULL;
}