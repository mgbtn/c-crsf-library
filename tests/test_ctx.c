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

#define MAX_PKT 64

static size_t make_rc(uint8_t device, int base, int16_t *channels, uint8_t *out) {
    for (int i = 0; i < 16; i++) {
        channels[i] = base + i;
    }
    return crsf_makeControl(device, channels, out);
}

static size_t make_gps(uint8_t device, int32_t lat, uint8_t *out) {
    crsf_gps_t gps;
    memset(&gps, 0, sizeof(gps));
    gps.latitude = lat;
    gps.longitude = -lat;
    gps.groundspeed = 100;
    gps.heading = 900;
    return crsf_makeGPS(device, &gps, out);
}

static size_t make_batt(uint8_t device, uint16_t mv, uint8_t *out) {
    crsf_battery_t bat;
    memset(&bat, 0, sizeof(bat));
    bat.voltage = mv;
    bat.current = 500;
    bat.mah = 1234;
    return crsf_makeBattery(device, &bat, out);
}

int main(void) {
    printf("=== CRSF Multi-Context Tests ===\n\n");

    uint8_t rc[MAX_PKT], gps[MAX_PKT], batt[MAX_PKT];
    int16_t channels[16];
    size_t rc_len = make_rc(CRSF_ADDR_FLIGHT_CONTROLLER, 992, channels, rc);
    size_t gps_len = make_gps(CRSF_ADDR_CRSF_TRANSMITTER, 12345678, gps);
    size_t batt_len = make_batt(CRSF_ADDR_CRSF_RECEIVER, 12000, batt);

    {
        printf("--- NULL ctx ---\n");
        uint8_t dummy = 0;
        crsf_ctx_init(NULL);
        crsf_ctx_reset(NULL);
        crsf_ctx_addstr(NULL, &dummy, 1);
        TEST("null_checkBuff", crsf_ctx_checkBuff(NULL) == NULL);
        TEST("null_addstr_data", (crsf_ctx_addstr(NULL, NULL, 0), 1));
    }

    {
        printf("--- Two contexts ---\n");
        crsf_ctx_t a, b;
        crsf_ctx_init(&a);
        crsf_ctx_init(&b);

        crsf_ctx_addstr(&a, rc, rc_len);
        crsf_ctx_addstr(&b, gps, gps_len);

        crsf_pack_t *pa = crsf_ctx_checkBuff(&a);
        crsf_pack_t *pb = crsf_ctx_checkBuff(&b);

        TEST("a_rc_packet",
             pa != NULL && pa->type == CRSF_FRAME_RC_CHANNELS && pa->device == CRSF_ADDR_FLIGHT_CONTROLLER);
        TEST("b_gps_packet", pb != NULL && pb->type == CRSF_FRAME_GPS && pb->device == CRSF_ADDR_CRSF_TRANSMITTER);
        TEST("separate_results",
             pa != NULL && pb != NULL && pa != pb && pa->raw != pb->raw && pa->payload != pb->payload);
        TEST("a_buffer_empty", crsf_ctx_checkBuff(&a) == NULL);
        TEST("b_buffer_empty", crsf_ctx_checkBuff(&b) == NULL);

        crsf_rc_channels_t rc_out;
        crsf_error_t err_a = crsf_getControl(pa, &rc_out);
        TEST("a_payload_intact", err_a == CRSF_OK && rc_out.channels[0] == (channels[0] & 0x07FF) &&
                                     rc_out.channels[15] == (channels[15] & 0x07FF));

        crsf_gps_t gps_out;
        crsf_error_t err_b = crsf_getGPS(pb, &gps_out);
        TEST("b_payload_intact", err_b == CRSF_OK && gps_out.latitude == 12345678);
    }

    {
        printf("--- Interleaved byte feeding ---\n");
        crsf_ctx_t a, b;
        crsf_ctx_init(&a);
        crsf_ctx_init(&b);

        size_t max_len = (rc_len > batt_len) ? rc_len : batt_len;
        int got_a = 0, got_b = 0, early = 0;
        crsf_pack_t *ra = NULL, *rb = NULL;

        for (size_t i = 0; i < max_len; i++) {
            if (i < rc_len) {
                crsf_ctx_addstr(&a, &rc[i], 1);
                crsf_pack_t *p = crsf_ctx_checkBuff(&a);
                if (p) {
                    ra = p;
                    got_a++;
                }
                if (p && i + 1 < rc_len)
                    early = 1;
            }
            if (i < batt_len) {
                crsf_ctx_addstr(&b, &batt[i], 1);
                crsf_pack_t *p = crsf_ctx_checkBuff(&b);
                if (p) {
                    rb = p;
                    got_b++;
                }
                if (p && i + 1 < batt_len)
                    early = 1;
            }
        }

        TEST("interleaved_no_partial_packet", !early);
        TEST("interleaved_a", got_a == 1 && ra != NULL && ra->type == CRSF_FRAME_RC_CHANNELS);
        TEST("interleaved_b", got_b == 1 && rb != NULL && rb->type == CRSF_FRAME_BATTERY_SENSOR);
        TEST("interleaved_a_device", ra != NULL && ra->device == CRSF_ADDR_FLIGHT_CONTROLLER);
        TEST("interleaved_b_device", rb != NULL && rb->device == CRSF_ADDR_CRSF_RECEIVER);

        crsf_battery_t bat_out;
        TEST("interleaved_b_payload",
             rb != NULL && crsf_getBattery(rb, &bat_out) == CRSF_OK && bat_out.voltage == 12000 && bat_out.mah == 1234);
    }

    {
        printf("--- Many contexts ---\n");
        enum { N = 4 };
        crsf_ctx_t ctx[N];
        uint8_t hb[N][MAX_PKT];
        size_t hb_len[N];
        const uint8_t devices[N] = {CRSF_ADDR_FLIGHT_CONTROLLER, CRSF_ADDR_CRSF_TRANSMITTER, CRSF_ADDR_CRSF_RECEIVER,
                                    CRSF_ADDR_BROADCAST};

        for (int i = 0; i < N; i++) {
            crsf_ctx_init(&ctx[i]);
            hb_len[i] = crsf_makeHeartbeat(devices[i], hb[i]);
            crsf_ctx_addstr(&ctx[i], hb[i], hb_len[i]);
        }

        int ok = 1;
        for (int i = 0; i < N; i++) {
            crsf_pack_t *p = crsf_ctx_checkBuff(&ctx[i]);
            if (!p || p->type != CRSF_FRAME_HEARTBEAT || p->device != devices[i]) {
                ok = 0;
            }
        }
        TEST("many_contexts_devices", ok);

        int empty = 1;
        for (int i = 0; i < N; i++) {
            if (crsf_ctx_checkBuff(&ctx[i]) != NULL)
                empty = 0;
        }
        TEST("many_contexts_drained", empty);
    }

    {
        printf("--- CRC failure isolation ---\n");
        crsf_ctx_t a, b;
        crsf_ctx_init(&a);
        crsf_ctx_init(&b);

        uint8_t bad[MAX_PKT];
        memcpy(bad, rc, rc_len);
        bad[rc_len - 1] ^= 0xFF;

        crsf_ctx_addstr(&a, bad, rc_len);
        crsf_ctx_addstr(&b, rc, rc_len);

        TEST("bad_crc_a_rejected", crsf_ctx_checkBuff(&a) == NULL);
        crsf_pack_t *pb = crsf_ctx_checkBuff(&b);
        TEST("good_packet_b_ok", pb != NULL && pb->type == CRSF_FRAME_RC_CHANNELS);
    }

    {
        printf("--- Reset isolation ---\n");
        crsf_ctx_t a, b;
        crsf_ctx_init(&a);
        crsf_ctx_init(&b);

        crsf_ctx_addstr(&a, rc, 5);
        crsf_ctx_addstr(&b, batt, 5);

        crsf_ctx_reset(&a);
        crsf_ctx_addstr(&b, &batt[5], batt_len - 5);

        TEST("a_cleared_by_reset", crsf_ctx_checkBuff(&a) == NULL);
        crsf_pack_t *pb = crsf_ctx_checkBuff(&b);
        TEST("b_survives_a_reset",
             pb != NULL && pb->type == CRSF_FRAME_BATTERY_SENSOR && pb->device == CRSF_ADDR_CRSF_RECEIVER);
    }

    {
        printf("--- Overflow isolation ---\n");
        crsf_ctx_t a, b;
        crsf_ctx_init(&a);
        crsf_ctx_init(&b);

        uint8_t garbage[CRSF_RINGBUF_SIZE + 8];
        memset(garbage, 0xFF, sizeof(garbage));
        crsf_ctx_addstr(&a, garbage, sizeof(garbage));

        crsf_ctx_addstr(&b, batt, batt_len);
        crsf_pack_t *pb = crsf_ctx_checkBuff(&b);
        TEST("b_ok_during_a_overflow", pb != NULL && pb->type == CRSF_FRAME_BATTERY_SENSOR);

        crsf_ctx_reset(&a);
        crsf_ctx_addstr(&a, rc, rc_len);
        crsf_pack_t *pa = crsf_ctx_checkBuff(&a);
        TEST("a_recovers_after_reset", pa != NULL && pa->type == CRSF_FRAME_RC_CHANNELS);
    }

    printf("\n=== Multi-Context: %d passed, %d failed ===\n", pass, fail);
    return fail ? 1 : 0;
}
