/* Copyright 2026 @ MONTECH community port
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Vial + VialRGB. Протокол совместим с Vial GUI и с контроллером
 * "QMK VialRGB" из OpenRGB.
 */

#include "quantum.h"
#include "raw_hid.h"
#include "mkey_vialrgb.h"
#include "battery.h"
#include "lpm.h"

#ifdef RGB_MATRIX_ENABLE

/* ---- идентификаторы Vial / VialRGB ------------------------------------- */

#define VIAL_PROTOCOL_VERSION 4
#define VIALRGB_PROTOCOL_VERSION 1

enum {
    VIAL_GET_KEYBOARD_ID = 0x00,
};

enum {
    CMD_CUSTOM_SET_VALUE = 0x07,
    CMD_CUSTOM_GET_VALUE = 0x08,
    CMD_CUSTOM_SAVE      = 0x09,
    CMD_VIAL             = 0xFE,
};

enum {
    VIALRGB_GET_INFO       = 0x40,
    VIALRGB_GET_MODE       = 0x41,
    VIALRGB_GET_SUPPORTED  = 0x42,
    VIALRGB_GET_NUM_LEDS   = 0x43,
    VIALRGB_GET_LED_INFO   = 0x44,
    VIALRGB_GET_BATTERY    = 0x45,
    VIALRGB_SET_MODE       = 0x41,
    VIALRGB_DIRECT_FASTSET = 0x42,
};

/* Каналы для раскраски по клавишам из VIA (usevia.app):
 *   32 = цвет клавиши (hue, sat), value_id = индекс LED, 240 = все сразу
 *   33 = яркость клавиши (0..15), value_id = индекс LED, 240 = все сразу */
#define CH_PERKEY_COLOR 32
#define CH_PERKEY_BRI   33
#define CH_ALL          240
#define VIA_CH_LIGHTING 3 /* канал штатного освещения VIA */

/* Служебная команда (не часть VialRGB): залить все LED одним цветом.
 * args = h, s, v. Один пакет вместо десяти — удобно для инструментов. */
#define VIALRGB_FILL_ALL 0x43

/* Служебная команда: временно отдать светодиоды заряда хосту
 * (args[0] = 1 приостановить индикатор, 0 — вернуть). */
#define MKEY_CMD_BAT_SUSPEND 0x44

static bool bat_suspended = false;

bool mkey_vial_battery_suspended(void) {
    return bat_suspended;
}

/* Значения из спецификации VialRGB (никогда не переиспользовать) */
enum {
    VIALRGB_EFFECT_OFF = 0,
    VIALRGB_EFFECT_DIRECT,
    VIALRGB_EFFECT_SOLID_COLOR,
    VIALRGB_EFFECT_ALPHAS_MODS,
    VIALRGB_EFFECT_GRADIENT_UP_DOWN,
    VIALRGB_EFFECT_GRADIENT_LEFT_RIGHT,
    VIALRGB_EFFECT_BREATHING,
    VIALRGB_EFFECT_BAND_SAT,
    VIALRGB_EFFECT_BAND_VAL,
    VIALRGB_EFFECT_BAND_PINWHEEL_SAT,
    VIALRGB_EFFECT_BAND_PINWHEEL_VAL,
    VIALRGB_EFFECT_BAND_SPIRAL_SAT,
    VIALRGB_EFFECT_BAND_SPIRAL_VAL,
    VIALRGB_EFFECT_CYCLE_ALL,
    VIALRGB_EFFECT_CYCLE_LEFT_RIGHT,
    VIALRGB_EFFECT_CYCLE_UP_DOWN,
    VIALRGB_EFFECT_RAINBOW_MOVING_CHEVRON,
    VIALRGB_EFFECT_CYCLE_OUT_IN,
    VIALRGB_EFFECT_CYCLE_OUT_IN_DUAL,
    VIALRGB_EFFECT_CYCLE_PINWHEEL,
    VIALRGB_EFFECT_CYCLE_SPIRAL,
    VIALRGB_EFFECT_DUAL_BEACON,
    VIALRGB_EFFECT_RAINBOW_BEACON,
    VIALRGB_EFFECT_RAINBOW_PINWHEELS,
    VIALRGB_EFFECT_RAINDROPS,
    VIALRGB_EFFECT_JELLYBEAN_RAINDROPS,
};

