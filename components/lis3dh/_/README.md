# ESPHome LIS3DH Component

Внешний компонент ESPHome для трёхосевого акселерометра **LIS3DH** (I²C).

## Структура файлов

```
components/
└── lis3dh/
    ├── __init__.py   — пустой файл-маркер пакета
    ├── sensor.py     — описание схемы конфигурации
    ├── lis3dh.h      — заголовок C++
    └── lis3dh.cpp    — реализация C++
example.yaml          — пример конфигурации
```

## Подключение (I²C)

| LIS3DH | ESP32 |
|--------|-------|
| VCC    | 3.3 V |
| GND    | GND   |
| SDA    | GPIO21|
| SCL    | GPIO22|
| SA0    | GND → адрес 0x18 |
| SA0    | VCC → адрес 0x19 |

## Параметры конфигурации

| Параметр        | Тип     | По умолчанию | Описание |
|-----------------|---------|--------------|----------|
| `address`       | hex     | `0x18`       | I²C адрес |
| `range`         | int     | `2`          | Диапазон ±g: `2 / 4 / 8 / 16` |
| `data_rate`     | int     | `50`         | ODR в Гц: `1 / 10 / 25 / 50 / 100 / 200 / 400` |
| `low_power`     | bool    | `false`      | Low-power режим (8-бит, выше энергоэффективность) |
| `update_interval`| duration| `60s`      | Интервал публикации |
| `accel_x`       | sensor  | —            | Ускорение по X (м/с²) |
| `accel_y`       | sensor  | —            | Ускорение по Y (м/с²) |
| `accel_z`       | sensor  | —            | Ускорение по Z (м/с²) |

## Режимы работы

- **High-Resolution (HR)** — `low_power: false` — 12-бит, высокая точность
- **Low-Power (LP)** — `low_power: true` — 8-бит, меньше потребление (~2 мкА @ 1 Гц)

## Использование

```yaml
external_components:
  - source:
      type: local
      path: components   # папка рядом с YAML

sensor:
  - platform: lis3dh
    address: 0x18
    range: 2
    data_rate: 50
    update_interval: 500ms
    accel_x:
      name: "Accel X"
    accel_y:
      name: "Accel Y"
    accel_z:
      name: "Accel Z"
```

## Выходные данные

Все значения публикуются в **м/с²** (умножаются на g = 9.80665 м/с²).  
Ускорение свободного падения при горизонтальном положении платы: Z ≈ +9.8 м/с².
