#ifndef CRSF_CONFIG_H
#define CRSF_CONFIG_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* === Конфигурация === */
#ifndef CRSF_RINGBUF_SIZE
#define CRSF_RINGBUF_SIZE 256
#endif

#define CRSF_MAX_PACKET_SIZE   64
#define CRSF_MAX_PAYLOAD_SIZE  60

#ifndef CRSF_DISABLE_LOGS
#define CRSF_DISABLE_LOGS 0
#endif

/* === Логирование === */
#if CRSF_DISABLE_LOGS
    #define CRSF_LOG(...)  do {} while(0)
    #define CRSF_ERR(...)  do {} while(0)
#else
    #define CRSF_LOG(fmt, ...)  printf("[LOG] " fmt "\n", ##__VA_ARGS__)
    #define CRSF_ERR(fmt, ...)  fprintf(stderr, "[ERR] " fmt "\n", ##__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif
#endif /* CRSF_CONFIG_H */