/* Copyright 2026 @ MONTECH community port
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hardware configuration derived from Lemokey P1 Pro (same PCB):
 *   keyboards/lemokey/p1_pro
 */

#pragma once

/* Encoder Configuration */
#define ENCODER_DEFAULT_POS 0x3
#define ENCODER_MAP_KEY_DELAY 2

/* I2C Driver Configuration (external EEPROM) */
#define I2C1_SCL_PIN B8
#define I2C1_SDA_PIN B9
#define I2C1_CLOCK_SPEED 400000
#define I2C1_DUTY_CYCLE FAST_DUTY_CYCLE_2

/* EEPROM Driver Configuration */
#define EXTERNAL_EEPROM_BYTE_COUNT 2048
#define EXTERNAL_EEPROM_PAGE_SIZE 32
#define EXTERNAL_EEPROM_WRITE_TIME 3

/* User used eeprom */
#define I2C1_OPMODE OPMODE_I2C
#define EXTERNAL_EEPROM_I2C_BASE_ADDRESS 0b10100010
#define KEYCODE_BUFFER_ENABLE

#define LED_DRIVER_SHUTDOWN_PIN B14

/* Hold Fn+Win to lock Win key */
#define WIN_LOCK_HOLD_TIME 3000

/* Firmware Version for VIA */
#define VIA_FIRMWARE_VERSION 0x00000001

#ifdef RGB_MATRIX_ENABLE
/* RGB Matrix driver configuration */
#    define DRIVER_COUNT 2
/* 81 клавиша + 7 LED кольца энкодера + 2 LED индикации заряда = 90.
 * Кольцо и индикация заряда отсутствуют в матрице клавиатуры. */
#define RGB_MATRIX_LED_COUNT 90

#    define SPI_SCK_PIN A5
#    define SPI_MISO_PIN A6
#    define SPI_MOSI_PIN A7

#    define DRIVER_CS_PINS \
        { B15, C6 }
#    define SNLED23751_SPI_DIVISOR 16
#    define SPI_DRIVER SPIDQ

/* Кольцо энкодера сидит на каналах вплоть до L_16, т.е. нужны все 12 фаз.
 * В заводской прошивке в регистр 0x14 пишется 0x00 (MSKPHASE_12CHANNEL). */
#    define SNLED27351_PHASE_CHANNEL MSKPHASE_12CHANNEL

/* Ток драйвера. Заводская прошивка при 12 фазах пишет 0x1B (а не 0x2C, как P1 Pro
 * при 9 фазах) — берём ровно это значение, иначе яркость будет отличаться. */
#    define SNLED27351_CURRENT_TUNE \
        { 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B }

/* Set to infinit, which is use in USB mode by default */
#    define RGB_MATRIX_TIMEOUT RGB_MATRIX_TIMEOUT_INFINITE

/* Allow shutdown of led driver to save power */
#    define RGB_MATRIX_DRIVER_SHUTDOWN_ENABLE
/* Turn off backlight on low brightness to save power */
#    define RGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL 32

/* Caps lock indicating led */
#    define CAPS_LOCK_INDEX 44
#    define DIM_CAPS_LOCK
#    define SPACE_KEY_LOW_BAT_IND { 74 }
#    define MKEY_WIN_LOCK_LED 72

#    define RGB_MATRIX_KEYPRESSES
#    define RGB_MATRIX_FRAMEBUFFER_EFFECTS

#endif

#ifdef LK_WIRELESS_ENABLE
/* Hardware configuration */
#    define WT_DRIVER SPIDQ

/* MKey PRO 75 routes the two wireless switch positions opposite to P1 Pro. */
#    define P2P4_MODE_SELECT_PIN C13
#    define BT_MODE_SELECT_PIN C11

#    define LKBT51_RESET_PIN C4
#    define LKBT51_INT_INPUT_PIN B1
#    define BLUETOOTH_INT_OUTPUT_PIN A4

#    define USB_POWER_SENSE_PIN B0
#    define USB_POWER_CONNECTED_LEVEL 0

#    define BAT_CHARGING_PIN C10
#    define BAT_CHARGING_LEVEL 0

#    define BAT_LOW_LED_PIN C5
#    define BAT_LOW_LED_PIN_ON_STATE 1

#    define DP_PULLUP_CONTROL_PIN C6

#    define BT_HOST_DEVICES_COUNT 3

#    if defined(RGB_MATRIX_ENABLE) || defined(LED_MATRIX_ENABLE)

#        define BT_INDICATION_LED_LIST \
            { 15, 16, 17 }

#        define P24G_INDICATION_LED_INDEX 18

#        define BAT_LEVEL_LED_LIST \
            { 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 }

/* Backlit disable timeout when keyboard is disconnected(unit: second) */
#        define DISCONNECTED_BACKLIGHT_DISABLE_TIMEOUT 60

/* Backlit disable timeout when keyboard is connected(unit: second) */
#        define CONNECTED_BACKLIGHT_DISABLE_TIMEOUT 60

/* Reinit LED driver on transport changed */
#        define REINIT_LED_DRIVER 1

#    endif

/* Keep USB connection in bluetooth mode */
#    define KEEP_USB_CONNECTION_IN_WIRELESS_MODE

/* Enable bluetooth NKRO */
#    define WIRELESS_NKRO_ENABLE

/* Raw hid command for factory test and bluetooth DFU */
#    define RAW_HID_CMD 0xAA ... 0xAB
#else
/* Raw hid command for factory test */
#    define RAW_HID_CMD 0xAB
#endif

/* Factory test keys */
#define DIP_SWITCH_PINS { C0 }

#define FN_KEY_1 MO(1)
#define FN_KEY_2 MO(3)
#define FN_BL_TRIG_KEY KC_END

#define MATRIX_IO_DELAY 10
