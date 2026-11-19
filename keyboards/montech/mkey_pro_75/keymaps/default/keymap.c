/* Copyright 2026 @ MONTECH community port
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include QMK_KEYBOARD_H
#include "lemokey_common.h"
#include "battery.h"
#include "lpm.h"
#include "mkey_vialrgb.h"

// clang-format off
enum layers {
    BASE,
    FN,
    MAC_BASE,
    MAC_FN,
};

enum my_keycodes {
    KNB_MODE = NEW_SAFE_RANGE,
    KNB_LIGHT,
};

/* Боковая полоска индикации — последние 7 LED подсветки */
#define KNOB_RING_FIRST LED_BAR_FIRST
#define KNOB_RING_LAST  (LED_BAR_FIRST + LED_BAR_COUNT - 1)

/* Режимы ручки, как на заводской прошивке: громкость -> зум -> эффект подсветки */
enum {
    KNOB_VOL = 0,
    KNOB_ZOOM,
    KNOB_RGB,
    KNOB_MODE_COUNT,
};

static bool     knob_light_enabled = true;
static uint8_t  knob_mode    = KNOB_VOL;
static uint32_t knob_ind_tmr = 0;
static uint32_t bat_disp_tmr = 0;

/* Version tag avoids interpreting erased or older EEPROM as settings. */
#define KNOB_SETTINGS_TAG 0x4D4B0100UL
#define KNOB_SETTINGS_LIGHT (1U << 2)

static void save_knob_settings(void) {
    eeconfig_update_user(KNOB_SETTINGS_TAG | knob_mode |
                         (knob_light_enabled ? KNOB_SETTINGS_LIGHT : 0));
}

void eeconfig_init_user(void) {
    eeconfig_update_user(KNOB_SETTINGS_TAG | KNOB_SETTINGS_LIGHT);
}

void keyboard_post_init_user(void) {
    uint32_t settings = eeconfig_read_user();
    uint8_t mode = settings & 3U;
    if ((settings & ~7UL) == KNOB_SETTINGS_TAG && mode < KNOB_MODE_COUNT) {
        knob_mode = mode;
        knob_light_enabled = (settings & KNOB_SETTINGS_LIGHT) != 0;
    }
}

/* Цифровой ряд 1..0 — шкала заряда */
static const uint8_t bat_leds[10] = {15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
     [BASE] = LAYOUT_ansi_82(
        KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_DEL,             KNB_MODE,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_HOME,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGUP,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,             KC_PGDN,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  KC_UP,
        KC_LCTL,  KC_LGUI,  KC_LALT,                                KC_SPC,                       KC_RALT, MO(FN),KC_RCTL,            KC_LEFT,  KC_DOWN,  KC_RGHT),

    /* Fn-слой повторяет заводской: RGB на Esc/F-ряду, медиа, BT на 1/2/3, 2.4G на 4,
     * Ins/End на Fn+PgUp/PgDn, RGB-регулировки на стрелках, GU_TOGG на Fn+Win.
     * Fn+Del — сброс EEPROM (раскладка вернётся к этой, из прошивки). */
    [FN] = LAYOUT_ansi_82(
        RGB_TOG,  RGB_MOD,  RGB_SPI,  RGB_SPD,  RGB_RMOD, KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_PSCR,  KC_MUTE,  KC_TASK,  KC_SLEP,  KC_LOCK,  EE_CLR,             KC_MUTE,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        _______,  _______,  _______,  _______,  RGB_SAI,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            KC_INS,
        _______,  _______,  _______,  _______,  RGB_SAD,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,            KC_END,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,  _______,  RGB_VAI,
        _______,  GU_TOGG,  _______,                                _______,                      _______,  _______,  _______,            RGB_HUD,  RGB_VAD,  RGB_HUI),

    [MAC_BASE] = LAYOUT_ansi_82(
        KC_ESC,   RGB_MOD,  RGB_SPI,  RGB_SPD,  KNB_LIGHT,KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_LNPAD, KC_MUTE,  KC_VOLD,  KC_VOLU,  KC_F12,   KC_DEL,             KNB_MODE,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_HOME,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGUP,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,             KC_PGDN,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  KC_UP,
        KC_LCTL,  KC_LALT,  KC_LGUI,                                KC_SPC,                       KC_RGUI, MO(MAC_FN), KC_RCTL,            KC_LEFT,  KC_DOWN,  KC_RGHT),

    [MAC_FN] = LAYOUT_ansi_82(
        RGB_TOG,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,  EE_CLR,             KC_MUTE,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        _______,  _______,  _______,  _______,  RGB_SAI,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            KC_INS,
        _______,  _______,  _______,  _______,  RGB_SAD,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,            KC_END,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,  _______,  RGB_VAI,
        _______,  _______,  _______,                                _______,                      _______,  _______,  _______,            RGB_HUD,  RGB_VAD,  RGB_HUI),

};
// clang-format on

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KNB_MODE:
            if (record->event.pressed) {
                knob_mode    = (knob_mode + 1) % KNOB_MODE_COUNT;
                knob_ind_tmr = timer_read32();
                save_knob_settings();
            }
            return false;

        case KNB_LIGHT:
            if (record->event.pressed) {
                knob_light_enabled = !knob_light_enabled;
                save_knob_settings();
            }
            return false;

        case BAT_LVL:
            /* свой показ уровня: работает и по проводу, не только по радио */
            if (record->event.pressed) {
                bat_disp_tmr = timer_read32();
            }
            break; /* дальше отдаём в заводской обработчик */
    }

    if (!process_record_lemokey_common(keycode, record)) {
        return false;
    }
    return true;
}

