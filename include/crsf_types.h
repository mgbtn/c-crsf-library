#ifndef CRSF_TYPES_H
#define CRSF_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "crsf_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* === Типы пакетов === */
typedef enum {
    CRSF_FRAME_GPS              = 0x02,
    CRSF_FRAME_GPS_TIME         = 0x03,
    CRSF_FRAME_GPS_EXTENDED     = 0x06,
    CRSF_FRAME_VARIO            = 0x07,
    CRSF_FRAME_BATTERY_SENSOR   = 0x08,
    CRSF_FRAME_BARO_ALT         = 0x09,
    CRSF_FRAME_AIRSPEED         = 0x0A,
    CRSF_FRAME_HEARTBEAT        = 0x0B,
    CRSF_FRAME_RPM              = 0x0C,
    CRSF_FRAME_TEMP             = 0x0D,
    CRSF_FRAME_VOLTAGES         = 0x0E,
    CRSF_FRAME_VTX_TELEMETRY    = 0x10,
    CRSF_FRAME_BAROMETER        = 0x11,
    CRSF_FRAME_MAGNETOMETER     = 0x12,
    CRSF_FRAME_ACCEL_GYRO       = 0x13,
    CRSF_FRAME_LINK_STATISTICS  = 0x14,
    CRSF_FRAME_RC_CHANNELS      = 0x16,
    CRSF_FRAME_SUBSET_RC        = 0x17,
    CRSF_FRAME_LINK_STAT_RX     = 0x1C,
    CRSF_FRAME_LINK_STAT_TX     = 0x1D,
    CRSF_FRAME_ATTITUDE         = 0x1E,
    CRSF_FRAME_FLIGHT_MODE      = 0x21,
} crsf_frame_type_t;

/* === Адреса устройств === */
typedef enum {
    CRSF_ADDR_BROADCAST         = 0x00,
    CRSF_ADDR_FLIGHT_CONTROLLER = 0xC8,
    CRSF_ADDR_CRSF_TRANSMITTER  = 0xEE,
    CRSF_ADDR_CRSF_RECEIVER     = 0xEC,
} crsf_device_addr_t;

/* === Представление пакета === */
typedef struct {
    uint8_t  device;
    uint8_t  length;
    uint8_t  type;
    uint8_t *payload;
    uint8_t  crc;
    uint8_t *raw;
} crsf_pack_t;

/* === Распарсенные данные === */
typedef struct __attribute__((packed)) {
    int32_t  latitude;
    int32_t  longitude;
    uint16_t groundspeed;
    uint16_t heading;
    int16_t  altitude;
    uint8_t  satellites;
} crsf_gps_t;

typedef struct __attribute__((packed)) {
    int16_t vertical_speed;
} crsf_vario_t;

typedef struct __attribute__((packed)) {
    uint16_t voltage;
    uint16_t current;
    uint32_t mah;
    uint8_t  percentage;
} crsf_battery_t;

typedef struct __attribute__((packed)) {
    int32_t altitude;
} crsf_baro_alt_t;

typedef struct __attribute__((packed)) {
    int8_t  rssi1;
    int8_t  rssi2;
    uint8_t link_quality;
    int8_t  snr;
    int8_t  active_antenna;
    int8_t  rf_mode;
    uint8_t uplink_power;
} crsf_link_stats_t;

typedef struct __attribute__((packed)) {
    int16_t channels[16];
} crsf_rc_channels_t;

typedef struct __attribute__((packed)) {
    int16_t pitch;
    int16_t roll;
    int16_t yaw;
} crsf_attitude_t;

typedef struct __attribute__((packed)) {
    uint32_t sample_time;   // Отметка времени выборки, мкс
    int16_t  gyro_x;        // LSB = INT16_MAX/2000 DPS
    int16_t  gyro_y;        // LSB = INT16_MAX/2000 DPS
    int16_t  gyro_z;        // LSB = INT16_MAX/2000 DPS
    int16_t  acc_x;         // LSB = INT16_MAX/16 G
    int16_t  acc_y;         // LSB = INT16_MAX/16 G
    int16_t  acc_z;         // LSB = INT16_MAX/16 G
    int16_t  gyro_temp;     // Температура гироскопа, сотые доли градуса
} crsf_accel_gyro_t;

typedef struct {
    char mode[17];
} crsf_flight_mode_t;

/* === Коды ошибок === */
typedef enum {
    CRSF_OK = 0,
    CRSF_ERR_NO_DATA = -1,
    CRSF_ERR_CRC_MISMATCH = -2,
    CRSF_ERR_INVALID_LENGTH = -3,
    CRSF_ERR_INVALID_TYPE = -4,
    CRSF_ERR_BUFFER_OVERFLOW = -5,
    CRSF_ERR_INVALID_PAYLOAD = -6,
    CRSF_ERR_SYNC_NOT_FOUND = -7,
} crsf_error_t;

/* === Внутренний контекст === */
typedef struct {
    uint8_t  buffer[CRSF_RINGBUF_SIZE];
    volatile size_t head;
    volatile size_t tail;
    volatile size_t size;
    uint8_t  parse_state;
    uint8_t  packet_buf[CRSF_MAX_PACKET_SIZE];
    size_t   packet_len;
    crsf_pack_t pack;
} crsf_ctx_t;

#ifdef __cplusplus
}
#endif
#endif /* CRSF_TYPES_H */