#define VIALRGB_EFFECT_SKIP 0xFFFF

/* Карта: VialRGB id <-> эффект QMK. Порядок — по возрастанию id. */
typedef struct {
    uint16_t vialrgb_id;
    uint16_t qmk_id;
} vialrgb_mode_t;

static const vialrgb_mode_t supported_modes[] = {
    {VIALRGB_EFFECT_DIRECT, RGB_MATRIX_SOLID_COLOR}, /* обрабатывается отдельно */
    {VIALRGB_EFFECT_SOLID_COLOR, RGB_MATRIX_SOLID_COLOR},
    {VIALRGB_EFFECT_BREATHING, RGB_MATRIX_BREATHING},
    {VIALRGB_EFFECT_BAND_SPIRAL_VAL, RGB_MATRIX_BAND_SPIRAL_VAL},
    {VIALRGB_EFFECT_CYCLE_ALL, RGB_MATRIX_CYCLE_ALL},
    {VIALRGB_EFFECT_CYCLE_LEFT_RIGHT, RGB_MATRIX_CYCLE_LEFT_RIGHT},
    {VIALRGB_EFFECT_CYCLE_UP_DOWN, RGB_MATRIX_CYCLE_UP_DOWN},
    {VIALRGB_EFFECT_RAINBOW_MOVING_CHEVRON, RGB_MATRIX_RAINBOW_MOVING_CHEVRON},
    {VIALRGB_EFFECT_CYCLE_OUT_IN, RGB_MATRIX_CYCLE_OUT_IN},
    {VIALRGB_EFFECT_CYCLE_OUT_IN_DUAL, RGB_MATRIX_CYCLE_OUT_IN_DUAL},
    {VIALRGB_EFFECT_CYCLE_PINWHEEL, RGB_MATRIX_CYCLE_PINWHEEL},
    {VIALRGB_EFFECT_CYCLE_SPIRAL, RGB_MATRIX_CYCLE_SPIRAL},
    {VIALRGB_EFFECT_DUAL_BEACON, RGB_MATRIX_DUAL_BEACON},
    {VIALRGB_EFFECT_RAINBOW_BEACON, RGB_MATRIX_RAINBOW_BEACON},
    {VIALRGB_EFFECT_JELLYBEAN_RAINDROPS, RGB_MATRIX_JELLYBEAN_RAINDROPS},
};

#define SUPPORTED_MODES_LENGTH (sizeof(supported_modes) / sizeof(*supported_modes))

/* ---- состояние --------------------------------------------------------- */

static HSV     direct_colors[RGB_MATRIX_LED_COUNT];
static uint16_t current_mode   = VIALRGB_EFFECT_OFF;
static bool     direct_enabled = false;

bool mkey_vial_direct_active(void) {
    return direct_enabled;
}

void mkey_vial_get_direct(uint8_t index, uint8_t *h, uint8_t *s, uint8_t *v) {
    if (index >= RGB_MATRIX_LED_COUNT) {
        *h = *s = *v = 0;
        return;
    }
    *h = direct_colors[index].h;
    *s = direct_colors[index].s;
    *v = direct_colors[index].v;
}

/* ---- режимы ------------------------------------------------------------ */

static uint16_t vial_to_qmk(uint16_t id) {
    for (uint8_t i = 0; i < SUPPORTED_MODES_LENGTH; i++) {
        if (supported_modes[i].vialrgb_id == id) return supported_modes[i].qmk_id;
    }
    return 0;
}

static uint16_t qmk_to_vial(uint16_t id) {
    if (!rgb_matrix_is_enabled()) return VIALRGB_EFFECT_OFF;
    if (direct_enabled) return VIALRGB_EFFECT_DIRECT;
    for (uint8_t i = 0; i < SUPPORTED_MODES_LENGTH; i++) {
        if (supported_modes[i].qmk_id == id) return supported_modes[i].vialrgb_id;
    }
    return VIALRGB_EFFECT_OFF;
}

