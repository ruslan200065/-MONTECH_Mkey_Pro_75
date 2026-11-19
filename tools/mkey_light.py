#!/usr/bin/env python3
"""
MKey PRO 75 - USB lighting control (VialRGB).

Works with firmware v4+ (which adds Vial/VialRGB).

    python tools/mkey_light.py --selftest      # check protocol support
    python tools/mkey_light.py --rainbow       # animated per-key rainbow
    python tools/mkey_light.py --color FF0044  # solid color, all LEDs
    python tools/mkey_light.py --off           # backlight off

Requires: pip install hidapi
"""

import argparse
import sys
import time

try:
    import hid
except ImportError:
    print("hidapi is required:  pip install hidapi")
    sys.exit(1)

VID = 0x362D
PID = 0x1700
USAGE_PAGE = 0xFF60
USAGE = 0x61
REPORT = 32

CMD_GET_PROTOCOL_VERSION = 0x01
CMD_CUSTOM_SET_VALUE = 0x07
CMD_CUSTOM_GET_VALUE = 0x08
CMD_VIAL = 0xFE
CMD_FW_PROTOCOL = 0xA0

VIALRGB_GET_INFO = 0x40
VIALRGB_GET_MODE = 0x41
VIALRGB_GET_SUPPORTED = 0x42
VIALRGB_GET_NUM_LEDS = 0x43
VIALRGB_GET_LED_INFO = 0x44
VIALRGB_GET_BATTERY = 0x45
VIALRGB_SET_MODE = 0x41
VIALRGB_DIRECT_FASTSET = 0x42

EFFECT_OFF = 0
EFFECT_DIRECT = 1


def find_device():
    devs = hid.enumerate(VID, PID)
    for d in devs:
        if d.get("usage_page") == USAGE_PAGE and d.get("usage") == USAGE:
            return d
    return devs[0] if devs else None


def xfer(dev, payload, expect_reply=True):
    buf = [0x00] + list(payload)
    buf += [0] * (REPORT + 1 - len(buf))
    dev.write(buf[: REPORT + 1])
    if not expect_reply:
        return None
    try:
        return dev.read(REPORT, 700)
    except Exception:
        return None


def get_u16(dev, subcmd):
    """Ответ на GET: [cmd][subcmd][payload...] -> payload начинается с байта 2."""
    r = xfer(dev, [CMD_CUSTOM_GET_VALUE, subcmd])
    return (r[2] | (r[3] << 8)) if r and len(r) >= 4 else None


def get_args(dev, subcmd, extra=()):
    r = xfer(dev, [CMD_CUSTOM_GET_VALUE, subcmd] + list(extra))
    return r[2:] if r else None


def selftest(dev):
    ok = True

    print("== raw HID ==")
    r = xfer(dev, [CMD_FW_PROTOCOL])
    if r and r[0] == CMD_FW_PROTOCOL:
        print(f"  vendor fw protocol : 0x{r[1]:02X}   -> raw HID works")
    else:
        print("  vendor fw protocol : no reply    -> raw HID broken")
        ok = False

    print("== VIA ==")
    r = xfer(dev, [CMD_GET_PROTOCOL_VERSION])
    if r and r[0] == CMD_GET_PROTOCOL_VERSION:
        via = (r[1] << 8) | r[2]
        print(f"  VIA protocol       : {via}" + ("" if via >= 9 else "   (OpenRGB needs >= 9)"))
        ok = ok and via >= 9
    else:
        print("  VIA protocol       : no reply")
        ok = False

    print("== Vial / VialRGB ==")
    r = xfer(dev, [CMD_VIAL, 0x00])
    if r is None:
        print("  Vial               : no reply (firmware without Vial)")
        return False

    vial_ver = r[0] | (r[1] << 8) | (r[2] << 16) | (r[3] << 24)
    uid = "".join(chr(c) if 32 <= c < 127 else "." for c in r[4:12])
    flag = r[12]
    print(f"  Vial protocol      : {vial_ver}")
    print(f"  UID                : {uid}")
    print(f"  VialRGB present    : {'yes' if flag & 1 else 'NO'}")

    if not (vial_ver >= 4 and flag & 1):
        print("  -> OpenRGB will NOT accept this device")
        return False

    a = get_args(dev, VIALRGB_GET_INFO)
    print(f"  VialRGB protocol   : {a[0] | (a[1] << 8)}")
    print(f"  max brightness     : {a[2]}")
    print(f"  LED count          : {get_u16(dev, VIALRGB_GET_NUM_LEDS)}")

    s = get_args(dev, VIALRGB_GET_SUPPORTED, [0, 0])
    ids = [s[i] | (s[i + 1] << 8) for i in range(0, 30, 2)]
    ids = [i for i in ids if i != 0xFFFF]
    print(f"  supported effects  : {ids}")

    m = get_args(dev, VIALRGB_GET_MODE)
    print(f"  current mode       : {m[0] | (m[1] << 8)}")

    print()
    print("RESULT: " + ("OK - OpenRGB will detect this device" if ok else "problems, see above"))
    return ok


