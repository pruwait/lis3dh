# ESPHome LIS3DH Component

[![ESPHome](https://img.shields.io/badge/ESPHome-compatible-blue?logo=esphome)](https://esphome.io)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Framework](https://img.shields.io/badge/framework-ESP--IDF-orange)](https://docs.espressif.com/projects/esp-idf/)

Внешний компонент [ESPHome](https://esphome.io) для трёхосевого акселерометра **LIS3DH** на шине I²C.

Поддерживает:
- Три независимых сенсора (X / Y / Z) в м/с²
- 4 диапазона измерений: ±2g / ±4g / ±8g / ±16g
- 7 частот опроса: 1 — 400 Гц
- High-Resolution режим (12-бит) и Low-Power режим (8-бит)
- Настройку прерывания INT1 для wake-up из deep sleep
- ESP32 с фреймворком ESP-IDF

---

## Подключение

| LIS3DH | ESP32 |
|--------|-------|
| VCC | 3.3 В |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |
| INT1 | GPIO33 (RTC GPIO) |
| SA0 | GND → адрес `0x18` / VCC → адрес `0x19` |
| CS | VCC ← **обязательно** для I²C режима |

---

## Установка

```yaml
external_components:
  - source: github://pruwait/lis3dh@main
    components: [lis3dh]
    refresh: 0s
```

---

## Минимальный пример

```yaml
external_components:
  - source: github://pruwait/lis3dh@main
    components: [lis3dh]

i2c:
  sda: GPIO21
  scl: GPIO22

sensor:
  - platform: lis3dh
    address: 0x18
    accel_x:
      name: "Accel X"
    accel_y:
      name: "Accel Y"
    accel_z:
      name: "Accel Z"
```

---

## Параметры конфигурации

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|:------------:|----------|
| `address` | hex | `0x18` | I²C адрес (`0x18` или `0x19`) |
| `range` | int | `2` | Диапазон: `2` / `4` / `8` / `16` (в единицах g) |
| `data_rate` | int | `50` | Частота (Гц): `1` / `10` / `25` / `50` / `100` / `200` / `400` |
| `low_power` | bool | `false` | Low-Power режим (8-бит) |
| `update_interval` | duration | `60s` | Интервал публикации |
| `accel_x` | sensor | — | Ускорение по X, м/с² |
| `accel_y` | sensor | — | Ускорение по Y, м/с² |
| `accel_z` | sensor | — | Ускорение по Z, м/с² |
| `wakeup` | блок | — | Конфигурация прерывания INT1 |

### Параметры блока `wakeup`

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|:------------:|----------|
| `threshold` | float | `0.1` | Порог в g (0.001 — 16.0) |
| `duration` | int | `1` | Число периодов ODR выше порога (0 — 127) |
| `axes` | enum | `xyz` | Оси: `x` / `y` / `z` / `xyz` |
| `latch` | bool | `true` | Удерживать флаг до явного сброса |

---

## Пример с wake-up

```yaml
sensor:
  - platform: lis3dh
    address: 0x18
    range: 2
    data_rate: 25
    update_interval: 500ms

    wakeup:
      threshold: 0.15   # 0.15g — лёгкий толчок
      duration: 2       # 2 × 40 мс = 80 мс (при 25 Гц)
      axes: xyz
      latch: true

    accel_x:
      name: "Ускорение X"
    accel_y:
      name: "Ускорение Y"
    accel_z:
      name: "Ускорение Z"

deep_sleep:
  esp32_ext0_wakeup:
    pin: GPIO33
    mode: HIGH
```

---

## Режимы работы

### High-Resolution (по умолчанию, `low_power: false`)

12-битное разрешение. Рекомендуется для большинства применений.

### Low-Power (`low_power: true`)

8-битное разрешение. Потребление ~2 мкА при 1 Гц. Для устройств на батарейном питании.

---

## Чувствительность

| Диапазон | HR (12-бит) | LP (8-бит) |
|----------|:-----------:|:----------:|
| ±2g | 1 мг/LSB | 16 мг/LSB |
| ±4g | 2 мг/LSB | 32 мг/LSB |
| ±8g | 4 мг/LSB | 64 мг/LSB |
| ±16g | 12 мг/LSB | 192 мг/LSB |

Все значения публикуются в **м/с²** (1 g = 9.80665 м/с²).

---

## Выбор порога wake-up

| Сценарий | `threshold` |
|----------|:-----------:|
| Вибрация двигателя | `0.05` |
| Лёгкий толчок | `0.10–0.15` |
| Активное движение | `0.25–0.50` |
| Удар | `1.0+` |

### Влияние `duration` на задержку реакции

| `data_rate` | Период | `duration: 2` |
|:-----------:|:------:|:-------------:|
| 10 Гц | 100 мс | 200 мс |
| 25 Гц | 40 мс | **80 мс** ← рекомендуется |
| 50 Гц | 20 мс | 40 мс |

---

## Структура файлов

```
components/lis3dh/
├── __init__.py     # маркер пакета Python
├── sensor.py       # схема конфигурации ESPHome
├── lis3dh.h        # заголовочный файл C++
└── lis3dh.cpp      # реализация C++
```

---

## Совместимость

| Платформа | Фреймворк | Статус |
|-----------|-----------|:------:|
| ESP32 | ESP-IDF | ✅ |
| ESP32 | Arduino | ✅ |
| ESP8266 | Arduino | ⚠️ Не тестировалось |

Требуется ESPHome **2023.6.0** или новее.

---

## Отладка

```yaml
logger:
  level: DEBUG
```

```
[C][lis3dh] LIS3DH Accelerometer:
[C][lis3dh]   Address: 0x18
[C][lis3dh]   Range:      ±2g
[C][lis3dh]   Data rate:  25 Hz
[C][lis3dh]   Low-power:  no
[C][lis3dh]   Wakeup interrupt: ENABLED
[C][lis3dh]     Threshold: 0.150 g
[C][lis3dh]     Duration:  2 ODR periods
[D][lis3dh]   Accel: X=0.0392 Y=-0.1961 Z=9.7860 m/s²
```

---

## Лицензия

MIT
