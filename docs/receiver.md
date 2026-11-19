# 2.4 GHz receiver investigation

Tested with the keyboard working through its receiver and the keyboard's USB cable disconnected.

| Device | VID:PID | Collection |
|---|---|---|
| Keyboard over USB | `362D:1700` | QMK Raw HID `FF60:0061` |
| Wireless Link receiver | `362D:D101` | Raw HID `FF60:0061`, bridge `008C:0001` |

The LKBT51 bridge is supported from v1.1.0. The receiver sends Raw HID reports to the keyboard MCU using event `0x92`, and the keyboard returns replies in LKBT51 command `0x93`. VIA and VialRGB both use this route. The firmware keeps USB replies on USB when the originating request came in over USB.

Before these two paths were added, a receiver connected through 2.4 GHz typed normally but did not return VIA or VialRGB replies. After the change the real receiver returned VIA protocol 12, VialRGB protocol 1, the effects list, and 90 LED entries. The final two LED entries drive the battery gauge.

The standard Vial identity request `FE 00` still produces no response through the receiver. The OpenRGB plugin queries VialRGB directly and registers separate key and encoder-ring zones. The stock OpenRGB VialRGB detector expects the identity response, so the plugin is needed over 2.4 GHz.

Use the USB definition for `362D:1700` and the receiver definition for `362D:D101`. Bootloader and factory-test commands are not routed through the receiver.
