/**
 * @file test_parse.c
 * @brief Тесты парсеров пакетов CRSF
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "crsf.h"

static int pass = 0, fail = 0;

#define TEST(name, cond) do { \
    if (cond) { printf("PASS: %s\n", name); pass++; } \
    else { printf("FAIL: %s\n", name); fail++; } \
} while(0)

int main(void) {
    crsf_init();
    
    printf("=== CRSF Parser Tests ===\n\n");
    
    /* ========================================================================
     * Тест 1: RC Channels (0x16)
     * ======================================================================== */
    {
        printf("--- RC Channels (0x16) ---\n");
        
        uint8_t buf[64];
        int16_t channels[16];
        for (int i = 0; i < 16; i++) {
            channels[i] = 992 + (i * 50);
        }
        
        size_t len = crsf_makeControl(CRSF_ADDR_FLIGHT_CONTROLLER, channels, buf);
        printf("  RC packet len: %zu (expected 26)\n", len);
        TEST("makeControl_len", len == 26);
        
        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        printf("  CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[2 + buf[1] - 1]);
        TEST("makeControl_crc", crc_calc == buf[2 + buf[1] - 1]);
        
        crsf_pack_t pack;
        pack.device = buf[0];
        pack.length = buf[1];
        pack.type = buf[2];
        pack.payload = &buf[3];
        pack.crc = buf[2 + buf[1] - 1];
        pack.raw = buf;
        
        crsf_rc_channels_t rc;
        crsf_error_t err = crsf_getControl(&pack, &rc);
        printf("  parseControl err: %d, ch[0]=%d (expected %d)\n", 
               err, rc.channels[0], channels[0] & 0x07FF);
        TEST("parseControl_ok", err == CRSF_OK);
        TEST("parseControl_ch0", rc.channels[0] == (channels[0] & 0x07FF));
        
        int all_match = 1;
        for (int i = 0; i < 16; i++) {
            if (rc.channels[i] != (channels[i] & 0x07FF)) {
                all_match = 0;
                printf("  ch[%d] mismatch: %d != %d\n", i, rc.channels[i], channels[i] & 0x07FF);
                break;
            }
        }
        TEST("parseControl_all_channels", all_match);
        
        printf("\n");
    }
    
    /* ========================================================================
     * Тест 2: GPS (0x02)
     * ======================================================================== */
    {
        printf("--- GPS (0x02) ---\n");
        
        uint8_t buf[64];
        crsf_gps_t gps_in = {
            .latitude = 350000000,
            .longitude = 1490000000,
            .groundspeed = 1500,
            .heading = 9000,
            .altitude = 0,
            .satellites = 0
        };
        
        size_t len = crsf_makeGPS(CRSF_ADDR_FLIGHT_CONTROLLER, &gps_in, buf);
        printf("  GPS packet len: %zu (expected 16)\n", len);
        TEST("makeGPS_len", len == 16);
        
        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        printf("  CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[2 + buf[1] - 1]);
        TEST("makeGPS_crc", crc_calc == buf[2 + buf[1] - 1]);
        
        crsf_pack_t pack = {
            .device = buf[0],
            .length = buf[1],
            .type = buf[2],
            .payload = &buf[3],
            .crc = buf[2 + buf[1] - 1],
            .raw = buf
        };
        
        crsf_gps_t gps_out;
        crsf_error_t err = crsf_getGPS(&pack, &gps_out);
        printf("  parseGPS err: %d\n", err);
        TEST("parseGPS_ok", err == CRSF_OK);
        TEST("parseGPS_lat", gps_out.latitude == gps_in.latitude);
        TEST("parseGPS_lon", gps_out.longitude == gps_in.longitude);
        
        printf("\n");
    }
    
    /* ========================================================================
     * Тест 3: Battery Sensor (0x08)
     * ======================================================================== */
    {
        printf("--- Battery Sensor (0x08) ---\n");
        
        uint8_t buf[64];
        crsf_battery_t bat_in = {
            .voltage = 126,
            .current = 50,
            .mah = 1500,
            .percentage = 0
        };
        
        size_t len = crsf_makeBattery(CRSF_ADDR_FLIGHT_CONTROLLER, &bat_in, buf);
        printf("  Battery packet len: %zu (expected 12)\n", len);
        TEST("makeBattery_len", len == 12);
        
        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        printf("  CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[2 + buf[1] - 1]);
        TEST("makeBattery_crc", crc_calc == buf[2 + buf[1] - 1]);
        
        crsf_pack_t pack = {
            .device = buf[0],
            .length = buf[1],
            .type = buf[2],
            .payload = &buf[3],
            .crc = buf[2 + buf[1] - 1],
            .raw = buf
        };
        
        crsf_battery_t bat_out;
        crsf_error_t err = crsf_getBattery(&pack, &bat_out);
        printf("  parseBattery err: %d\n", err);
        TEST("parseBattery_ok", err == CRSF_OK);
        TEST("parseBattery_volt", bat_out.voltage == bat_in.voltage);
        TEST("parseBattery_curr", bat_out.current == bat_in.current);
        TEST("parseBattery_mah", bat_out.mah == bat_in.mah);
        
        printf("\n");
    }
    
    /* ========================================================================
     * Тест 4: Link Statistics (0x14)
     * ======================================================================== */
    {
        printf("--- Link Statistics (0x14) ---\n");
        
        uint8_t buf[64];
        crsf_link_stats_t stats_in = {
            .rssi1 = -75,
            .rssi2 = -78,
            .link_quality = 85,
            .snr = 12,
            .active_antenna = 0,
            .rf_mode = 1,
            .uplink_power = 25
        };
        
        size_t len = crsf_makeLinkStats(CRSF_ADDR_FLIGHT_CONTROLLER, &stats_in, buf);
        printf("  LinkStats packet len: %zu (expected 14)\n", len);
        TEST("makeLinkStats_len", len == 14);
        
        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        printf("  CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[2 + buf[1] - 1]);
        TEST("makeLinkStats_crc", crc_calc == buf[2 + buf[1] - 1]);
        
        crsf_pack_t pack = {
            .device = buf[0],
            .length = buf[1],
            .type = buf[2],
            .payload = &buf[3],
            .crc = buf[2 + buf[1] - 1],
            .raw = buf
        };
        
        crsf_link_stats_t stats_out;
        crsf_error_t err = crsf_getLinkStats(&pack, &stats_out);
        printf("  parseLinkStats err: %d\n", err);
        TEST("parseLinkStats_ok", err == CRSF_OK);
        TEST("parseLinkStats_rssi", stats_out.rssi1 == stats_in.rssi1);
        TEST("parseLinkStats_lq", stats_out.link_quality == stats_in.link_quality);
        TEST("parseLinkStats_snr", stats_out.snr == stats_in.snr);
        
        printf("\n");
    }
    
    /* ========================================================================
     * Тест 5: Attitude (0x1E)
     * ======================================================================== */
    {
        printf("--- Attitude (0x1E) ---\n");
        
        uint8_t buf[64];
        crsf_attitude_t att_in = {
            .pitch = 0,
            .roll = 0,
            .yaw = 15708
        };
        
        size_t len = crsf_makeAttitude(CRSF_ADDR_FLIGHT_CONTROLLER, &att_in, buf);
        printf("  Attitude packet len: %zu (expected 10)\n", len);
        TEST("makeAttitude_len", len == 10);
        
        uint8_t crc_calc = crsf_crc8(&buf[2], buf[1] - 1);
        printf("  CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[2 + buf[1] - 1]);
        TEST("makeAttitude_crc", crc_calc == buf[2 + buf[1] - 1]);
        
        crsf_pack_t pack = {
            .device = buf[0],
            .length = buf[1],
            .type = buf[2],
            .payload = &buf[3],
            .crc = buf[2 + buf[1] - 1],
            .raw = buf
        };
        
        crsf_attitude_t att_out;
        crsf_error_t err = crsf_getAttitude(&pack, &att_out);
        printf("  parseAttitude err: %d\n", err);
        TEST("parseAttitude_ok", err == CRSF_OK);
        TEST("parseAttitude_pitch", att_out.pitch == att_in.pitch);
        TEST("parseAttitude_roll", att_out.roll == att_in.roll);
        TEST("parseAttitude_yaw", att_out.yaw == att_in.yaw);
        
        printf("\n");
    }
    
    /* ========================================================================
     * Тест 6: Accel Gyro (0x13) — разбор заранее сформированного кадра
     * ======================================================================== */
    {
        printf("--- Accel Gyro (0x13) ---\n");
        
        // C8 14 13 | 12 34 56 78 | 04D2 FB2E 7FFF | 8000 3039 FFFF | 0BB8 | D9
        uint8_t raw[] = {
            0xC8, 0x14, 0x13,
            0x12, 0x34, 0x56, 0x78,   // sample_time = 305419896
            0x04, 0xD2,               // gyro_x =  1234
            0xFB, 0x2E,               // gyro_y = -1234
            0x7F, 0xFF,               // gyro_z =  32767
            0x80, 0x00,               // acc_x  = -32768
            0x30, 0x39,               // acc_y  =  12345
            0xFF, 0xFF,               // acc_z  = -1
            0x0B, 0xB8,               // gyro_temp = 3000
            0xD9                      // CRC
        };
        
        TEST("accelGyro_frame_len", sizeof(raw) == 22);
        TEST("accelGyro_crc", crsf_crc8(&raw[2], raw[1] - 1) == raw[sizeof(raw) - 1]);
        
        crsf_pack_t pack = {
            .device = raw[0],
            .length = raw[1],
            .type = raw[2],
            .payload = &raw[3],
            .crc = raw[sizeof(raw) - 1],
            .raw = raw
        };
        
        crsf_accel_gyro_t ag;
        crsf_error_t err = crsf_getAccelGyro(&pack, &ag);
        printf("  parseAccelGyro err: %d\n", err);
        TEST("parseAccelGyro_ok", err == CRSF_OK);
        TEST("parseAccelGyro_time", ag.sample_time == 305419896u);
        TEST("parseAccelGyro_gyro_x", ag.gyro_x == 1234);
        TEST("parseAccelGyro_gyro_y", ag.gyro_y == -1234);
        TEST("parseAccelGyro_gyro_z", ag.gyro_z == 32767);
        TEST("parseAccelGyro_acc_x", ag.acc_x == -32768);
        TEST("parseAccelGyro_acc_y", ag.acc_y == 12345);
        TEST("parseAccelGyro_acc_z", ag.acc_z == -1);
        TEST("parseAccelGyro_temp", ag.gyro_temp == 3000);
        
        crsf_pack_t pack_long = pack;
        pack_long.length = 21;
        TEST("parseAccelGyro_long", crsf_getAccelGyro(&pack_long, &ag) == CRSF_OK);
        TEST("parseAccelGyro_long_time", ag.sample_time == 305419896u);
        
        crsf_pack_t pack_short = pack;
        pack_short.length = 19;
        TEST("parseAccelGyro_short",
             crsf_getAccelGyro(&pack_short, &ag) == CRSF_ERR_INVALID_LENGTH);
        
        crsf_pack_t pack_wrong = pack;
        pack_wrong.type = CRSF_FRAME_ATTITUDE;
        TEST("parseAccelGyro_wrong_type",
             crsf_getAccelGyro(&pack_wrong, &ag) == CRSF_ERR_INVALID_TYPE);
        
        TEST("parseAccelGyro_null_pack", crsf_getAccelGyro(NULL, &ag) == CRSF_ERR_INVALID_PAYLOAD);
        TEST("parseAccelGyro_null_out", crsf_getAccelGyro(&pack, NULL) == CRSF_ERR_INVALID_PAYLOAD);
        
        printf("\n");
    }
    
    /* ========================================================================
     * Тест 7: Heartbeat (0x0B)
     * ======================================================================== */
    {
        printf("--- Heartbeat (0x0B) ---\n");
        
        uint8_t buf[64];
        size_t len = crsf_makeHeartbeat(CRSF_ADDR_FLIGHT_CONTROLLER, buf);
        printf("  Heartbeat packet len: %zu (expected 4)\n", len);
        TEST("makeHeartbeat_len", len == 4);
        
        uint8_t crc_calc = crsf_crc8(&buf[2], 1);
        printf("  CRC: calc=0x%02X, pkt=0x%02X\n", crc_calc, buf[3]);
        TEST("makeHeartbeat_crc", crc_calc == buf[3]);
        
        crsf_pack_t pack = {
            .device = buf[0],
            .length = buf[1],
            .type = buf[2],
            .payload = NULL,
            .crc = buf[3],
            .raw = buf
        };
        
        TEST("heartbeat_valid", pack.length == 2 && pack.type == 0x0B);
        
        printf("\n");
    }
    
    printf("=== Parse Tests: %d passed, %d failed ===\n", pass, fail);
    return fail ? 1 : 0;
}