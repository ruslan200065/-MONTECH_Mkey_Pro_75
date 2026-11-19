# MONTECH MKey PRO 75 plugin for OpenRGB

The Windows x64 DLL targets **OpenRGB 1.0rc3, plugin API 4, Qt 5.15**. It adds one device with a keyboard matrix (81 LEDs) and a separate encoder ring (7 LEDs). It prefers USB when both USB and the paired 2.4 GHz receiver are available. The battery gauge remains firmware-controlled.

1. Install firmware v1.1.0 or later and connect via USB or 2.4 GHz.
2. Add `OpenRGBMontech24G.dll` in OpenRGB **Settings → Plugins** or place it in `%APPDATA%\OpenRGB\plugins`, then restart OpenRGB.
3. If you previously added custom `362D:1700` or `362D:D101` entries under **QMK VialRGB Devices**, remove those entries to avoid duplicate detection and the receiver's Vial identity timeout.

The controller queries VialRGB directly. The wireless receiver answers lighting queries but does not return the separate Vial identity query used by the built-in detector.

## Build

Use an x64 MSVC developer command prompt with Qt 5.15 for MSVC on PATH. Copy the x64 `hidapi.lib` from `dependencies/hidapi-win/x64` in the pinned OpenRGB revision below to `upstream/hidapi/hidapi.lib`, then run:

```cmd
qmake Montech24G.pro -spec win32-msvc
nmake
```

The plugin uses the host installation's `hidapi.dll` at runtime. The vendored OpenRGB API and controller sources are based on [commit 6fbcf62](https://github.com/CalcProgrammer1/OpenRGB/tree/6fbcf62d7694e7b92fd0a5884b40b92984fbd1b0). The controller's non-virtual thread stop helper was added so the plugin can quiesce its worker before destroying the HID handle. See [upstream/LICENSE](upstream/LICENSE).
