/* Copyright 2026 @ MONTECH community port
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Vial / VialRGB support.
 *
 * Позволяет управлять подсветкой по клавишам из внешних приложений:
 *   - Vial GUI (вкладка Lighting)
 *   - OpenRGB (контроллер "QMK VialRGB": устройство добавляется в настройках
 *     OpenRGB по VID/PID 0x362D/0x1700)
 *
 * Протокол (raw HID, 32 байта):
 *   [0] = 0xFE, [1] = 0x00                     -> VIAL_GET_KEYBOARD_ID
 *   [0] = 0x08 / 0x07, [1] = 0x40..0x44        -> VialRGB get / set
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Кольцо энкодера: светодиоды 81..87 (индексы фиксированы раскладкой таблицы,
 * не зависят от RGB_MATRIX_LED_COUNT — в диагностической сборке их больше). */
#define LED_BAR_FIRST 81
#define LED_BAR_COUNT 7

/* Индикация заряда: два отдельных светодиода сбоку (88, 89). */
#define BAT_LED_FIRST 88
#define BAT_LED_COUNT 2

bool mkey_battery_has_measurement(void);
void mkey_draw_battery_leds(uint8_t led_min, uint8_t led_max);

/* Возвращает true, если пакет обработан (ответ уже отправлен). */
bool mkey_vial_raw_hid(uint8_t *data, uint8_t length);

/* Включён ли режим прямой адресации цветов (плашка OpenRGB/Vial). */
bool mkey_vial_direct_active(void);

/* Приостановлен ли штатный индикатор заряда (чтобы инструмент мог мигать
 * светодиодами заряда сам). */
bool mkey_vial_battery_suspended(void);

/* Забрать цвет LED index (HSV) для режима прямой адресации. */
void mkey_vial_get_direct(uint8_t index, uint8_t *h, uint8_t *s, uint8_t *v);
