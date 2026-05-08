/**
 * @file crsf_parse.c
 * @brief Парсеры пакетов CRSF
 */

#include <string.h>
#include "crsf.h"

/* ============================================================================
 * ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (Big-Endian чтение)
 * ============================================================================ */

static inline uint16_t be16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

static inline int16_t be16s(const uint8_t *p) {
    return (int16_t)be16(p);
}

static inline uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

static inline int32_t be32s(const uint8_t *p) {
    return (int32_t)be32(p);
}

/* ============================================================================
 * 0x16 RC CHANNELS PACKED
 * ============================================================================ */
static inline uint16_t unpack_11bit(const uint8_t *data, uint8_t channel) {
    uint16_t bit_offset = channel * 11;
    uint16_t byte_offset = bit_offset / 8;
    uint8_t bit_shift = bit_offset % 8;
    
    uint16_t value = ((uint16_t)data[byte_offset + 1] << (8 - bit_shift)) |
                     ((uint16_t)data[byte_offset] >> bit_shift);
    
    if (bit_shift > 5) {
        value |= ((uint16_t)data[byte_offset + 2] << (16 - bit_shift));
    }
    
    return value & 0x07FF;
}

crsf_error_t crsf_getControl(const crsf_pack_t *pack, crsf_rc_channels_t *out) {
    if (!pack || !out) return CRSF_ERR_INVALID_PAYLOAD;
    if (pack->type != CRSF_FRAME_RC_CHANNELS) return CRSF_ERR_INVALID_TYPE;
    
    if (pack->length != 24) {
        CRSF_LOG("PARSE RC: bad length %u != 24", pack->length);
        return CRSF_ERR_INVALID_LENGTH;
    }
    if (!pack->payload) return CRSF_ERR_INVALID_PAYLOAD;
    
    for (int i = 0; i < 16; i++) {
        out->channels[i] = (int16_t)unpack_11bit(pack->payload, (uint8_t)i);
    }
    
    return CRSF_OK;
}

/* ============================================================================
 * 0x02 GPS (12 байт payload)
 * ============================================================================ */

crsf_error_t crsf_getGPS(const crsf_pack_t *pack, crsf_gps_t *out) {
    if (!pack || !out) return CRSF_ERR_INVALID_PAYLOAD;
    if (pack->type != CRSF_FRAME_GPS) return CRSF_ERR_INVALID_TYPE;
    
    if (pack->length != 14) {
        CRSF_LOG("PARSE GPS: bad length %u != 14", pack->length);
        return CRSF_ERR_INVALID_LENGTH;
    }
    if (!pack->payload) return CRSF_ERR_INVALID_PAYLOAD;
    
    out->latitude    = be32s(&pack->payload[0]);   // [3-6]
    out->longitude   = be32s(&pack->payload[4]);   // [7-10]
    out->groundspeed = be16(&pack->payload[8]);    // [11-12]
    out->heading     = be16(&pack->payload[10]);   // [13-14]
    out->altitude    = 0;  // Пу-пу-пууу
    out->satellites  = 0;
    
    return CRSF_OK;
}

/* ============================================================================
 * 0x08 BATTERY SENSOR (8 байт payload)
 * ============================================================================ */

crsf_error_t crsf_getBattery(const crsf_pack_t *pack, crsf_battery_t *out) {
    if (!pack || !out) return CRSF_ERR_INVALID_PAYLOAD;
    if (pack->type != CRSF_FRAME_BATTERY_SENSOR) return CRSF_ERR_INVALID_TYPE;
    
    if (pack->length != 10) {
        CRSF_LOG("PARSE BATT: bad length %u != 10", pack->length);
        return CRSF_ERR_INVALID_LENGTH;
    }
    if (!pack->payload || pack->length < 9) return CRSF_ERR_INVALID_PAYLOAD;
    
    out->voltage    = be16(&pack->payload[0]);
    out->current    = be16(&pack->payload[2]);
    out->mah        = ((uint32_t)pack->payload[4] << 24) |
                      ((uint32_t)pack->payload[5] << 16) |
                      ((uint32_t)pack->payload[6] << 8)  |
                      (uint32_t)pack->payload[7];
    out->percentage = 0;
    
    return CRSF_OK;
}

/* ============================================================================
 * 0x14 LINK STATISTICS
 * ============================================================================ */

crsf_error_t crsf_getLinkStats(const crsf_pack_t *pack, crsf_link_stats_t *out) {
    if (!pack || !out) return CRSF_ERR_INVALID_PAYLOAD;
    if (pack->type != CRSF_FRAME_LINK_STATISTICS) return CRSF_ERR_INVALID_TYPE;
    
    if (pack->length != 12) {
        CRSF_LOG("PARSE LINK: bad length %u != 12", pack->length);
        return CRSF_ERR_INVALID_LENGTH;
    }
    if (!pack->payload || pack->length < 11) return CRSF_ERR_INVALID_PAYLOAD;
    
    out->rssi1          = (int8_t)pack->payload[0];
    out->rssi2          = (int8_t)pack->payload[1];
    out->link_quality   = pack->payload[2];
    out->snr            = (int8_t)pack->payload[3];
    out->active_antenna = (int8_t)pack->payload[4];
    out->rf_mode        = (int8_t)pack->payload[5];
    out->uplink_power   = pack->payload[6];
    
    return CRSF_OK;
}

/* ============================================================================
 * 0x1E ATTITUDE
 * ============================================================================ */

crsf_error_t crsf_getAttitude(const crsf_pack_t *pack, crsf_attitude_t *out) {
    if (!pack || !out) return CRSF_ERR_INVALID_PAYLOAD;
    if (pack->type != CRSF_FRAME_ATTITUDE) return CRSF_ERR_INVALID_TYPE;
    
    if (pack->length != 8) {
        CRSF_LOG("PARSE ATT: bad length %u != 8", pack->length);
        return CRSF_ERR_INVALID_LENGTH;
    }
    if (!pack->payload || pack->length < 7) return CRSF_ERR_INVALID_PAYLOAD;
    
    out->pitch = be16s(&pack->payload[0]);
    out->roll  = be16s(&pack->payload[2]);
    out->yaw   = be16s(&pack->payload[4]);
    
    return CRSF_OK;
}

/* ============================================================================
 * 0x21 FLIGHT MODE
 * ============================================================================ */

crsf_error_t crsf_getFlightMode(const crsf_pack_t *pack, crsf_flight_mode_t *out) {
    if (!pack || !out) return CRSF_ERR_INVALID_PAYLOAD;
    if (pack->type != CRSF_FRAME_FLIGHT_MODE) return CRSF_ERR_INVALID_TYPE;
    
    if (pack->length < 3) return CRSF_ERR_INVALID_LENGTH;
    if (!pack->payload) return CRSF_ERR_INVALID_PAYLOAD;
    
    size_t mode_len = pack->length - 2;
    if (mode_len > 16) mode_len = 16;
    
    memset(out->mode, 0, sizeof(out->mode));
    memcpy(out->mode, pack->payload, mode_len);
    out->mode[16] = '\0';
    
    return CRSF_OK;
}