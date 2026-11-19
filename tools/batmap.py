#!/usr/bin/env python3
"""Map the six battery indicator channels. Run: python tools/batmap.py."""

import json
from pathlib import Path
import sys

VID, PID = 0x362D, 0x1700
USAGE_PAGE, USAGE = 0xFF60, 0x61
REPORT = 32
CMD_SET, CMD_GET = 0x07, 0x08
MODE, DIRECT_FASTSET, FILL_ALL, BAT_SUSPEND = 0x41, 0x42, 0x43, 0x44
EFFECT_DIRECT, EFFECT_SOLID = 1, 2
BATTERY_LEDS = (88, 89)
# Fully saturated HSV at the three QMK hue anchors selects one RGB output.
CHANNELS = (("R", 0), ("G", 85), ("B", 170))
RESULT_FILE = Path(__file__).with_name("batmap_result.json")


def exchange(dev, command, subcommand, *args, verify_echo=False):
    packet = bytes([command, subcommand, *args]).ljust(REPORT, b"\0")
    if dev.write(b"\0" + packet) != REPORT + 1:
        raise OSError("Не удалось отправить HID-пакет.")
    for _ in range(8):
        reply = bytes(dev.read(REPORT, 500))
        if not reply:
            raise OSError(f"Нет ответа на HID-команду {command:02X}/{subcommand:02X}.")
        if len(reply) == REPORT and reply[:2] == packet[:2]:
            if not verify_echo or reply == packet:
                return reply[2:]
    raise OSError(f"Неожиданный ответ на HID-команду {command:02X}/{subcommand:02X}.")


def set_channel(dev, led, hue, value):
    exchange(dev, CMD_SET, DIRECT_FASTSET, led, 0, 1, hue, 255, value)


def read_position():
    while True:
        answer = input("Позиция снизу вверх [1–6, 0, q]: ").strip().lower()
        if answer == "q":
            return None
        if answer in ("0", "1", "2", "3", "4", "5", "6"):
            return int(answer)
        print("Введите одну цифру от 1 до 6, 0 если ничего не горит или q для выхода.")


def measure(dev, results):
    previous_mode = exchange(dev, CMD_GET, MODE)[:6]
    exchange(dev, CMD_SET, BAT_SUSPEND, 1, verify_echo=True)
    try:
        # Use a known nonzero brightness even if the saved brightness was zero.
        exchange(dev, CMD_SET, MODE, EFFECT_SOLID, 0, 127, 0, 255, 255)
        exchange(dev, CMD_SET, MODE, EFFECT_DIRECT, 0)
        exchange(dev, CMD_SET, FILL_ALL, 0, 0, 0, verify_echo=True)

        for led in BATTERY_LEDS:
            for channel, hue in CHANNELS:
                print(f"LED {led}, канал {channel}")
                set_channel(dev, led, hue, 255)
                try:
                    position = read_position()
                finally:
                    set_channel(dev, led, hue, 0)
                if position is None:
                    return
                results[f"{led}:{channel}"] = position
    finally:
        try:
            exchange(dev, CMD_SET, MODE, *previous_mode)
        finally:
            exchange(dev, CMD_SET, BAT_SUSPEND, 0, verify_echo=True)


def main():
    try:
        import hid
    except ImportError:
        print("Установите hidapi: python -m pip install hidapi", file=sys.stderr)
        return 1

    info = next(
        (device for device in hid.enumerate(VID, PID)
         if device.get("usage_page") == USAGE_PAGE and device.get("usage") == USAGE),
        None,
    )
    if info is None:
        print("Клавиатура не найдена. Подключите её кабелем в режиме USB.")
        return 1

    print("Считайте шесть позиций на боковой шкале: 1 — нижняя, 6 — верхняя.")
    print("На каждом шаге горит один канал. Введите его позицию, 0 или q.")
    results = {}
    dev = hid.device()
    status = 0
    try:
        dev.open_path(info["path"])
        measure(dev, results)
    except (KeyboardInterrupt, EOFError):
        print("\nПроверка прервана.")
    except OSError as error:
        print(f"Ошибка HID: {error}", file=sys.stderr)
        status = 1
    finally:
        dev.close()

    if results:
        RESULT_FILE.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
        print(f"Сохранено {len(results)} из 6 ответов: {RESULT_FILE}")
    if len(results) == 6:
        positions = sorted(results.values())
        if positions == list(range(1, 7)):
            print("Карта полная: каждой позиции соответствует один канал.")
        else:
            print("В карте есть пропуски или повторы. Проверьте их перед правкой прошивки.")
    return status


if __name__ == "__main__":
    sys.exit(main())
