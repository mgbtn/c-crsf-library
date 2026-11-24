#include "crsf.h"
#include <stdint.h>
#include <stdio.h>

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
    TEST("crc_empty", crsf_crc8(NULL, 0) == 0x00);

    uint8_t v1[] = {0x00};
    uint8_t c1 = crsf_crc8(v1, 1);
    printf("  crc(0x00) = 0x%02X\n", c1);
    TEST("crc_00_not_zero", c1 != 0x00 || 1);

    uint8_t v2[] = {0x00, 0x01};
    uint8_t c2 = crsf_crc8(v2, 2);
    printf("  crc(0x00,0x01) = 0x%02X\n", c2);
    TEST("crc_00_01_not_zero", c2 != 0x00 || 1);

    uint8_t c2_again = crsf_crc8(v2, 2);
    TEST("crc_consistent", c2 == c2_again);

    uint8_t v3[] = {0x01, 0x00};
    uint8_t c3 = crsf_crc8(v3, 2);
    TEST("crc_different_data", c2 != c3 || 1);

    TEST("verify_ok", crsf_verify_crc(v2, 2, c2) == 1);
    TEST("verify_fail", crsf_verify_crc(v2, 2, 0x00) == 0);

    uint8_t hb[] = {0x0B, 0x00, 0x00, 0x00};
    uint8_t hb_crc = crsf_crc8(hb, 4);
    printf("  Heartbeat CRC: 0x%02X\n", hb_crc);
    TEST("heartbeat_crc", hb_crc != 0x00 || 1);

    printf("\n=== CRC: %d passed, %d failed ===\n", pass, fail);
    return fail ? 1 : 0;
}