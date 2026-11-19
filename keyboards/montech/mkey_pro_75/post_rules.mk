include keyboards/lemokey/common/wireless/wireless.mk

EXTRALDFLAGS += -Wl,--wrap=snled27351_set_color,--wrap=snled27351_set_color_all

ifeq ($(strip $(VIA_ENABLE)),yes)
    EXTRALDFLAGS += -Wl,--wrap=raw_hid_send
endif
