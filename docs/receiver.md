# 2.4 GHz receiver investigation

Tested with the keyboard working through its receiver and the keyboard's USB cable disconnected.

| Device | VID:PID | Collection |
|---|---|---|
| Keyboard over USB | `362D:1700` | QMK Raw HID `FF60:0061` |
| Wireless Link receiver | `362D:D101` | Raw HID `FF60:0061`, bridge `008C:0001` |

The receiver's Raw HID collection accepted report writes but returned no replies to VIA protocol version, uptime, Vial ID, VialRGB info or LED count. The bridge collection has 32-byte payloads and Windows reports output ID `B2` and input IDs `B1` and `54`. With output report ID `B2`, delivery to the USB interface succeeded but version, state and VIA queries returned no input reply. Attempts to read its declared feature reports also returned errors.

The [Keychron bridge investigation](https://github.com/Tymon3310/keychron-vial/blob/main/docs/launcher/bridge-dongle-protocol.md) describes a different bridge flow with unnumbered reports. On the pinned upstream revision for this port, the LKBT51 dispatch forwards host LED status and radio/DFU events. It contains no existing VIA/VialRGB round-trip over the radio. The available evidence does not establish whether this receiver can support one with a different bridge protocol.

The experimental JSON records the receiver VID/PID and the same physical layout. VIA and OpenRGB require a working request/reply transport before they can configure the keyboard over 2.4 GHz. Use USB for configuration and testing lighting until that transport is available.
