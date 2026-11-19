#!/usr/bin/env python3
"""
MKey PRO 75 - ambient / game lighting.

Samples the screen and pushes the colors to the keyboard in real time
(VialRGB direct mode). Works with any game or video - no per-game support needed.

    python tools/mkey_ambient.py --mode screen     # screen edge, per-key colors
    python tools/mkey_ambient.py --mode avg        # single average color
    python tools/mkey_ambient.py --mode music      # reacts to system volume/level
    python tools/mkey_ambient.py --mode screen --fps 30 --smooth 0.35

Requires: pip install hidapi pillow
"""

import argparse
import sys
import time

try:
    import hid
except ImportError:
    print("hidapi is required:  pip install hidapi")
    sys.exit(1)

try:
    from PIL import ImageGrab, Image
except ImportError:
    ImageGrab = None

VID = 0x362D
PID = 0x1700
USAGE_PAGE = 0xFF60
USAGE = 0x61
REPORT = 32

CMD_CUSTOM_SET_VALUE = 0x07
CMD_CUSTOM_GET_VALUE = 0x08
VIALRGB_GET_INFO = 0x40
VIALRGB_GET_NUM_LEDS = 0x43
VIALRGB_GET_LED_INFO = 0x44
VIALRGB_SET_MODE = 0x41
VIALRGB_DIRECT_FASTSET = 0x42
EFFECT_DIRECT = 1


def find_device():
    for d in hid.enumerate(VID, PID):
        if d.get("usage_page") == USAGE_PAGE and d.get("usage") == USAGE:
            return d
    return None


def xfer(dev, payload, reply=True):
    buf = [0x00] + list(payload)
    buf += [0] * (REPORT + 1 - len(buf))
    dev.write(buf[: REPORT + 1])
    if reply:
        try:
            return dev.read(REPORT, 300)
        except Exception:
            return None
    return None


def query(dev, subcmd, extra=()):
    r = xfer(dev, [CMD_CUSTOM_GET_VALUE, subcmd] + list(extra))
    return r


def led_map(dev, n):
    """Returns list of (x, y) in QMK 224x64 space for every LED."""
    pts = []
    for i in range(n):
        r = query(dev, VIALRGB_GET_LED_INFO, [i & 0xFF, i >> 8])
        if r:
            pts.append((r[0], r[1]))
        else:
            pts.append((i * 224 // max(n, 1), 32))
    return pts


def set_direct(dev):
    xfer(dev, [CMD_CUSTOM_SET_VALUE, VIALRGB_SET_MODE, EFFECT_DIRECT, 0, 0x7F, 0, 255, 200], reply=False)


def send_leds(dev, cols):
    i = 0
    while i < len(cols):
        chunk = cols[i : i + 9]
        payload = [CMD_CUSTOM_SET_VALUE, VIALRGB_DIRECT_FASTSET, i & 0xFF, i >> 8, len(chunk)]
        for (h, s, v) in chunk:
            payload += [h, s, v]
        xfer(dev, payload, reply=False)
        i += len(chunk)
    # drain replies so the IN queue does not fill up
    try:
        while dev.read(REPORT, 1):
            pass
    except Exception:
        pass


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
    return int(h * 256 / 360) & 0xFF, s, min(mx, 255)


def frame_screen(pts, band=0.35):
    """Samples the bottom band of the screen, maps it onto the keyboard."""
    img = ImageGrab.grab()
    w, h = img.size
    y0 = int(h * (1.0 - band))
    img = img.crop((0, y0, w, h)).resize((224, 64), Image.BOX)
    px = img.load()
    out = []
    for (x, y) in pts:
        c = px[min(x, 223), min(y, 63)]
        out.append(rgb_to_hsv(c[0], c[1], c[2]))
    return out


def frame_avg(pts):
    img = ImageGrab.grab().resize((8, 8), Image.BOX)
    px = img.load()
    r = g = b = 0
    for yy in range(8):
        for xx in range(8):
            c = px[xx, yy]
            r += c[0]
            g += c[1]
            b += c[2]
    c = (r // 64, g // 64, b // 64)
    return [rgb_to_hsv(*c)] * len(pts)


def smooth(prev, cur, k):
    if prev is None:
        return cur
    out = []
    for (ph, ps, pv), (ch, cs, cv) in zip(prev, cur):
        # hue is circular, but for ambient light linear blending is fine
        out.append((
            int(ph + (ch - ph) * k),
            int(ps + (cs - ps) * k),
            int(pv + (cv - pv) * k),
        ))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", choices=["screen", "avg"], default="screen")
    ap.add_argument("--fps", type=float, default=25.0)
    ap.add_argument("--smooth", type=float, default=0.35, help="0 = no smoothing, 1 = instant")
    ap.add_argument("--band", type=float, default=0.35, help="fraction of the screen to sample")
    ap.add_argument("--brightness", type=int, default=200)
    args = ap.parse_args()

    if ImageGrab is None:
        print("Pillow is required:  pip install pillow")
        return 1

    info = find_device()
    if not info:
        print("Keyboard not found (VID 0x362D, PID 0x1700), USB mode required.")
        return 1

    dev = hid.device()
    dev.open_path(info["path"])

    r = query(dev, VIALRGB_GET_INFO)
    if not r:
        print("No VialRGB answer - flash firmware v4+ first.")
        return 2

    n = (query(dev, VIALRGB_GET_NUM_LEDS) or [88, 0])[0] | ((query(dev, VIALRGB_GET_NUM_LEDS) or [0, 0])[1] << 8)
    if not n:
        n = 88
    print(f"LEDs: {n}, mode: {args.mode}, fps: {args.fps}")

    pts = led_map(dev, n)
    set_direct(dev)

    delay = 1.0 / max(args.fps, 1)
    prev = None
    frames = 0
    try:
        while True:
            t0 = time.time()
            cur = frame_screen(pts, args.band) if args.mode == "screen" else frame_avg(pts)
            cur = [(h, s, min(v, args.brightness)) for (h, s, v) in cur]
            prev = smooth(prev, cur, args.smooth)
            send_leds(dev, prev)
            frames += 1
            dt = time.time() - t0
            if dt < delay:
                time.sleep(delay - dt)
            if frames % 100 == 0:
                print(f"  {frames} frames, {dt*1000:.0f} ms/frame")
    except KeyboardInterrupt:
        print(f"\nstopped after {frames} frames")
    finally:
        dev.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