static void set_mode(uint16_t mode) {
    current_mode = mode;
    if (mode == VIALRGB_EFFECT_OFF) {
        direct_enabled = false;
        rgb_matrix_disable_noeeprom();
        return;
    }
    if (mode == VIALRGB_EFFECT_DIRECT) {
        direct_enabled = true;
        rgb_matrix_enable_noeeprom();
        rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
        return;
    }
    uint16_t qmk = vial_to_qmk(mode);
    if (qmk) {
        direct_enabled = false;
        rgb_matrix_enable_noeeprom();
        rgb_matrix_mode_noeeprom(qmk);
    }
}

/* ---- get --------------------------------------------------------------- */

static void get_info(uint8_t *args) {
    args[0] = VIALRGB_PROTOCOL_VERSION & 0xFF;
    args[1] = VIALRGB_PROTOCOL_VERSION >> 8;
    args[2] = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
}

static void get_mode(uint8_t *args) {
    uint16_t mode = qmk_to_vial(rgb_matrix_get_mode());
    if (!rgb_matrix_is_enabled()) mode = VIALRGB_EFFECT_OFF;
    args[0] = mode & 0xFF;
    args[1] = mode >> 8;
    args[2] = rgb_matrix_get_speed();
    args[3] = rgb_matrix_get_hue();
    args[4] = rgb_matrix_get_sat();
    args[5] = rgb_matrix_get_val();
}

static void get_supported(uint8_t *args, uint8_t length) {
    uint16_t gt;
    memcpy(&gt, args, sizeof(gt));

    for (uint8_t i = 0; i < length; i++) args[i] = 0xFF;

    uint8_t pos = 0;
    for (uint8_t i = 0; i < SUPPORTED_MODES_LENGTH; i++) {
        uint16_t id = supported_modes[i].vialrgb_id;
        if (id > gt && length >= pos + 2) {
            args[pos++] = id & 0xFF;
            args[pos++] = id >> 8;
        }
    }
}

static void get_led_info(uint16_t led, uint8_t *args) {
    if (led >= RGB_MATRIX_LED_COUNT) return;

    args[0] = g_led_config.point[led].x;
    args[1] = g_led_config.point[led].y;
    args[2] = g_led_config.flags[led];

    args[3] = 0xFF;
    args[4] = 0xFF;
    args[5] = 0;

    /* Кольцо энкодера не имеет позиции в матрице клавиатуры. Отдаём его отдельной
     * колонкой matrix_col = MATRIX_COLS, чтобы OpenRGB показал кольцо отдельной
     * секцией справа, а не потерял эти LED. */
    if (led >= LED_BAR_FIRST && led < LED_BAR_FIRST + LED_BAR_COUNT) {
        args[3] = led - LED_BAR_FIRST;
        args[4] = MATRIX_COLS;
        args[5] = 1;
        return;
    }

    /* Индикация заряда — вторая отдельная колонка справа. */
    if (led >= BAT_LED_FIRST && led < BAT_LED_FIRST + BAT_LED_COUNT) {
        args[3] = led - BAT_LED_FIRST;
        args[4] = MATRIX_COLS + 1;
        args[5] = 1;
        return;
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (g_led_config.matrix_co[row][col] == led) {
                args[3] = row;
                args[4] = col;
                args[5] = 1;
                return;
            }
        }
    }
}

/* ---- set --------------------------------------------------------------- */

static void set_leds(uint8_t *args, uint8_t length) {
    if (length < 3) return;

    uint16_t first = args[0] | (args[1] << 8);
    uint8_t  count = args[2];
    args += 3;
    length -= 3;

    if (count * 3 > length) return;

    for (uint8_t i = 0; i < count; i++) {
        if (first + i >= RGB_MATRIX_LED_COUNT) break;
        direct_colors[first + i].h = args[i * 3 + 0];
        direct_colors[first + i].s = args[i * 3 + 1];
        uint8_t val                = args[i * 3 + 2];
        direct_colors[first + i].v = (val > RGB_MATRIX_MAXIMUM_BRIGHTNESS) ? RGB_MATRIX_MAXIMUM_BRIGHTNESS : val;
    }
}

