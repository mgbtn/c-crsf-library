#ifndef CRSF_TYPES_H
#define CRSF_TYPES_H

#include "crsf_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* === Типы пакетов === */
typedef enum {
    CRSF_FRAME_GPS = 0x02,
    CRSF_FRAME_GPS_TIME = 0x03,
    CRSF_FRAME_GPS_EXTENDED = 0x06,
    CRSF_FRAME_VARIO = 0x07,
    CRSF_FRAME_BATTERY_SENSOR = 0x08,
    CRSF_FRAME_BARO_ALT = 0x09,
    CRSF_FRAME_AIRSPEED = 0x0A,
    CRSF_FRAME_HEARTBEAT = 0x0B,
    CRSF_FRAME_RPM = 0x0C,
    CRSF_FRAME_TEMP = 0x0D,
    CRSF_FRAME_VOLTAGES = 0x0E,
    CRSF_FRAME_VTX_TELEMETRY = 0x10,
    CRSF_FRAME_BAROMETER = 0x11,
    CRSF_FRAME_MAGNETOMETER = 0x12,
    CRSF_FRAME_ACCEL_GYRO = 0x13,
    CRSF_FRAME_LINK_STATISTICS = 0x14,
    CRSF_FRAME_RC_CHANNELS = 0x16,
    CRSF_FRAME_SUBSET_RC = 0x17,
    CRSF_FRAME_LINK_STAT_RX = 0x1C,
    CRSF_FRAME_LINK_STAT_TX = 0x1D,
    CRSF_FRAME_ATTITUDE = 0x1E,
    CRSF_FRAME_FLIGHT_MODE = 0x21,
} crsf_frame_type_t;

/* === Адреса устройств === */
typedef enum {
    CRSF_ADDR_BROADCAST = 0x00,
    CRSF_ADDR_FLIGHT_CONTROLLER = 0xC8,
    CRSF_ADDR_CRSF_TRANSMITTER = 0xEE,
    CRSF_ADDR_CRSF_RECEIVER = 0xEC,
} crsf_device_addr_t;

/* === Представление пакета === */
typedef struct {
    uint8_t device;   // Адрес устройства (SYNC)
    uint8_t length;   // Длина поля TYPE + PAYLOAD + CRC
    uint8_t type;     // Тип пакета (crsf_frame_type_t)
    uint8_t *payload; // Указатель на данные пакета
    uint8_t crc;      // Принятая контрольная сумма
    uint8_t *raw;     // Указатель на полный сырой кадр
} crsf_pack_t;

/* === Распарсенные данные === */
typedef struct __attribute__((packed)) {
    int32_t latitude;     // Широта, градусы * 1e7
    int32_t longitude;    // Долгота, градусы * 1e7
    uint16_t groundspeed; // Путевая скорость, км/ч * 10
    uint16_t heading;     // Курс, градусы * 100
    int16_t altitude;     // Высота, метры (смещение +1000 м)
    uint8_t satellites;   // Количество спутников
} crsf_gps_t;

typedef struct __attribute__((packed)) {
    int16_t vertical_speed; // Вертикальная скорость, см/с
} crsf_vario_t;

typedef struct __attribute__((packed)) {
    uint16_t voltage;   // Напряжение, В * 10
    uint16_t current;   // Ток, А * 10
    uint32_t mah;       // Израсходованная ёмкость, мАч
    uint8_t percentage; // Остаток заряда, %
} crsf_battery_t;

typedef struct __attribute__((packed)) {
    int32_t altitude; // Высота по барометру, см
} crsf_baro_alt_t;

typedef struct __attribute__((packed)) {
    int8_t rssi1;          // RSSI первой антенны, дБм
    int8_t rssi2;          // RSSI второй антенны, дБм
    uint8_t link_quality;  // Качество связи, %
    int8_t snr;            // Отношение сигнал/шум, дБ
    int8_t active_antenna; // Активная антенна
    int8_t rf_mode;        // Режим радио
    uint8_t uplink_power;  // Мощность uplink, мВт
} crsf_link_stats_t;

typedef struct __attribute__((packed)) {
    int16_t channels[16]; // Значения 16 каналов (11 бит каждое)
} crsf_rc_channels_t;

typedef struct __attribute__((packed)) {
    int16_t pitch; // Тангаж, LSB = 100 мкрад (1e-4 рад), -180..+180°
    int16_t roll;  // Крен, LSB = 100 мкрад (1e-4 рад), -180..+180°
    int16_t yaw;   // Рыскание, LSB = 100 мкрад (1e-4 рад), -180..+180°
} crsf_attitude_t;

typedef struct __attribute__((packed)) {
    uint32_t sample_time; // Отметка времени выборки, мкс
    int16_t gyro_x;       // LSB = INT16_MAX/2000 DPS
    int16_t gyro_y;       // LSB = INT16_MAX/2000 DPS
    int16_t gyro_z;       // LSB = INT16_MAX/2000 DPS
    int16_t acc_x;        // LSB = INT16_MAX/16 G
    int16_t acc_y;        // LSB = INT16_MAX/16 G
    int16_t acc_z;        // LSB = INT16_MAX/16 G
    int16_t gyro_temp;    // Температура гироскопа, сотые доли градуса
} crsf_accel_gyro_t;

typedef struct {
    char mode[17]; // Название режима полёта (строка с завершающим '\0')
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
    uint8_t buffer[CRSF_RINGBUF_SIZE];        // Кольцевой буфер принятых байт
    volatile size_t head;                     // Индекс записи в кольцевой буфер
    volatile size_t tail;                     // Индекс чтения из кольцевого буфера
    volatile size_t size;                     // Количество байт в буфере
    uint8_t parse_state;                      // Текущее состояние парсера
    uint8_t packet_buf[CRSF_MAX_PACKET_SIZE]; // Буфер собираемого кадра
    size_t packet_len;                        // Длина собранной части кадра
    crsf_pack_t pack;                         // Результат разбора последнего пакета
} crsf_ctx_t;

#ifdef __cplusplus
}
#endif
#endif /* CRSF_TYPES_H */