DST_NAME = airkiss

SRCS =  main.c \
	ap_config.c \
	board.c

EXTRACFLAGS += -I./src
EXTRACFLAGS += -I./tencent/incl
EXTRALIBS += ./tencent/libs/libairkiss_aes.a

LDSCRIPT = $(TOOLCHAIN_DIR)/$(ARCH_NAME-y).ld

include $(TOOLCHAIN_DIR)/targets.mk
include $(TOOLCHAIN_DIR)/rules.mk