/* ---- точка входа ------------------------------------------------------- */

/* Имена LED в OpenRGB берутся из кейкода в позиции (row,col) слоя 0.
 * У кольца и индикации заряда позиции вне матрицы (col >= MATRIX_COLS),
 * поэтому на такой запрос отвечаем сами — иначе имя пустое. */
static bool get_keycode_ext(uint8_t *data, uint8_t length) {
    uint8_t row = data[2];
    uint8_t col = data[3];
    if (col < MATRIX_COLS) return false;

    uint16_t kc = KC_NO;

    if (col == MATRIX_COLS && row < LED_BAR_COUNT) {
        /* Кольцо: 7 позиций по кругу (Top, Upper Right, ... Upper Left) */
        static const uint16_t ring_kc[LED_BAR_COUNT] = {
            KC_UP,   /* Top         */
            KC_PGUP, /* Upper Right */
            KC_PGDN, /* Lower Right */
            KC_DOWN, /* Bottom      */
            KC_END,  /* Lower Left  */
            KC_LEFT, /* Left        */
            KC_HOME, /* Upper Left  */
        };
        kc = ring_kc[row];
    } else if (col == MATRIX_COLS + 1 && row < BAT_LED_COUNT) {
        static const uint16_t bat_kc[BAT_LED_COUNT] = {KC_INS, KC_DEL};
        kc = bat_kc[row];
    }

    data[4] = kc >> 8;
    data[5] = kc & 0xFF;
    raw_hid_send(data, length);
    return true;
}

