include keyboards/lemokey/common/wireless/wireless.mk

EXTRALDFLAGS += -Wl,--wrap=snled27351_set_color,--wrap=snled27351_set_color_all