def set_mode(dev, mode):
    xfer(dev, [CMD_CUSTOM_SET_VALUE, VIALRGB_SET_MODE, mode & 0xFF, mode >> 8, 0x7F, 0, 255, 200],
         expect_reply=False)


def send_leds(dev, colors):
    i = 0
    while i < len(colors):
        chunk = colors[i : i + 9]
        payload = [CMD_CUSTOM_SET_VALUE, VIALRGB_DIRECT_FASTSET, i & 0xFF, i >> 8, len(chunk)]
        for (h, s, v) in chunk:
            payload += [h, s, v]
        xfer(dev, payload, expect_reply=False)
        i += len(chunk)


def rgb_to_hsv(r, g, b):
    mx, mn = max(r, g, b), min(r, g, b)
    df = mx - mn
    if df == 0:
        h = 0
    elif mx == r:
        h = (60 * ((g - b) / df) + 360) % 360
    elif mx == g:
        h = (60 * ((b - r) / df) + 120) % 360
    else:
        h = (60 * ((r - g) / df) + 240) % 360
    s = 0 if mx == 0 else int(df * 255 / mx)
    return int(h * 256 / 360) & 0xFF, s, mx


def dump_leds(dev, n):
    print(f"{'idx':>3} {'x':>4} {'y':>3} {'flags':>5} {'row':>4} {'col':>4}  zone")
    for i in range(n):
        a = get_args(dev, VIALRGB_GET_LED_INFO, [i & 0xFF, i >> 8])
        if not a:
            print(f"{i:>3}  -- no reply --")
            continue
        row, col = a[3], a[4]
        zone = "knob ring" if row != 0xFF and col == 15 else ("keyboard" if row != 0xFF else "unmapped")
        print(f"{i:>3} {a[0]:>4} {a[1]:>3} {a[2]:>5} {row:>4} {col:>4}  {zone}")


def dump_battery(dev):
    a = get_args(dev, VIALRGB_GET_BATTERY)
    if not a:
        print("no answer (firmware < v9?)")
        return 1
    mv = a[0] | (a[1] << 8)
    pct = a[2]
    usb = a[3]
    chg = a[4]
    print(f"voltage      : {mv} mV")
    print(f"percentage   : {pct} %")
    print(f"usb power    : {'yes' if usb else 'no'}")
    print(f"charging pin : {'charging' if chg else 'not charging'}")
    if mv == 0:
        print("-> модуль ещё не отдал напряжение (battery_get_voltage() == 0)")
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--dump-leds", action="store_true")
    ap.add_argument("--battery", action="store_true")
    ap.add_argument("--rainbow", action="store_true")
    ap.add_argument("--color", type=str, help="RRGGBB")
    ap.add_argument("--off", action="store_true")
    ap.add_argument("--hold", type=float, default=0.0, help="seconds to keep animating")
    args = ap.parse_args()

    info = find_device()
    if not info:
        print("Keyboard not found (VID 0x362D, PID 0x1700).")
        print("Check: cable connected, switch in USB mode.")
        return 1

    print(f"device: {info.get('product_string')}")
    dev = hid.device()
    dev.open_path(info["path"])

    try:
        if args.selftest:
            return 0 if selftest(dev) else 2

        if args.dump_leds:
            dump_leds(dev, get_u16(dev, VIALRGB_GET_NUM_LEDS) or 90)
            return 0

        if args.battery:
            return dump_battery(dev)

        if args.off:
            set_mode(dev, EFFECT_OFF)
            print("backlight off")
            return 0

        n = get_u16(dev, VIALRGB_GET_NUM_LEDS) or 88
        set_mode(dev, EFFECT_DIRECT)

        if args.color:
            rgb = bytes.fromhex(args.color)
            h, s, v = rgb_to_hsv(*rgb)
            send_leds(dev, [(h, s, v)] * n)
            print(f"all {n} LEDs -> {args.color}")
        elif args.rainbow:
            frames, start = 0, time.time()
            while True:
                shift = frames * 8
                send_leds(dev, [((i * 256 // n + shift) & 0xFF, 255, 180) for i in range(n)])
                frames += 1
                if args.hold and time.time() - start > args.hold:
                    break
                time.sleep(1 / 30)
            print(f"rainbow: {frames} frames")
        else:
            ap.print_help()
    finally:
        dev.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