bool mkey_vial_raw_hid(uint8_t *data, uint8_t length) {
    if (length < 2) return false;

    /* VIA: dynamic keymap get keycode (0x04) для вне-матричных позиций */
    if (data[0] == 0x04 && length >= 6 && get_keycode_ext(data, length)) {
        return true;
    }

    /* --- Vial: идентификация клавиатуры (нужна OpenRGB для детекта) --- */
    if (data[0] == CMD_VIAL && data[1] == VIAL_GET_KEYBOARD_ID) {
        uint32_t             ver = VIAL_PROTOCOL_VERSION;
        uint8_t              buf[32];
        static const uint8_t uid[8] = {'M', 'K', 'E', 'Y', '7', '5', '0', '1'};

        memset(buf, 0, sizeof(buf));
        memcpy(&buf[0], &ver, sizeof(ver));
        memcpy(&buf[4], uid, sizeof(uid));
        buf[12] = 0x01; /* bit0: VialRGB присутствует */

        memcpy(data, buf, sizeof(buf));
        raw_hid_send(data, length);
        return true;
    }

    uint8_t  cmd  = data[0];
    uint8_t  sub  = data[1];
    uint8_t *args = &data[2];

    if (sub == CH_PERKEY_COLOR || sub == CH_PERKEY_BRI) {
        if (cmd == CMD_CUSTOM_SAVE) {
            /* Direct colors are a live preview and only live in RAM. */
            raw_hid_send(data, length);
            return true;
        }
        if (length < 5) return false;
        if (cmd == CMD_CUSTOM_GET_VALUE) {
            uint8_t id = args[0] == CH_ALL ? 0 : args[0];
            HSV color = {0, 0, 0};
            if (id < RGB_MATRIX_LED_COUNT) color = direct_colors[id];
            if (sub == CH_PERKEY_COLOR) {
                args[1] = color.h;
                args[2] = color.s;
            } else {
                args[1] = (color.v + 8) / 17;
            }
            raw_hid_send(data, length);
            return true;
        }
    }

    /* Раскраска по клавишам из VIA — пишем в тот же буфер, что и Direct. */
    if (cmd == CMD_CUSTOM_SET_VALUE && (sub == CH_PERKEY_COLOR || sub == CH_PERKEY_BRI)) {
        uint8_t id  = args[0];
        uint8_t val = args[1];
        uint16_t first = (id == CH_ALL) ? 0 : id;
        uint16_t last  = (id == CH_ALL) ? (RGB_MATRIX_LED_COUNT - 1) : id;

        if (id == CH_ALL || id < RGB_MATRIX_LED_COUNT) {
            for (uint16_t i = first; i <= last && i < RGB_MATRIX_LED_COUNT; i++) {
                if (sub == CH_PERKEY_COLOR) {
                    direct_colors[i].h = val;          /* hue */
                    direct_colors[i].s = args[2];      /* sat  */
                    if (!direct_colors[i].v) direct_colors[i].v = 255;
                } else {
                    direct_colors[i].v = (val > 15) ? 255 : (uint8_t)(val * 17);
                }
            }
            direct_enabled = true;
            rgb_matrix_enable_noeeprom();
            rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
        }
        raw_hid_send(data, length);
        return true;
    }

    /* VIA выбрала обычный эффект — выходим из режима раскраски. */
    if (cmd == CMD_CUSTOM_SET_VALUE && sub == VIA_CH_LIGHTING && args[0] == 2) {
        direct_enabled = false;
        return false; /* пусть обработает сама VIA */
    }

    if (cmd == CMD_CUSTOM_GET_VALUE) {
        switch (sub) {
            case VIALRGB_GET_INFO:
                memset(args, 0, length - 2);
                get_info(args);
                break;
            case VIALRGB_GET_MODE:
                memset(args, 0, length - 2);
                get_mode(args);
                break;
            case VIALRGB_GET_SUPPORTED:
                get_supported(args, length - 2);
                break;
            case VIALRGB_GET_NUM_LEDS:
                memset(args, 0, length - 2);
                args[0] = RGB_MATRIX_LED_COUNT & 0xFF;
                args[1] = RGB_MATRIX_LED_COUNT >> 8;
                break;
            case VIALRGB_GET_LED_INFO: {
                uint16_t led;
                memcpy(&led, args, sizeof(led));
                memset(args, 0, length - 2);
                get_led_info(led, args);
                break;
            }
            case VIALRGB_GET_BATTERY: {
                /* диагностика: напряжение (мВ) и процент */
                uint16_t mv  = battery_get_voltage();
                uint8_t  pct = battery_get_percentage();
                memset(args, 0, length - 2);
                args[0] = mv & 0xFF;
                args[1] = mv >> 8;
                args[2] = pct;
                args[3] = usb_power_connected() ? 1 : 0;
#if defined(BAT_CHARGING_PIN)
                args[4] = (readPin(BAT_CHARGING_PIN) == BAT_CHARGING_LEVEL) ? 1 : 0;
#endif
                break;
            }
            default:
                return false;
        }
        raw_hid_send(data, length);
        return true;
    }

    if (cmd == CMD_CUSTOM_SET_VALUE) {
        switch (sub) {
            case VIALRGB_SET_MODE: {
                uint16_t mode = args[0] | (args[1] << 8);
                set_mode(mode);
                /* В режиме Direct OpenRGB присылает val=0 и цвет не задаёт —
                 * применять его нельзя, иначе подсветка гаснет целиком. */
                if (mode != VIALRGB_EFFECT_DIRECT) {
                    rgb_matrix_set_speed_noeeprom(args[2]);
                    rgb_matrix_sethsv_noeeprom(args[3], args[4], args[5]);
                }
                break;
            }
            case VIALRGB_DIRECT_FASTSET:
                set_leds(args, length - 2);
                break;
            case VIALRGB_FILL_ALL: {
                for (uint16_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
                    direct_colors[i].h = args[0];
                    direct_colors[i].s = args[1];
                    direct_colors[i].v = args[2];
                }
                direct_enabled = true;
                rgb_matrix_enable_noeeprom();
                rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
                break;
            }
            case MKEY_CMD_BAT_SUSPEND:
                bat_suspended = args[0] ? true : false;
                break;
            default:
                return false;
        }
        /* ВАЖНО: отвечаем всегда. OpenRGB после каждого пакета ждёт ответа,
         * иначе кадр растягивается на секунды (цвета «ползут» кусками). */
        raw_hid_send(data, length);
        return true;
    }

    return false;
}

#endif /* RGB_MATRIX_ENABLE */
