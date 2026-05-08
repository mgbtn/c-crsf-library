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
    
    printf("\n=== Ringbuf: %d passed, %d failed ===\n", pass, fail);
    return fail ? 1 : 0;
}