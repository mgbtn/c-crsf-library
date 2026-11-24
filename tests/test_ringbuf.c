#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "crsf.h"

static int pass = 0, fail = 0;

#define TEST(name, cond) do { \
    if (cond) { \
        printf("PASS: %s\n", name); \
        pass++; \
    } else { \
        printf("FAIL: %s\n", name); \
        fail++; \
    } \
} while(0)

int main(void) {
    crsf_init();
    
    // Тест 1: пустой буфер
    TEST("init_empty", crsf_checkBuff() == NULL);
    
    // Тест 2: один тестовый байт - не пакет
    uint8_t b = 0xC8;
    crsf_addstr(&b, 1);
    TEST("partial_null", crsf_checkBuff() == NULL);
    
    // Тест 3: Heartbeat с ПРАВИЛЬНЫМ CRC
    crsf_reset();
    
    // Формируем пакет:
    // SYNC=0xC8, LEN=5, TYPE=0x0B, PAYLOAD=3 байта, CRC
    uint8_t hb_type_payload[] = {0x0B, 0x00, 0x00, 0x00};
    uint8_t hb_crc = crsf_crc8(hb_type_payload, 4);
    
    uint8_t hb[] = {
        0xC8,                    // [0] SYNC
        0x05,                    // [1] LEN (TYPE+PAY+CRC = 1+3+1 = 5)
        0x0B,                    // [2] TYPE (Heartbeat)
        0x00, 0x00, 0x00,        // [3-5] PAYLOAD (3 bytes)
        hb_crc                   // [6] CRC
    };
    
    printf("  Heartbeat CRC: 0x%02X\n", hb_crc);
    crsf_addstr(hb, sizeof(hb));
    
    crsf_pack_t *p = crsf_checkBuff();
    TEST("parse_sync", p != NULL && p->device == 0xC8);
    TEST("parse_len", p != NULL && p->length == 5);
    TEST("parse_type", p != NULL && p->type == 0x0B);
    
    // Тест 4: reset очищает буфер
    crsf_reset();
    TEST("reset_empty", crsf_checkBuff() == NULL);
    
    // Тест 5: пакет с НЕПРАВИЛЬНЫМ CRC отбрасывается
    crsf_reset();
    hb[6] ^= 0xFF;  // ломаем CRC на индексе 6
    printf("  Corrupted CRC: 0x%02X\n", hb[6]);
    crsf_addstr(hb, sizeof(hb));
    crsf_pack_t *p2 = crsf_checkBuff();
    TEST("bad_crc_rejected", p2 == NULL);

    // Тест 6: два независимых контекста, чередующийся разбор
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
    TEST("ctx_independent_buffers", pa != NULL && pb != NULL && pa->raw != pb->raw &&
                                   pa->payload != pb->payload);

    // Сброс одного контекста не влияет на другой
    crsf_ctx_reset(&ctx_a);
    TEST("ctx_reset_a", crsf_ctx_checkBuff(&ctx_a) == NULL);
    crsf_ctx_addstr(&ctx_b, hb_b, sizeof(hb_b));
    crsf_pack_t *pb2 = crsf_ctx_checkBuff(&ctx_b);
    TEST("ctx_b_still_works", pb2 != NULL && pb2->device == 0xEE);

    printf("\n=== Ringbuf: %d passed, %d failed ===\n", pass, fail);
    return fail ? 1 : 0;
}