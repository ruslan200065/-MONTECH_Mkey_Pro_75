# MONTECH MKey PRO 75 — community QMK firmware

[Русская версия](README.ru.md) · [Downloads and release notes](https://github.com/ruslan200065/-MONTECH_Mkey_Pro_75/releases)

Buildable firmware for the **ANSI MONTECH MKey PRO 75**, based on Keychron's QMK wireless branch and adapted from the Lemokey P1 Pro. The release contains a ready-to-flash WB32 binary. The source includes the keyboard definition, both keymaps and the changes to the shared wireless layer.

| Feature | USB | 2.4 GHz | Bluetooth |
|---|---|---|---|
| Keyboard, encoder, onboard lighting and OS switch | Yes | Yes | Yes |
| VIA remapping and lighting controls | Yes | Experimental definition only | — |
| OpenRGB live control | Yes | Receiver transport unverified | — |

### What's included

- The MONTECH matrix column order, 12-phase SNLED27351 configuration and factory LED wiring for the keys and encoder ring. See [hardware notes](docs/hardware-proof.md).
- Working USB / Bluetooth / 2.4 GHz selector and Win/Mac switch, with independent base and Fn layers. In Mac mode, the top row controls media and lighting, and Fn restores the function keys. The encoder's zoom uses Ctrl on Windows and Command on Mac.
- Three encoder modes (volume, zoom, lighting effect), ring feedback and EEPROM storage for the selected mode and ring on/off setting. Fn + rotation controls brightness.
- 81 key LEDs and 7 ring LEDs, plus six physical battery segments driven by two RGB entries. The segments fill from the bottom. During charging the filled segments stay on and the remaining ones blink. Fn+Esc turns off the main lighting and side gauge together. Fn+B briefly shows charge on the number row.
- Battery-backed wireless operation with a 60-second backlight idle timeout, a 128/255 PWM cap and automatic WB32 STOP sleep. A key press is the wake source. Disconnecting from a host can turn lighting off sooner.
- VialRGB Direct mode and effect selection over USB, packet acknowledgements for streaming, independent ring coordinates and per-key VIA colour controls. Live per-key colours are volatile. Optional Python tools provide screen-reactive lighting, HID diagnostics and battery segment mapping.

### Flash the release

1. Back up your VIA layout. Download the current **ANSI** `.bin` from Releases.
2. Open the binary in [QMK Toolbox](https://qmk.fm/toolbox) and set the keyboard's connection switch to **USB**.
3. Unplug the cable. Hold **Esc** while reconnecting it to enter WB32 DFU. Release Esc, then flash and wait for completion. Reconnect normally.
4. If VIA retains old layers and the Mac layout is missing, export any custom layout, then use **Fn+Del in Win mode** to reset EEPROM to this build's defaults. This clears user settings.

The matching factory **QMK BIN_ANSI** and the official flashing instructions are on the [MONTECH download page](https://www.montechpc.com/mkey-pro). This community binary is for ANSI hardware.

### VIA and OpenRGB

- **VIA via USB.** In [VIA](https://usevia.app), turn on the Design tab in Settings. Load [the USB V3 definition](via/mkey_pro_75_ansi_usb.json) as a draft, then authorize the keyboard on Configure. Layers 0/1 are Windows and its Fn layer, 2/3 are Mac and its Fn layer.
- **OpenRGB via USB.** In a build with the **QMK VialRGB Devices** detector, add VID `362D`, PID `1700`, then rescan or reconnect the keyboard. Select **Direct** to stream colours, or choose a supported onboard effect. The encoder ring appears as a separate column in the LED map. [OpenRGB source](https://gitlab.com/CalcProgrammer1/OpenRGB)
- **2.4 GHz.** [The experimental VIA definition](via/mkey_pro_75_ansi_24g_experimental.json) identifies the receiver as `362D:D101`. The tested receiver exposed HID interfaces but did not answer VIA or VialRGB queries. The definition describes the layout. Working 2.4 GHz configuration and OpenRGB require a verified command path through the receiver. See [receiver test results](docs/receiver.md).

### Build from source

On Windows with Git and Docker Desktop running Linux containers, use PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup.ps1
powershell -ExecutionPolicy Bypass -File scripts/build.ps1
```

The scripts fetch the pinned Keychron QMK revision and its dependencies, sync the port and overlays and write the compiled binary to `firmware/montech_mkey_pro_75_via.bin`. QMK embeds the build date, so a local rebuild can differ in bytes. The release also provides the complete source tree used by the build.

### Optional utilities

```powershell
python -m pip install hidapi pillow
python tools/mkey_light.py --selftest
python tools/mkey_light.py --dump-leds
python tools/mkey_ambient.py --mode screen
python tools/batmap.py
```

Use a USB cable for these utilities. Close other RGB clients while using the mapping tool.

### Credits and license

This is a community port. Thanks to MONTECH for the hardware and reference downloads, the Keychron/Lemokey and QMK contributors for the underlying firmware, and [caloutw](https://github.com/caloutw/MONTECH-Mkey-Pro-75-QMK-Plus) for the earlier MKey PRO 75 community work that helped inform the port. The port is distributed under [GPL-2.0](LICENSE). Upstream component notices are retained in the source bundle.
