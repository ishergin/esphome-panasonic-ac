# Modbus RS485 Gateway для Panasonic AC

Этот компонент добавляет Modbus RTU шлюз к существующей интеграции Panasonic AC для ESPHome. Теперь вы можете управлять кондиционером через Modbus по RS485.

## Подключение оборудования

### Схема подключения

```
ESP32-C3                    MAX485/MAX3485                RS485 Bus
┌─────────┐                ┌─────────────┐               ┌─────────┐
│         │                │             │               │         │
│  GPIO5  ├────────TX──────►│DI          │               │  Modbus │
│         │                │             │    A ├────────►│ Master  │
│  GPIO6  ├────────RX──────►│RO          │    │          │  (ПЛК/SCADA)│
│         │                │             │    │          │         │
│  GPIO7  ├────────DE/RE───►│DE/RE       │    B ├────────►│         │
│         │                │             │               │         │
│  3.3V   ├────────VCC─────►│VCC          │               │         │
│         │                │             │               │         │
│  GND    ├────────GND─────►│GND          │               │         │
│         │                │             │               │         │
└─────────┘                └─────────────┘               └─────────┘
```

### Пины (настраиваются в YAML)

| Функция | Пин по умолчанию | Описание |
|---------|------------------|----------|
| TX      | GPIO5            | Передача данных на MAX485 |
| RX      | GPIO6            | Приём данных от MAX485 |
| DE/RE   | GPIO7            | Управление направлением (опционально) |

## Регистры Modbus

### Coils (чтение/запись) - Function 01/05

| Адрес | Имя | Значение | Описание |
|-------|-----|----------|----------|
| 0x0000 | Power | 0=Off, 1=On | Включение/выключение |
| 0x0006 | Eco | 0=Off, 1=On | Эко-режим |
| 0x0007 | Econavi | 0=Off, 1=On | Econavi режим |
| 0x0008 | Mild Dry | 0=Off, 1=On | Mild Dry режим |
| 0x0009 | NanoeX | 0=Off, 1=On | NanoeX ионизатор |

### Holding Registers (чтение/запись) - Function 03/06

| Адрес | Имя | Значение | Описание |
|-------|-----|----------|----------|
| 0x0001 | Mode | 0=Off, 1=Heat, 2=Cool, 3=Dry, 4=FAN, 5=Auto | Режим работы |
| 0x0002 | Target Temp | 160-300 (16.0-30.0°C) | Установка температуры |
| 0x0003 | Fan Mode | 0=Auto, 1=1, 2=2, 3=3, 4=4, 5=5 | Скорость вентилятора |
| 0x0004 | Swing V | 0=Off, 1=Swing, 2=Up, 3=UpCenter, 4=Center, 5=DownCenter, 6=Down | Вертикальные жалюзи |
| 0x0005 | Swing H | 0=Off, 1=Auto, 2=Left, 3=LeftCenter, 4=Center, 5=RightCenter, 6=Right | Горизонтальные жалюзи |
| 0x000A | Preset | 0=Normal, 1=Powerful, 2=Quiet | Дополнительный режим работы |

### Input Registers (только чтение) - Function 04

| Адрес | Имя | Значение | Описание |
|-------|-----|----------|----------|
| 0x0010 | Current Temp | Температура × 10 | Текущая температура в помещении |
| 0x0011 | Outside Temp | Температура × 10 | Наружная температура |
| 0x0012 | Power | Ватты | Потребляемая мощность |
| 0x0013 | Status | Биты состояния | Статус кондиционера |
| 0x0071 | Firmware | 250 = v2.5.0 | Версия прошивки |
| 0x0072 | Device ID | 0xAC01 | Идентификатор устройства |

### System Holding Registers

| Адрес | Имя | Значение | Описание |
|-------|-----|----------|----------|
| 0x0070 | Device Address | 1-247 | Modbus адрес устройства (чтение/запись) |

### Биты статуса (регистр 0x0013)

| Бит | Описание |
|-----|----------|
| 0 | Power ON |
| 1 | Compressor ON |
| 2 | Defrost |

## Примеры запросов Modbus

### Включить кондиционер
```
Запрос:  01 05 00 00 FF 00 CRC CRC
Ответ:   01 05 00 00 FF 00 CRC CRC
```

### Установить режим COOL (0x02)
```
Запрос:  01 06 00 01 00 02 CRC CRC
Ответ:   01 06 00 01 00 02 CRC CRC
```

### Установить температуру 24°C (240 = 0x00F0)
```
Запрос:  01 06 00 02 00 F0 CRC CRC
Ответ:   01 06 00 02 00 F0 CRC CRC
```

### Установить preset Powerful
```
Запрос:  01 06 00 0A 00 01 CRC CRC
Ответ:   01 06 00 0A 00 01 CRC CRC
```

### Прочитать текущую температуру
```
Запрос:  01 04 00 10 00 01 CRC CRC
Ответ:   01 04 02 00 F0 CRC CRC  (24.0°C)
```

### Прочитать мощность
```
Запрос:  01 04 00 12 00 01 CRC CRC
Ответ:   01 04 02 01 2C CRC CRC  (300W)
```

### Изменить Modbus адрес устройства (например, на адрес 0x05)
```
Запрос:  01 06 00 70 00 05 CRC CRC
Ответ:   01 06 00 70 00 05 CRC CRC
```
После смены адреса устройство отвечает только по новому адресу!
Чтобы снова найти устройство, используйте broadcast адрес 0 или перебирайте адреса.

