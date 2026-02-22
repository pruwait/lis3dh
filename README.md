# ESPHome LIS3DH Component

[![ESPHome](https://img.shields.io/badge/ESPHome-compatible-blue?logo=esphome)](https://esphome.io)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Внешний компонент [ESPHome](https://esphome.io) для трёхосевого акселерометра **LIS3DH** на шине I²C.

Поддерживает:
- Три независимых сенсора (X / Y / Z) в м/с²
- 4 диапазона измерений: ±2g / ±4g / ±8g / ±16g
- 7 частот опроса: 1 — 400 Гц
- High-Resolution режим (12-бит) и Low-Power режим (8-бит)
- ESP8266 и ESP32

---

## Подключение

| LIS3DH | MCU         |
|--------|-------------|
| VCC    | 3.3 V       |
| GND    | GND         |
| SDA    | GPIO21 (ESP32) / D2 (ESP8266) |
| SCL    | GPIO22 (ESP32) / D1 (ESP8266) |
| SA0    | GND → адрес `0x18` |
| SA0    | VCC → адрес `0x19` |

> CS должен быть подтянут к VCC для работы в режиме I²C.

---

## Установка

Добавьте секцию `external_components` в ваш YAML-конфиг:

```yaml
external_components:
  - source: github://your-username/esphome-lis3dh@main
    components: [lis3dh]
```

---

## Пример конфигурации

```yaml
esphome:
  name: lis3dh-device

esp32:
  board: esp32dev

external_components:
  - source: github://your-username/esphome-lis3dh@main
    components: [lis3dh]
    refresh: 1d

logger:

i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true

sensor:
  - platform: lis3dh
    address: 0x18
    range: 2
    data_rate: 50
    update_interval: 1s

    accel_x:
      name: "Ускорение X"
    accel_y:
      name: "Ускорение Y"
    accel_z:
      name: "Ускорение Z"
```

---

## Параметры конфигурации

| Параметр          | Тип      | По умолчанию | Описание |
|-------------------|----------|:------------:|----------|
| `address`         | hex      | `0x18`       | I²C адрес (`0x18` или `0x19`) |
| `range`           | int      | `2`          | Диапазон измерений: `2` / `4` / `8` / `16` (в единицах g) |
| `data_rate`       | int      | `50`         | Частота обновления (Гц): `1` / `10` / `25` / `50` / `100` / `200` / `400` |
| `low_power`       | bool     | `false`      | Режим пониженного потребления (8-бит) |
| `update_interval` | duration | `60s`        | Интервал публикации значений |
| `accel_x`         | sensor   | —            | Ускорение по оси X, м/с² |
| `accel_y`         | sensor   | —            | Ускорение по оси Y, м/с² |
| `accel_z`         | sensor   | —            | Ускорение по оси Z, м/с² |

Каждый из сенсоров (`accel_x`, `accel_y`, `accel_z`) поддерживает все стандартные параметры ESPHome: `name`, `id`, `filters`, `on_value` и т.д.

---

## Режимы работы

### High-Resolution (по умолчанию)

```yaml
low_power: false
```

12-битное разрешение. Потребление ~10 мкА при 1 Гц. Рекомендуется для большинства применений.

### Low-Power

```yaml
low_power: true
```

8-битное разрешение. Потребление ~2 мкА при 1 Гц. Подходит для устройств на батарейном питании с невысокими требованиями к точности.

---

## Чувствительность

| Диапазон | HR (12-бит) | LP (8-бит) |
|----------|-------------|------------|
| ±2g      | 1 мг/LSB    | 16 мг/LSB  |
| ±4g      | 2 мг/LSB    | 32 мг/LSB  |
| ±8g      | 4 мг/LSB    | 64 мг/LSB  |
| ±16g     | 12 мг/LSB   | 192 мг/LSB |

Все значения публикуются в **м/с²** (1g = 9.80665 м/с²). При горизонтальном положении платы ожидаемое значение по оси Z ≈ +9.8 м/с².

---

## Структура репозитория

```
components/
└── lis3dh/
    ├── __init__.py   # маркер пакета Python
    ├── sensor.py     # схема конфигурации ESPHome
    ├── lis3dh.h      # заголовочный файл C++
    └── lis3dh.cpp    # реализация C++
example.yaml          # пример конфигурации
```

---

## Отладка

Включите уровень логирования `DEBUG` для подробного вывода:

```yaml
logger:
  level: DEBUG
```

В логах появятся строки вида:

```
[D][lis3dh:042]: Accel: X=0.0392 Y=-0.1961 Z=9.7860 m/s²
```

При старте компонент также выводит свою конфигурацию:

```
[C][lis3dh:035]: LIS3DH Accelerometer:
[C][lis3dh:036]:   Address: 0x18
[C][lis3dh:037]:   Range:      ±2g
[C][lis3dh:038]:   Data rate:  50 Hz
[C][lis3dh:039]:   Low-power:  no
```

---

## Совместимость

| Платформа | Статус |
|-----------|--------|
| ESP32     | ✅ Проверено |
| ESP8266   | ✅ Должно работать |
| RP2040    | ⚠️ Не тестировалось |

Требуется ESPHome **2023.6.0** или новее.

---

## Лицензия

MIT — см. файл [LICENSE](LICENSE).
