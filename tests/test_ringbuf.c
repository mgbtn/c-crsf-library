#include "crsf.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int pass = 0, fail = 0;

#define TEST(name, cond)                                                                                               \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            printf("PASS: %s\n", name);                                                                                \
            pass++;                                                                                                    \
        } else {                                                                                                       \
            printf("FAIL: %s\n", name);                                                                                \
            fail++;                                                                                                    \
        }                                                                                                              \
    } while (0)

int main(void) {
    crsf_ctx_t ctx;
    crsf_ctx_init(&ctx);

    TEST("init_empty", crsf_ctx_checkBuff(&ctx) == NULL);

    uint8_t b = 0xC8;
    crsf_ctx_addstr(&ctx, &b, 1);
    TEST("partial_null", crsf_ctx_checkBuff(&ctx) == NULL);

    crsf_ctx_reset(&ctx);

    uint8_t hb_type_payload[] = {0x0B, 0x00, 0x00, 0x00};
    uint8_t hb_crc = crsf_crc8(hb_type_payload, 4);

    uint8_t hb[] = {0xC8, 0x05, 0x0B, 0x00, 0x00, 0x00, hb_crc};

    printf("  Heartbeat CRC: 0x%02X\n", hb_crc);
    crsf_ctx_addstr(&ctx, hb, sizeof(hb));

    crsf_pack_t *p = crsf_ctx_checkBuff(&ctx);
    TEST("parse_sync", p != NULL && p->device == 0xC8);
    TEST("parse_len", p != NULL && p->length == 5);
    TEST("parse_type", p != NULL && p->type == 0x0B);

    crsf_ctx_reset(&ctx);
    TEST("reset_empty", crsf_ctx_checkBuff(&ctx) == NULL);

    crsf_ctx_reset(&ctx);
    hb[6] ^= 0xFF;
    printf("  Corrupted CRC: 0x%02X\n", hb[6]);
    crsf_ctx_addstr(&ctx, hb, sizeof(hb));
    crsf_pack_t *p2 = crsf_ctx_checkBuff(&ctx);
    TEST("bad_crc_rejected", p2 == NULL);

    crsf_ctx_t ctx_a, ctx_b;
    crsf_ctx_init(&ctx_a);
    crsf_ctx_init(&ctx_b);

    uint8_t hb_a[] = {0xC8, 0x05, 0x0B, 0x11, 0x22, 0x33, 0x00};
    hb_a[6] = crsf_crc8(&hb_a[2], 4);
    uint8_t hb_b[] = {0xEE, 0x05, 0x0B, 0xAA, 0xBB, 0xCC, 0x00};
    hb_b[6] = crsf_crc8(&hb_b[2], 4);

    crsf_ctx_addstr(&ctx_a, hb_a, 3);
    crsf_ctx_addstr(&ctx_b, hb_b, 3);
    TEST("ctx_partial_a", crsf_ctx_checkBuff(&ctx_a) == NULL);
    TEST("ctx_partial_b", crsf_ctx_checkBuff(&ctx_b) == NULL);

    crsf_ctx_addstr(&ctx_a, &hb_a[3], sizeof(hb_a) - 3);
    crsf_ctx_addstr(&ctx_b, &hb_b[3], sizeof(hb_b) - 3);

    crsf_pack_t *pa = crsf_ctx_checkBuff(&ctx_a);
    crsf_pack_t *pb = crsf_ctx_checkBuff(&ctx_b);
    TEST("ctx_a_device", pa != NULL && pa->device == 0xC8);
    TEST("ctx_b_device", pb != NULL && pb->device == 0xEE);
    TEST("ctx_a_payload", pa != NULL && pa->payload[0] == 0x11 && pa->payload[2] == 0x33);
    TEST("ctx_b_payload", pb != NULL && pb->payload[0] == 0xAA && pb->payload[2] == 0xCC);
    TEST("ctx_independent_buffers", pa != NULL && pb != NULL && pa->raw != pb->raw && pa->payload != pb->payload);

    crsf_ctx_reset(&ctx_a);
    TEST("ctx_reset_a", crsf_ctx_checkBuff(&ctx_a) == NULL);
    crsf_ctx_addstr(&ctx_b, hb_b, sizeof(hb_b));
    crsf_pack_t *pb2 = crsf_ctx_checkBuff(&ctx_b);
    TEST("ctx_b_still_works", pb2 != NULL && pb2->device == 0xEE);

    printf("\n=== Ringbuf: %d passed, %d failed ===\n", pass, fail);
    return fail ? 1 : 0;
}