/* Вращение ручки: на Fn — яркость подсветки, иначе по текущему режиму. */
bool encoder_update_user(uint8_t index, bool clockwise) {
    rgb_matrix_disable_time_reset();
    lpm_timer_reset();
    if (layer_state_is(FN) || layer_state_is(MAC_FN)) {
        clockwise ? rgb_matrix_increase_val() : rgb_matrix_decrease_val();
        return false;
    }

    switch (knob_mode) {
        case KNOB_VOL:
            tap_code(clockwise ? KC_VOLU : KC_VOLD);
            break;
        case KNOB_ZOOM:
            if (get_highest_layer(default_layer_state) == MAC_BASE) {
                tap_code16(clockwise ? G(KC_EQL) : G(KC_MINS));
            } else {
                tap_code16(clockwise ? C(KC_EQL) : C(KC_MINS));
            }
            break;
        case KNOB_RGB:
            clockwise ? rgb_matrix_step() : rgb_matrix_step_reverse();
            break;
    }
    return false;
}

/* Отрисовка поверх эффекта: прямой режим (OpenRGB/Vial), полоска заряда, режим ручки. */
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    /* 1. Прямой режим: цвета из приложения. OpenRGB сам масштабирует их своим
     * ползунком яркости, поэтому здесь ничего не пересчитываем. */
    if (mkey_vial_direct_active()) {
        for (uint8_t i = led_min; i < led_max && i < RGB_MATRIX_LED_COUNT; i++) {
            uint8_t h, s, v;
            mkey_vial_get_direct(i, &h, &s, &v);
            HSV hsv = {h, s, v};
            RGB rgb = hsv_to_rgb(hsv);
            rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        }
    }

    /* 2. Индикация заряда на боковых светодиодах — всегда */
    mkey_draw_battery_leds(led_min, led_max);

    /* The lock marker follows backlight sleep so it cannot keep the driver on. */
    if (keymap_config.no_gui && MKEY_WIN_LOCK_LED >= led_min && MKEY_WIN_LOCK_LED < led_max) {
        rgb_matrix_set_color(MKEY_WIN_LOCK_LED, 255, 0, 0);
    }

    /* 3. Кольцо энкодера: вспышка при смене режима ручки.
     * В остальное время кольцо рисует текущий эффект — оно отдельная секция
     * в OpenRGB и управляется как обычные LED. */
    if (!knob_light_enabled) {
        for (uint8_t i = KNOB_RING_FIRST; i <= KNOB_RING_LAST; i++) {
            if (i >= led_min && i < led_max) rgb_matrix_set_color(i, 0, 0, 0);
        }
    } else if (knob_ind_tmr && timer_elapsed32(knob_ind_tmr) < 700) {
        static const uint8_t hue[KNOB_MODE_COUNT] = {0, 85, 170};
        HSV                  hsv                  = {hue[knob_mode], 255, 255};
        RGB                  rgb                  = hsv_to_rgb(hsv);
        for (uint8_t i = KNOB_RING_FIRST; i <= KNOB_RING_LAST; i++) {
            if (i >= led_min && i < led_max) rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        }
    }

    /* 3. Уровень заряда на цифровом ряду по Fn+B */
    if (mkey_battery_has_measurement() && bat_disp_tmr && timer_elapsed32(bat_disp_tmr) < 4000) {
        uint8_t pct = battery_get_percentage();
        uint8_t n   = pct / 10;
        if (n == 0) n = 1;
        for (uint8_t i = 0; i < 10; i++) {
            if (bat_leds[i] < led_min || bat_leds[i] >= led_max) continue;
            if (i < n) {
                if (pct <= 30) {
                    rgb_matrix_set_color(bat_leds[i], 255, 0, 0);
                } else if (pct <= 60) {
                    rgb_matrix_set_color(bat_leds[i], 255, 120, 0);
                } else {
                    rgb_matrix_set_color(bat_leds[i], 0, 255, 0);
                }
            } else {
                rgb_matrix_set_color(bat_leds[i], 0, 0, 0);
            }
        }
    }
    return false;
}
