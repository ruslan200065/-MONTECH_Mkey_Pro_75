/* Copyright 2026 @ MONTECH community port
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * LED wiring table and key matrix -> LED index map are byte-identical to the
 * Lemokey P1 Pro (same PCB / same LED driver chain), see keyboards/lemokey/p1_pro.
 */

#include "quantum.h"
#include "dip_switch.h"
#include "mkey_vialrgb.h"
#include "lemokey_task.h"
#ifdef FACTORY_TEST_ENABLE
#    include "factory_test.h"
#    include "lemokey_common.h"
#endif
#ifdef LK_WIRELESS_ENABLE
#    include "lkbt51.h"
#    include "wireless.h"
#    include "wireless_common.h"
#    include "battery.h"
#    include "lpm.h"
#    include "transport.h"
#endif

// clang-format off

#ifdef RGB_MATRIX_ENABLE
const snled27351_led_t PROGMEM g_snled27351_leds[RGB_MATRIX_LED_COUNT] = {
/* Refer to snled27351manual for these locations
 *   driver
 *   |  R location
 *   |  |       G location
 *   |  |       |       B location
 *   |  |       |       | */
    {0, A_15,   C_15,   B_15},
    {0, A_14,   C_14,   B_14},
    {0, A_13,   C_13,   B_13},
    {0, A_12,   C_12,   B_12},
    {0, A_11,   C_11,   B_11},
    {0, A_10,   C_10,   B_10},
    {0, A_9,    C_9,    B_9},
    {0, A_8,    C_8,    B_8},
    {0, A_7,    C_7,    B_7},
    {0, A_6,    C_6,    B_6},
    {0, A_5,    C_5,    B_5},
    {0, A_4,    C_4,    B_4},
    {0, A_3,    C_3,    B_3},
    {0, A_2,    C_2,    B_2},

    {0, G_15,   I_15,   H_15},
    {0, G_14,   I_14,   H_14},
    {0, G_13,   I_13,   H_13},
    {0, G_12,   I_12,   H_12},
    {0, G_11,   I_11,   H_11},
    {0, G_10,   I_10,   H_10},
    {0, G_9,    I_9,    H_9},
    {0, G_8,    I_8,    H_8},
    {0, G_7,    I_7,    H_7},
    {0, G_6,    I_6,    H_6},
    {0, G_5,    I_5,    H_5},
    {0, G_4,    I_4,    H_4},
    {0, G_3,    I_3,    H_3},
    {0, G_2,    I_2,    H_2},
    {0, G_1,    I_1,    H_1},

    {0, D_15,   F_15,   E_15},
    {0, D_14,   F_14,   E_14},
    {0, D_13,   F_13,   E_13},
    {0, D_12,   F_12,   E_12},
    {0, D_11,   F_11,   E_11},
    {0, D_10,   F_10,   E_10},
    {0, D_9,    F_9,    E_9},
    {0, D_8,    F_8,    E_8},
    {0, D_7,    F_7,    E_7},
    {0, D_6,    F_6,    E_6},
    {0, D_5,    F_5,    E_5},
    {0, D_4,    F_4,    E_4},
    {0, D_3,    F_3,    E_3},
    {0, D_2,    F_2,    E_2},
    {0, D_1,    F_1,    E_1},

    {1, A_15,   C_15,   B_15},
    {1, A_14,   C_14,   B_14},
    {1, A_13,   C_13,   B_13},
    {1, A_12,   C_12,   B_12},
    {1, A_11,   C_11,   B_11},
    {1, A_10,   C_10,   B_10},
    {1, A_9,    C_9,    B_9},
    {1, A_8,    C_8,    B_8},
    {1, A_7,    C_7,    B_7},
    {1, A_6,    C_6,    B_6},
    {1, A_5,    C_5,    B_5},
    {1, A_4,    C_4,    B_4},
    {1, A_2,    C_2,    B_2},
    {1, A_1,    C_1,    B_1},

    {1, G_15,   I_15,   H_15},
    {1, G_13,   I_13,   H_13},
    {1, G_12,   I_12,   H_12},
    {1, G_11,   I_11,   H_11},
    {1, G_10,   I_10,   H_10},
    {1, G_9,    I_9,    H_9},
    {1, G_8,    I_8,    H_8},
    {1, G_7,    I_7,    H_7},
    {1, G_6,    I_6,    H_6},
    {1, G_5,    I_5,    H_5},
    {1, G_4,    I_4,    H_4},
    {1, G_3,    I_3,    H_3},
    {1, G_2,    I_2,    H_2},

    {1, D_15,   F_15,   E_15},
    {1, D_14,   F_14,   E_14},
    {1, D_13,   F_13,   E_13},
    {1, D_9,    F_9,    E_9},
    {1, D_6,    F_6,    E_6},
    {1, D_5,    F_5,    E_5},
    {1, D_4,    F_4,    E_4},
    {1, D_3,    F_3,    E_3},
    {1, D_2,    F_2,    E_2},
    {1, D_1,    F_1,    E_1},

    /* --- кольцо энкодера, 7 LED (индексы 81..87), как в заводской прошивке --- */
    {0, D_16,   F_16,   E_16},
    {0, G_16,   I_16,   H_16},
    {1, A_16,   C_16,   B_16},
    {0, J_16,   L_16,   K_16},
    {1, G_16,   I_16,   H_16},
    {1, D_16,   F_16,   E_16},
    {0, A_16,   C_16,   B_16},
    /* Индикация заряда: два отдельных светодиода сбоку (driver0, J/L/K). */
    {0, J_15,   L_15,   K_15}, /* 88 */
    {0, J_14,   L_14,   K_14}, /* 89 */
};

