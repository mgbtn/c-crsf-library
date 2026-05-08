/**
 * @file crsf_make.c
 * @brief Генераторы пакетов CRSF
 */

#include <string.h>
#include "crsf.h"

/* ============================================================================
 * ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (Big-Endian запись)
 * ============================================================================ */

static inline void be16_write(uint8_t *p, uint16_t v) {
    p[0] = (v >> 8) & 0xFF;
    p[1] = v & 0xFF;
}

static inline void be32_write(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF;
    p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF;
    p[3] = v & 0xFF;
}

/* ============================================================================
 * 0x16 RC CHANNELS PACKED (22 байта payload)
 * ============================================================================ */

// Упаковка 11-бит значения в бит-поток (little-endian бит-пакинг)
static inline void pack_11bit(uint8_t *data, uint8_t channel, uint16_t value) {
    uint16_t bit_offset = channel * 11;
    uint16_t byte_offset = bit_offset / 8;
    uint8_t bit_shift = bit_offset % 8;
    
    value &= 0x07FF;  // Маска 11 бит
    
    data[byte_offset] |= (value << bit_shift) & 0xFF;
    data[byte_offset + 1] |= (value >> (8 - bit_shift)) & 0xFF;
    
    if (bit_shift > 5) {
        data[byte_offset + 2] |= (value >> (16 - bit_shift)) & 0xFF;
    }
}

size_t crsf_makeControl(uint8_t device, const int16_t *channels, uint8_t *out_buf) {
    if (!channels || !out_buf) return 0;
    
    memset(out_buf, 0, 64);
    
    uint8_t len = 24;  // не магия - TYPE(1) + PAY(22) + CRC(1) = 24
    
    out_buf[0] = device;
    out_buf[1] = len;
    out_buf[2] = CRSF_FRAME_RC_CHANNELS;
    
    uint8_t *payload = &out_buf[3];
    for (int i = 0; i < 16; i++) {
        uint16_t raw = (uint16_t)(channels[i] & 0x07FF);
        pack_11bit(payload, (uint8_t)i, raw);
    }
    
    out_buf[2 + len - 1] = crsf_crc8(&out_buf[2], len - 1);
    
    return 2 + len;
}

/* ============================================================================
 * 0x02 GPS (12 байт payload)
 * ============================================================================ */

size_t crsf_makeGPS(uint8_t device, const crsf_gps_t *gps, uint8_t *out_buf) {
    if (!gps || !out_buf) return 0;
    uint8_t len = 14;
    
    out_buf[0] = device;           // [0] SYNC
    out_buf[1] = len;              // [1] LEN
    out_buf[2] = CRSF_FRAME_GPS;   // [2] TYPE
    
    be32_write(&out_buf[3], (uint32_t)gps->latitude);    // [3-6]
    be32_write(&out_buf[7], (uint32_t)gps->longitude);   // [7-10]
    be16_write(&out_buf[11], gps->groundspeed);          // [11-12]
    be16_write(&out_buf[13], gps->heading);              // [13-14]
    
    out_buf[15] = crsf_crc8(&out_buf[2], len - 1);
    
    return 2 + len;
}

/* ============================================================================
 * 0x08 BATTERY SENSOR (8 байт payload)
 * ============================================================================ */

size_t crsf_makeBattery(uint8_t device, const crsf_battery_t *bat, uint8_t *out_buf) {
    if (!bat || !out_buf) return 0;
    
    uint8_t len = 10;
    
    out_buf[0] = device;
    out_buf[1] = len;
    out_buf[2] = CRSF_FRAME_BATTERY_SENSOR;
    
    be16_write(&out_buf[3], bat->voltage);
    be16_write(&out_buf[5], bat->current);
    out_buf[7] = (bat->mah >> 24) & 0xFF;
    out_buf[8] = (bat->mah >> 16) & 0xFF;
    out_buf[9] = (bat->mah >> 8) & 0xFF;
    out_buf[10] = bat->mah & 0xFF;
    
    out_buf[2 + len - 1] = crsf_crc8(&out_buf[2], len - 1);
    
    return 2 + len;
}

/* ============================================================================
 * 0x14 LINK STATISTICS (10 байт payload)
 * ============================================================================ */

size_t crsf_makeLinkStats(uint8_t device, const crsf_link_stats_t *stats, uint8_t *out_buf) {
    if (!stats || !out_buf) return 0;
    
    uint8_t len = 12;
    
    out_buf[0] = device;
    out_buf[1] = len;
    out_buf[2] = CRSF_FRAME_LINK_STATISTICS;
    
    out_buf[3]  = (uint8_t)stats->rssi1;
    out_buf[4]  = (uint8_t)stats->rssi2;
    out_buf[5]  = stats->link_quality;
    out_buf[6]  = (uint8_t)stats->snr;
    out_buf[7]  = (uint8_t)stats->active_antenna;
    out_buf[8]  = (uint8_t)stats->rf_mode;
    out_buf[9]  = stats->uplink_power;
    out_buf[10] = 0;
    out_buf[11] = 0;
    out_buf[12] = 0;
    
    out_buf[2 + len - 1] = crsf_crc8(&out_buf[2], len - 1);
    
    return 2 + len;
}

/* ============================================================================
 * 0x1E ATTITUDE (6 байт payload)
 * ============================================================================ */

size_t crsf_makeAttitude(uint8_t device, const crsf_attitude_t *att, uint8_t *out_buf) {
    if (!att || !out_buf) return 0;
    
    uint8_t len = 8;
    
    out_buf[0] = device;
    out_buf[1] = len;
    out_buf[2] = CRSF_FRAME_ATTITUDE;
    
    be16_write(&out_buf[3], (uint16_t)att->pitch);
    be16_write(&out_buf[5], (uint16_t)att->roll);
    be16_write(&out_buf[7], (uint16_t)att->yaw);
    
    out_buf[2 + len - 1] = crsf_crc8(&out_buf[2], len - 1);
    
    return 2 + len;
}

/* ============================================================================
 * 0x0B HEARTBEAT (0 байт payload)
 * ============================================================================ */

size_t crsf_makeHeartbeat(uint8_t device, uint8_t *out_buf) {
    if (!out_buf) return 0;
    
    uint8_t len = 2;
    
    out_buf[0] = device;
    out_buf[1] = len;
    out_buf[2] = CRSF_FRAME_HEARTBEAT;
    out_buf[2 + len - 1] = crsf_crc8(&out_buf[2], len - 1);
    
    return 2 + len;
}