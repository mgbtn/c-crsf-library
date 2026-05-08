# C CRSF Library

Кроссплатформенная библиотека на **C99** для парсинга и генерации пакетов протокола **CRSF/ELRS**

---

## Особенности

- **Поддержка пакетов:** RC Channels, GPS, Battery Sensor, Link Statistics, Attitude, Flight Mode, Heartbeat
- **Битовая упаковка:** Корректная работа с 11-битными значениями каналов (little-endian bit packing)
- **Кольцевой буфер** с защитой от переполнения и автоматической ресинхронизацией потока
- **Нулевые зависимости:** Чистый C99, работает на микроконтроллерах, Linux, Windows, macOS
- **Тестирование:** Полный набор юнит-тестов

---

## Сборка (CMake)

Требуется **CMake 3.16+** и любой C99-совместимый компилятор (GCC, Clang, MSVC).

### Windows

**1. Конфигурация**
```bash
# Вариант А: MinGW / GCC
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Вариант Б: MSVC (Visual Studio 2022)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

**2. Компиляция**
```bash
cmake --build build --config Release
```

**3. Запуск тестов**
```bash
ctest --test-dir build -C Release --output-on-failure
# или вручную:
.\build\test_crc.exe
.\build\test_parse.exe
.\build\test_make.exe
```

### Linux
Вы сами справитесь =)

---

## API

```c
#include "crsf.h"

// 1. Инициализация
crsf_init();

// 2. Передача сырых байт с UART в буфер
uint8_t rx_data[32];
size_t len = uart_read(rx_data, sizeof(rx_data));
crsf_addstr(rx_data, len);

// 3. Проверка буфера на наличие пакета
crsf_pack_t *pack;
while ((pack = crsf_checkBuff()) != NULL) {
    switch (pack->type) {
        case CRSF_FRAME_RC_CHANNELS: {
            crsf_rc_channels_t rc;
            if (crsf_getControl(pack, &rc) == CRSF_OK) {
                // Используем rc.channels[0]...[15]
            }
            break;
        }
        case CRSF_FRAME_GPS: {
            crsf_gps_t gps;
            crsf_getGPS(pack, &gps);
            break;
        }
        // ... другие типы пакетов
    }
}
```

---

## Конфигурация

Все настройки находятся в `include/crsf_config.h`:

| Макрос | Описание | По умолчанию |
|---|---|---|
| `CRSF_RINGBUF_SIZE` | Размер кольцевого буфера | `256` |
| `CRSF_MAX_PACKET_SIZE` | Макс. размер пакета | `64` |

---

## Тестирование

Библиотека покрыта модульными тестами и нагрузочным тестом:
- `test_crc` — валидация алгоритма CRC8 DVB-S2
- `test_ringbuf` — проверка кольцевого буфера и ресинхронизации
- `test_make` / `test_parse` — тесты генерации и парсинга

```bash
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```


# C CRSF Library

A cross-platform C99 library for parsing and generating CRSF/ELRS protocol packets.

## Features
- **Supported Packets:** RC Channels, GPS, Battery Sensor, Link Statistics, Attitude, Flight Mode, Heartbeat
- **Bit Packing:** Correct handling of 11-bit channel values (little-endian bit packing)
- **Ring Buffer:** Overflow-protected with automatic stream resynchronization
- **Zero Dependencies:** Pure C99, compatible with microcontrollers, Linux, Windows, and macOS
- **Testing:** Comprehensive unit test suite included

## Build (CMake)
Requires CMake 3.16+ and any C99-compliant compiler (GCC, Clang, MSVC).

### Windows
1. **Configuration**
   ```bash
   # Option A: MinGW / GCC
   cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

   # Option B: MSVC (Visual Studio 2022)
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   ```
2. **Compilation**
   ```bash
   cmake --build build --config Release
   ```
3. **Run Tests**
   ```bash
   ctest --test-dir build -C Release --output-on-failure
   # or manually:
   .\build\test_crc.exe
   .\build\test_parse.exe
   .\build\test_make.exe
   ```

### Linux
Do what you must and come what may

## API
```c
#include "crsf.h"

// 1. Initialize
crsf_init();

// 2. Feed raw UART bytes into the buffer
uint8_t rx_data[32];
size_t len = uart_read(rx_data, sizeof(rx_data));
crsf_addstr(rx_data, len);

// 3. Check buffer for available packets
crsf_pack_t *pack;
while ((pack = crsf_checkBuff()) != NULL) {
    switch (pack->type) {
        case CRSF_FRAME_RC_CHANNELS: {
            crsf_rc_channels_t rc;
            if (crsf_getControl(pack, &rc) == CRSF_OK) {
                // Use rc.channels[0]...[15]
            }
            break;
        }
        case CRSF_FRAME_GPS: {
            crsf_gps_t gps;
            crsf_getGPS(pack, &gps);
            break;
        }
        // ... other packet types
    }
}
```

## Configuration
All settings are located in `include/crsf_config.h`:

| Macro | Description | Default |
|---|---|---|
| `CRSF_RINGBUF_SIZE` | Ring buffer size | 256 |
| `CRSF_MAX_PACKET_SIZE` | Maximum packet size | 64 |

## Testing
The library is covered by unit tests and stress tests:
- `test_crc` — Validates the CRC8 DVB-S2 algorithm
- `test_ringbuf` — Tests ring buffer behavior and stream resynchronization
- `test_make` / `test_parse` — Tests packet generation and parsing

```bash
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```