#define __ NO_LED

led_config_t g_led_config = {
    {
        // Key Matrix to LED Index
        {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, __ },
        { 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 },
        { 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43 },
        { 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, __, 56, 57 },
        { 58, __, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, __},
        { 71, 72, 73, __, __, __, 74, __, __, 75, 76, 77, 78, 79, 80 },
    },
    {
        //  LED Index to Physical Position
        {0, 0}, {18, 0}, {33, 0}, {48, 0}, {62, 0}, {81, 0}, {95, 0}, {110, 0}, {125, 0}, {143, 0}, {158, 0}, {172, 0}, {187, 0}, {205, 0},
        {0,15}, {15,15}, {29,15}, {44,15}, {59,15}, {73,15}, {88,15}, {103,15}, {117,15}, {132,15}, {146,15}, {161,15}, {176,15}, {198,15}, {224,15},
        {4,26}, {22,26}, {37,26}, {51,26}, {66,26}, {81,26}, {95,26}, {110,26}, {125,26}, {139,26}, {154,26}, {168,26}, {183,26}, {201,26}, {224,26},
        {6,38}, {26,38}, {40,38}, {55,38}, {70,38}, {84,38}, {99,38}, {114,38}, {128,38}, {143,38}, {158,38}, {172,38},           {196,38}, {224,38},
        {8,49},          {33,49}, {48,49}, {62,49}, {77,49}, {92,49}, {106,49}, {121,49}, {136,49}, {150,49}, {165,49}, {185,49}, {209,52},
        {2,61}, {20,61}, {38,61},                            {94,61},                     {147,61}, {161,61}, {176,61}, {195,64}, {209,64}, {224,64},
        /* кольцо энкодера — 7 точек по кругу вокруг ручки */
        {216,2}, {211,12}, {210,7}, {213,3}, {219,3}, {222,7}, {221,12},
        /* индикация заряда — две точки у правого края */
        {224,4}, {224,14},
    },
    {
        // RGB LED Index to Flag
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    1, 1,
        1,    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1,          1,       1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,   /* кольцо энкодера */
        1, 1,                  /* индикация заряда */
    }
};
#endif

#define POWER_ON_LED_DURATION 3000
static uint32_t power_on_indicator_timer_buffer;

bool dip_switch_update_kb(uint8_t index, bool active) {
    if (index == 0) {
        /* C0 is low in Mac mode. This port keeps Win on layer 0. */
        default_layer_set(1UL << (active ? 2 : 0));
    }
    return dip_switch_update_user(index, active);
}

void keyboard_post_init_kb(void) {
#ifdef LK_WIRELESS_ENABLE
#    ifdef BT_MODE_SELECT_PIN
    palSetLineMode(BT_MODE_SELECT_PIN, PAL_MODE_INPUT);
#    endif
#    ifdef P2P4_MODE_SELECT_PIN
    palSetLineMode(P2P4_MODE_SELECT_PIN, PAL_MODE_INPUT);
#    endif

    palSetLineMode(B2, PAL_MODE_INPUT);
    palSetLineMode(C5, PAL_MODE_INPUT);

    writePin(BAT_LOW_LED_PIN, BAT_LOW_LED_PIN_ON_STATE);

    lkbt51_init(false);
    wireless_init();
#endif

    power_on_indicator_timer_buffer = timer_read32();

#ifdef ENCODER_ENABLE
    encoder_cb_init();
#endif
    keyboard_post_init_user();
}