### Прочитать текущий Modbus адрес
```
Запрос:  01 03 00 70 00 01 CRC CRC
Ответ:   01 03 02 00 01 CRC CRC  (адрес = 1)
```

### Прочитать версию прошивки
```
Запрос:  01 04 00 71 00 01 CRC CRC
Ответ:   01 04 02 00 FA CRC CRC  (v2.5.0 = 250)
```

### Прочитать ID устройства
```
Запрос:  01 04 00 72 00 01 CRC CRC
Ответ:   01 04 02 AC 01 CRC CRC  (0xAC01 = Panasonic AC)
```

## Конфигурация YAML

```yaml
esphome:
  name: panasonic-ac

esp32:
  board: esp32-c3-devkitm-1
  variant: esp32c3

# UART для подключения к кондиционеру (CN-CNT)
uart:
  - id: uart_ac
    tx_pin: GPIO20
    rx_pin: GPIO21
    baud_rate: 9600
    parity: EVEN

  # UART для RS485 Modbus
  - id: uart_modbus
    tx_pin: GPIO5
    rx_pin: GPIO6
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 1

# Конфигурация Modbus
modbus:
  - uart_id: uart_modbus
    id: modbus_bus
    # flow_control_pin: GPIO7  # Опционально

# Локальные компоненты
external_components:
  - source:
      type: local
      path: components
    components: [panasonic_ac, panasonic_ac_modbus]

# Modbus шлюз
panasonic_ac_modbus:
  id: ac_modbus
  address: 0x01  # Адрес устройства на шине Modbus

# Управление кондиционером
climate:
  - platform: panasonic_ac
    type: cnt
    name: Panasonic AC
    uart_id: uart_ac
    modbus: ac_modbus  # Синхронизация с Modbus
    horizontal_swing_select:
      name: Panasonic AC Horizontal Swing Mode
    vertical_swing_select:
      name: Panasonic AC Vertical Swing Mode
    outside_temperature:
      name: Panasonic AC Outside Temperature
    eco_switch:
      name: Panasonic AC Eco Switch
    econavi_switch:
      name: Panasonic AC Econavi Switch
    mild_dry_switch:
      name: Panasonic AC Mild Dry Switch
    current_power_consumption:
      name: Panasonic AC Power Consumption
```

## Параметры подключения RS485

- **Baud rate**: 9600
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1
- **Modbus RTU**
- **Адрес устройства**: 0x01 (настраивается)

## Управление Modbus адресом через Web UI

В Web UI ESPHome (порт 80) появится поле "Modbus Device Address" в разделе Configuration:

- Минимальное значение: 1
- Максимальное значение: 247
- Адрес сохраняется во flash памяти автоматически
- После смены устройство сразу начинает отвечать по новому адресу

### YAML для управления адресом
```yaml
number:
  - platform: template
    name: "Modbus Device Address"
    min_value: 1
    max_value: 247
    step: 1
    entity_category: config
    on_value:
      then:
        - lambda: 'id(ac_modbus).set_address((uint8_t)x);'
```

## Динамическое изменение Modbus адреса

Адрес устройства можно изменить удаленно через Modbus:

1. **Через Holding Register 0x0070** (рекомендуется):
   - Запишите новый адрес (1-247) в регистр 0x0070
   - Устройство сохранит адрес во flash памяти
   - После записи устройство отвечает только по новому адресу
   - Перезагрузка не требуется

2. **Сохранение в flash**:
   - Новый адрес сохраняется автоматически
   - После перезагрузки ESP устройство использует сохраненный адрес
   - Если flash пустой - используется адрес из конфигурации YAML

3. **Поиск устройства после смены адреса**:
   - Используйте broadcast адрес `0x00` (только для записи)
   - Или перебирайте все адреса 1-247 для опроса
   - Прочитайте регистр 0x0072 (Device ID = 0xAC01) для идентификации

### Пример смены адреса через broadcast
```
Запрос (broadcast, адрес 0):  00 06 00 70 00 0A CRC CRC  (новый адрес = 10)
Ответ: нет (broadcast не требует ответа)
```
Все устройства на шине получат команду, но изменит адрес только то, которое распознает регистр 0x0070.

## Диагностика

Включите debug логирование для проверки Modbus команд:

```yaml
logger:
  level: DEBUG
  logs:
    modbus: DEBUG
    panasonic_ac.modbus: DEBUG
```

## Интеграция с Home Assistant

Modbus регистры автоматически синхронизируются с Home Assistant через основной компонент climate. Но вы также можете добавить Modbus интеграцию в HA для прямого доступа к регистрам:

```yaml
# configuration.yaml Home Assistant
modbus:
  - type: serial
    port: /dev/ttyUSB0
    baudrate: 9600
    climate:
      - name: "Panasonic AC Modbus"
        slave: 1
        address: 1
        data_type: uint16
        count: 1
        temp_step: 0.5
        max_temp: 30
        min_temp: 16
```

## Безопасность

- Убедитесь, что адрес Modbus уникален на шине
- Используйте изолированный RS485 адаптер для промышленной среды
- Настройте таймауты в Modbus мастере (рекомендуется 100-500ms)
