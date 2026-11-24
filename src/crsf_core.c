#include <string.h>
#include "crsf.h"

#define STATE_SYNC      0
#define STATE_LEN       1
#define STATE_PAYLOAD   2

/* Контекст по умолчанию для устаревшего (однопоточного) API */
static crsf_ctx_t g_default_ctx;

static inline size_t ringbuf_avail_read(crsf_ctx_t *ctx) {
    return ctx->size;
}

static inline void ringbuf_push(crsf_ctx_t *ctx, uint8_t byte) {
    if (ctx->size >= CRSF_RINGBUF_SIZE) {
        ctx->tail = (ctx->tail + 1) % CRSF_RINGBUF_SIZE;
        ctx->size--;
        CRSF_ERR("RINGBUF: overflow, dropped byte");
    }
    ctx->buffer[ctx->head] = byte;
    ctx->head = (ctx->head + 1) % CRSF_RINGBUF_SIZE;
    ctx->size++;
}

static inline uint8_t ringbuf_pop(crsf_ctx_t *ctx) {
    if (ctx->size == 0) return 0;
    uint8_t b = ctx->buffer[ctx->tail];
    ctx->tail = (ctx->tail + 1) % CRSF_RINGBUF_SIZE;
    ctx->size--;
    return b;
}

static inline uint8_t ringbuf_peek(crsf_ctx_t *ctx, size_t off) {
    if (off >= ctx->size) return 0;
    return ctx->buffer[(ctx->tail + off) % CRSF_RINGBUF_SIZE];
}

void crsf_ctx_init(crsf_ctx_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parse_state = STATE_SYNC;
    CRSF_LOG("CRSF: initialized, buf=%d", CRSF_RINGBUF_SIZE);
}

void crsf_ctx_reset(crsf_ctx_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parse_state = STATE_SYNC;
    CRSF_LOG("CRSF: reset");
}

void crsf_ctx_addstr(crsf_ctx_t *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data || len == 0) return;
    for (size_t i = 0; i < len; i++) {
        ringbuf_push(ctx, data[i]);
    }
}

crsf_pack_t* crsf_ctx_checkBuff(crsf_ctx_t *ctx) {
    if (!ctx) return NULL;

    while (ringbuf_avail_read(ctx) > 0) {

        if (ctx->parse_state == STATE_SYNC) {
            uint8_t b = ringbuf_pop(ctx);
            ctx->packet_buf[0] = b;
            ctx->packet_len = 1;
            ctx->parse_state = STATE_LEN;
            CRSF_LOG("PARSE: SYNC=0x%02X", b);
            continue;
        }

        if (ctx->parse_state == STATE_LEN) {
            if (ringbuf_avail_read(ctx) < 1) break;
            uint8_t len = ringbuf_peek(ctx, 0);
            if (len < 2 || len > 62) {
                CRSF_ERR("PARSE: bad LEN=%u, resync", len);
                ringbuf_pop(ctx);
                ctx->parse_state = STATE_SYNC;
                continue;
            }
            ctx->packet_buf[1] = ringbuf_pop(ctx);
            ctx->packet_len = 2;
            ctx->parse_state = STATE_PAYLOAD;
            continue;
        }

        if (ctx->parse_state == STATE_PAYLOAD) {
            uint8_t need = ctx->packet_buf[1] + 2;
            if (ringbuf_avail_read(ctx) < (need - ctx->packet_len)) {
                break;
            }
            while (ctx->packet_len < need) {
                ctx->packet_buf[ctx->packet_len++] = ringbuf_pop(ctx);
            }

            ctx->parse_state = STATE_SYNC;

            uint8_t device = ctx->packet_buf[0];
            uint8_t length = ctx->packet_buf[1];
            uint8_t type   = ctx->packet_buf[2];
            uint8_t *payload = (length > 1) ? &ctx->packet_buf[3] : NULL;
            uint8_t crc_rx = ctx->packet_buf[1 + length];

            uint8_t crc_calc = crsf_crc8(&ctx->packet_buf[2], length - 1);
            if (crc_calc != crc_rx) {
                CRSF_ERR("PARSE: CRC fail calc=0x%02X rx=0x%02X type=0x%02X",
                         crc_calc, crc_rx, type);
                continue;
            }

            ctx->pack.device  = device;
            ctx->pack.length  = length;
            ctx->pack.type    = type;
            ctx->pack.payload = payload;
            ctx->pack.crc     = crc_rx;
            ctx->pack.raw     = ctx->packet_buf;

            CRSF_LOG("PARSE: OK dev=0x%02X type=0x%02X", device, type);
            return &ctx->pack;
        }
    }
    return NULL;
}

/* === Устаревший API на контексте по умолчанию (не потокобезопасен) === */

void crsf_init(void) {
    crsf_ctx_init(&g_default_ctx);
}

void crsf_reset(void) {
    crsf_ctx_reset(&g_default_ctx);
}

void crsf_addstr(const uint8_t *data, size_t len) {
    crsf_ctx_addstr(&g_default_ctx, data, len);
}

crsf_pack_t* crsf_checkBuff(void) {
    return crsf_ctx_checkBuff(&g_default_ctx);
}
