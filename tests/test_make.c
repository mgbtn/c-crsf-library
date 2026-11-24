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
    printf("=== CRSF Generator Tests (Round-Trip) ===\n\n");

    {
        printf("--- RC Channels Round-Trip ---\n");

        uint8_t buf[64];
        int16_t ch_in[16];

        for (int i = 0; i < 16; i++) {
            ch_in[i] = 1000 + (i * 62);
        }

        size_t len = crsf_makeControl(CRSF_ADDR_FLIGHT_CONTROLLER, ch_in, buf);
        printf("  RC len: %zu (expected 26)\n", len);
        printf("  RC ch[0] in: %d (0x%03X)\n", ch_in[0], ch_in[0] & 0x07FF);
        TEST("rtControl_gen", len == 26);

        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        printf("  RC CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[2 + buf[1] - 1]);
        TEST("rtControl_crc", crc_calc == buf[2 + buf[1] - 1]);

        crsf_pack_t pack = {.device = buf[0],
                            .length = buf[1],
                            .type = buf[2],
                            .payload = &buf[3],
                            .crc = buf[2 + buf[1] - 1],
                            .raw = buf};

        crsf_rc_channels_t rc;
        crsf_error_t err = crsf_getControl(&pack, &rc);
        printf("  parseControl err: %d, ch[0]=%d (expected %d)\n", err, rc.channels[0], ch_in[0] & 0x07FF);
        TEST("rtControl_parse", err == CRSF_OK);

        int match = 1;
        for (int i = 0; i < 16; i++) {
            if (rc.channels[i] != (ch_in[i] & 0x07FF)) {
                match = 0;
                printf("  ch[%d]: %d != %d\n", i, rc.channels[i], ch_in[i] & 0x07FF);
                break;
            }
        }
        TEST("rtControl_match", match);

        printf("\n");
    }

    {
        printf("--- GPS Round-Trip ---\n");

        uint8_t buf[64];
        crsf_gps_t gps_in = {.latitude = -351234567,
                             .longitude = 149876543,
                             .groundspeed = 2500,
                             .heading = 18000,
                             .altitude = 0,
                             .satellites = 0};

        size_t len = crsf_makeGPS(CRSF_ADDR_FLIGHT_CONTROLLER, &gps_in, buf);
        printf("  GPS len: %zu (expected 16)\n", len);
        TEST("rtGPS_gen", len == 16);

        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        printf("  GPS CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[2 + buf[1] - 1]);
        TEST("rtGPS_crc", crc_calc == buf[2 + buf[1] - 1]);

        crsf_pack_t pack = {.device = buf[0],
                            .length = buf[1],
                            .type = buf[2],
                            .payload = &buf[3],
                            .crc = buf[2 + buf[1] - 1],
                            .raw = buf};

        crsf_gps_t gps_out;
        crsf_error_t err = crsf_getGPS(&pack, &gps_out);
        printf("  parseGPS err: %d\n", err);
        TEST("rtGPS_parse", err == CRSF_OK);
        TEST("rtGPS_lat", gps_out.latitude == gps_in.latitude);
        TEST("rtGPS_lon", gps_out.longitude == gps_in.longitude);

        printf("\n");
    }

    {
        printf("--- Battery Round-Trip ---\n");

        uint8_t buf[64];
        crsf_battery_t bat_in = {.voltage = 222, .current = 150, .mah = 5000, .percentage = 0};

        size_t len = crsf_makeBattery(CRSF_ADDR_FLIGHT_CONTROLLER, &bat_in, buf);
        printf("  Battery len: %zu (expected 12)\n", len);
        TEST("rtBattery_gen", len == 12);

        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        TEST("rtBattery_crc", crc_calc == buf[2 + buf[1] - 1]);

        crsf_pack_t pack = {.device = buf[0],
                            .length = buf[1],
                            .type = buf[2],
                            .payload = &buf[3],
                            .crc = buf[2 + buf[1] - 1],
                            .raw = buf};

        crsf_battery_t bat_out;
        crsf_error_t err = crsf_getBattery(&pack, &bat_out);
        TEST("rtBattery_parse", err == CRSF_OK);
        TEST("rtBattery_volt", bat_out.voltage == bat_in.voltage);
        TEST("rtBattery_curr", bat_out.current == bat_in.current);
        TEST("rtBattery_mah", bat_out.mah == bat_in.mah);

        printf("\n");
    }

    {
        printf("--- Link Statistics Round-Trip ---\n");

        uint8_t buf[64];
        crsf_link_stats_t stats_in = {.rssi1 = -75,
                                      .rssi2 = -78,
                                      .link_quality = 85,
                                      .snr = 12,
                                      .active_antenna = 0,
                                      .rf_mode = 1,
                                      .uplink_power = 25};

        size_t len = crsf_makeLinkStats(CRSF_ADDR_FLIGHT_CONTROLLER, &stats_in, buf);
        TEST("rtLinkStats_gen", len == 14);

        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        TEST("rtLinkStats_crc", crc_calc == buf[2 + buf[1] - 1]);

        crsf_pack_t pack = {.device = buf[0],
                            .length = buf[1],
                            .type = buf[2],
                            .payload = &buf[3],
                            .crc = buf[2 + buf[1] - 1],
                            .raw = buf};

        crsf_link_stats_t stats_out;
        crsf_error_t err = crsf_getLinkStats(&pack, &stats_out);
        TEST("rtLinkStats_parse", err == CRSF_OK);
        TEST("rtLinkStats_rssi1", stats_out.rssi1 == stats_in.rssi1);
        TEST("rtLinkStats_rssi2", stats_out.rssi2 == stats_in.rssi2);
        TEST("rtLinkStats_lq", stats_out.link_quality == stats_in.link_quality);
        TEST("rtLinkStats_snr", stats_out.snr == stats_in.snr);

        printf("\n");
    }

    {
        printf("--- Attitude Round-Trip ---\n");

        uint8_t buf[64];
        crsf_attitude_t att_in = {.pitch = 1234, .roll = -5678, .yaw = 15708};

        size_t len = crsf_makeAttitude(CRSF_ADDR_FLIGHT_CONTROLLER, &att_in, buf);
        TEST("rtAttitude_gen", len == 10);

        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        TEST("rtAttitude_crc", crc_calc == buf[2 + buf[1] - 1]);

        crsf_pack_t pack = {.device = buf[0],
                            .length = buf[1],
                            .type = buf[2],
                            .payload = &buf[3],
                            .crc = buf[2 + buf[1] - 1],
                            .raw = buf};

        crsf_attitude_t att_out;
        crsf_error_t err = crsf_getAttitude(&pack, &att_out);
        TEST("rtAttitude_parse", err == CRSF_OK);
        TEST("rtAttitude_pitch", att_out.pitch == att_in.pitch);
        TEST("rtAttitude_roll", att_out.roll == att_in.roll);
        TEST("rtAttitude_yaw", att_out.yaw == att_in.yaw);

        printf("\n");
    }

    {
        printf("--- Heartbeat ---\n");

        uint8_t buf[64];
        size_t len = crsf_makeHeartbeat(CRSF_ADDR_FLIGHT_CONTROLLER, buf);
        TEST("makeHeartbeat_len", len == 4);

        uint8_t crc_calc = crsf_crc8(&buf[2], 1);
        TEST("heartbeat_crc", crc_calc == buf[3]);
        TEST("heartbeat_type", buf[2] == 0x0B);
        TEST("heartbeat_length", buf[1] == 2);

        printf("\n");
    }

    {
        printf("--- Accel Gyro Round-Trip ---\n");

        uint8_t buf[64];
        crsf_accel_gyro_t ag_in = {.sample_time = 123456789,
                                   .gyro_x = 1200,
                                   .gyro_y = -2400,
                                   .gyro_z = 32000,
                                   .acc_x = -32768,
                                   .acc_y = 12345,
                                   .acc_z = -1,
                                   .gyro_temp = 2500};

        size_t len = crsf_makeAccelGyro(CRSF_ADDR_FLIGHT_CONTROLLER, &ag_in, buf);
        printf("  AccelGyro len: %zu (expected 22)\n", len);
        TEST("rtAccelGyro_gen", len == 22);
        TEST("rtAccelGyro_len_field", buf[1] == 20);
        TEST("rtAccelGyro_type", buf[2] == 0x13);

        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        TEST("rtAccelGyro_crc", crc_calc == buf[2 + buf[1] - 1]);

        crsf_pack_t pack = {.device = buf[0],
                            .length = buf[1],
                            .type = buf[2],
                            .payload = &buf[3],
                            .crc = buf[2 + buf[1] - 1],
                            .raw = buf};

        crsf_accel_gyro_t ag_out;
        crsf_error_t err = crsf_getAccelGyro(&pack, &ag_out);
        TEST("rtAccelGyro_parse", err == CRSF_OK);
        TEST("rtAccelGyro_time", ag_out.sample_time == ag_in.sample_time);
        TEST("rtAccelGyro_gyro_x", ag_out.gyro_x == ag_in.gyro_x);
        TEST("rtAccelGyro_gyro_y", ag_out.gyro_y == ag_in.gyro_y);
        TEST("rtAccelGyro_gyro_z", ag_out.gyro_z == ag_in.gyro_z);
        TEST("rtAccelGyro_acc_x", ag_out.acc_x == ag_in.acc_x);
        TEST("rtAccelGyro_acc_y", ag_out.acc_y == ag_in.acc_y);
        TEST("rtAccelGyro_acc_z", ag_out.acc_z == ag_in.acc_z);
        TEST("rtAccelGyro_temp", ag_out.gyro_temp == ag_in.gyro_temp);

        printf("\n");
    }

    printf("=== Generator Tests: %d passed, %d failed ===\n", pass, fail);
    return fail ? 1 : 0;
}