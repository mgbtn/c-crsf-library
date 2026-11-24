#ifndef CRSF_H
#define CRSF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "crsf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* === Инициализация (устаревший однопоточный API) === */
void crsf_init(void);
void crsf_reset(void);

/* === Приём данных (устаревший однопоточный API) === */
void crsf_addstr(const uint8_t *data, size_t len);
crsf_pack_t* crsf_checkBuff(void);

/* === Контекстный API (независимые парсеры, потокобезопасно) === */
void crsf_ctx_init(crsf_ctx_t *ctx);
void crsf_ctx_reset(crsf_ctx_t *ctx);
void crsf_ctx_addstr(crsf_ctx_t *ctx, const uint8_t *data, size_t len);
crsf_pack_t* crsf_ctx_checkBuff(crsf_ctx_t *ctx);

/* === Парсинг === */
crsf_error_t crsf_getControl(const crsf_pack_t *pack, crsf_rc_channels_t *out);
crsf_error_t crsf_getGPS(const crsf_pack_t *pack, crsf_gps_t *out);
crsf_error_t crsf_getBattery(const crsf_pack_t *pack, crsf_battery_t *out);
crsf_error_t crsf_getLinkStats(const crsf_pack_t *pack, crsf_link_stats_t *out);
crsf_error_t crsf_getAttitude(const crsf_pack_t *pack, crsf_attitude_t *out);
crsf_error_t crsf_getFlightMode(const crsf_pack_t *pack, crsf_flight_mode_t *out);

/* === Генерация === */
size_t crsf_makeControl(uint8_t device, const int16_t *channels, uint8_t *out_buf);
size_t crsf_makeGPS(uint8_t device, const crsf_gps_t *gps, uint8_t *out_buf);
size_t crsf_makeBattery(uint8_t device, const crsf_battery_t *bat, uint8_t *out_buf);
size_t crsf_makeLinkStats(uint8_t device, const crsf_link_stats_t *stats, uint8_t *out_buf);
size_t crsf_makeAttitude(uint8_t device, const crsf_attitude_t *att, uint8_t *out_buf);
size_t crsf_makeHeartbeat(uint8_t device, uint8_t *out_buf);

/* === Утилиты === */
uint8_t crsf_crc8(const uint8_t *data, size_t len);

static inline bool crsf_verify_crc(const uint8_t *packet, size_t len, uint8_t expected) {
    return crsf_crc8(packet, len) == expected;
}

#ifdef __cplusplus
}
#endif
#endif /* CRSF_H */