bool lemokey_task_kb(void) {
    if (power_on_indicator_timer_buffer) {
        if (timer_elapsed32(power_on_indicator_timer_buffer) > POWER_ON_LED_DURATION) {
            power_on_indicator_timer_buffer = 0;

#ifdef LK_WIRELESS_ENABLE
            writePin(BAT_LOW_LED_PIN, !BAT_LOW_LED_PIN_ON_STATE);
#endif

        } else {
#ifdef LK_WIRELESS_ENABLE
            writePin(BAT_LOW_LED_PIN, BAT_LOW_LED_PIN_ON_STATE);
#endif
        }
    }
    return true;
}

#ifdef LK_WIRELESS_ENABLE
bool lpm_is_kb_idle(void) {
    return power_on_indicator_timer_buffer == 0 && !factory_reset_indicating();
}
#endif

void mkey_draw_battery_leds(uint8_t led_min, uint8_t led_max) {
    enum { SEGMENT_COUNT = 6, SEGMENT_BRIGHTNESS = 190, HYSTERESIS = 2 };
    static uint8_t segments = 0;
    static bool level_valid = false;
    /* Physical positions from batmap_result.json, zero-based, in RGB order. */
    static const uint8_t positions[BAT_LED_COUNT][3] = {{5, 1, 3}, {4, 0, 2}};

    if (!rgb_matrix_is_enabled() || mkey_vial_battery_suspended()) return;

    bool charging = usb_power_connected()
#if defined(BAT_CHARGING_PIN)
                    && (readPin(BAT_CHARGING_PIN) == BAT_CHARGING_LEVEL)
#endif
        ;

    bool measured = mkey_battery_has_measurement();
    if (measured) {
        uint8_t percent = MIN(battery_get_percentage(), 100);
        uint8_t target = percent * SEGMENT_COUNT / 100;
        if (!level_valid || target > segments) {
            segments = target;
        } else {
            uint8_t falling_percent = MIN(percent + HYSTERESIS, 100);
            segments = falling_percent * SEGMENT_COUNT / 100 < segments
                           ? falling_percent * SEGMENT_COUNT / 100
                           : segments;
        }
    } else {
        segments = 0;
    }
    level_valid = measured;

    uint8_t lit = (1U << segments) - 1;
    if (measured && charging && (timer_read32() / 500) % 2 == 0) {
        /* During the bright phase, turn on the remaining segments as well. */
        lit = (1U << SEGMENT_COUNT) - 1;
    }

    for (uint8_t i = 0; i < BAT_LED_COUNT; i++) {
        uint8_t led = BAT_LED_FIRST + i;
        if (led < led_min || led >= led_max) continue;
        uint8_t red   = (lit & (1U << positions[i][0])) ? SEGMENT_BRIGHTNESS : 0;
        uint8_t green = (lit & (1U << positions[i][1])) ? SEGMENT_BRIGHTNESS : 0;
        uint8_t blue  = (lit & (1U << positions[i][2])) ? SEGMENT_BRIGHTNESS : 0;
        rgb_matrix_set_color(led, red, green, blue);
    }
}

/* Limit LED output on battery without altering the saved VIA/QMK brightness. */
static void limit_wireless_rgb(uint8_t *red, uint8_t *green, uint8_t *blue) {
    enum { BATTERY_PWM_LIMIT = 128 };
    uint8_t peak = MAX(*red, MAX(*green, *blue));
    if (peak <= BATTERY_PWM_LIMIT || !(get_transport() & TRANSPORT_WIRELESS) || usb_power_connected()) return;

    *red   = (uint16_t)*red * BATTERY_PWM_LIMIT / peak;
    *green = (uint16_t)*green * BATTERY_PWM_LIMIT / peak;
    *blue  = (uint16_t)*blue * BATTERY_PWM_LIMIT / peak;
}

void __real_snled27351_set_color(int index, uint8_t red, uint8_t green, uint8_t blue);
void __real_snled27351_set_color_all(uint8_t red, uint8_t green, uint8_t blue);

void __wrap_snled27351_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    limit_wireless_rgb(&red, &green, &blue);
    __real_snled27351_set_color(index, red, green, blue);
}

void __wrap_snled27351_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    limit_wireless_rgb(&red, &green, &blue);
    __real_snled27351_set_color_all(red, green, blue);
}
