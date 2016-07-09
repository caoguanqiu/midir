# Xiaomi inc 
#

###########################################################################
# global environment variable
###########################################################################
ifeq ($(CONFIG_CPU_MC200),y)
	PLATFORM := mc200
endif

ifeq ($(CONFIG_CPU_MW300),y)
	PLATFORM := mw300
endif

ifeq ($(XIP),1)
	ifeq ($(CONFIG_ENABLE_MCU_PM3),y)
		XIP-PM3-OPT := -xip-pm3
	else
		XIP-PM3-OPT := -xip
	endif
else
	XIP-PM3-OPT :=
endif

###########################################################################
#define project name & library name
###########################################################################
miio_app_name := miio_app

#define target 
exec-y += $(miio_app_name)

###########################################################################
# define c flags for this project
###########################################################################
EXTRACFLAGS := 	-I $(d)/miio_common/Source						\
				-I $(d)/miio_common/Source/include				\
				-I $(d)/miio_common/Source/include/lib 			\
				-I $(d)/miio_common/Source/include/ot

EXTRACFLAGS += 	-DMIIO_ADDON_API=0			\
				-DBUTTON_COUNT=$(BUTTON_COUNT) \
				-DUSER=$(addsuffix \", $(addprefix \", $(USER)))

ifneq ($(JENKINS_BUILD),)
EXTRACFLAGS += -DJENKINS_BUILD=$(JENKINS_BUILD)
endif

ifeq ($(RELEASE),1)
EXTRACFLAGS += 	-DAPPCONFIG_DEBUG_ENABLE=0 -DRELEASE=1
else
EXTRACFLAGS += 	-DAPPCONFIG_DEBUG_ENABLE=1
endif

ifeq ($(DRIVER_IN_RAM1), 1)
EXTRACFLAGS += -DDRIVER_IN_RAM1=1
endif

ifeq ($(FORCE_CFG_IN_PSM), 1)
EXTRACFLAGS += -DFORCE_CFG_IN_PSM
endif

ifeq ($(ENABLE_AIRKISS), 1)
EXTRACFLAGS += -DENABLE_AIRKISS=1	
$(miio_app_name)-prebuilt-libs-y += $(d)/miio_common/Source/static_lib/airkiss/libairkiss_aes.a
endif

ifeq ($(EASY_BACKTRACE),1)
EXTRACFLAGS += -fno-omit-frame-pointer -DEASY_BACKTRACE=1 
endif

ifeq ($(MW300_68PIN),1)
EXTRACFLAGS += -DMW300_68PIN=1 
endif

ifeq ($(MIIO_COMMANDS), 1)
EXTRACFLAGS += -DMIIO_COMMANDS=1
endif

ifeq ($(MIIO_COMMANDS), 2)
EXTRACFLAGS += -DMIIO_COMMANDS=1
EXTRACFLAGS += -DMIIO_COMMANDS_DEBUG=1
endif

ifeq ($(MIIO_BLE), 1)
EXTRACFLAGS += -DMIIO_BLE=1
EXTRACFLAGS += -DWM_IOT_PLATFORM=TRUE
endif


#define c flags for target
$(miio_app_name)-cflags-y += $(EXTRACFLAGS)

###########################################################################
#common obj list
###########################################################################
RAW_SRCS_OPEN = main.c 						\
		misc/led_indicator.c 				\
		misc/miio_cli.c 					\
		util/dump_hex_info.c 				\
		util/util.c							\
		src_lib/jsmn/jsmn_api.c				\
		src_lib/jsmn/jsmn.c					\
		src_lib/kernel_crc8.c 				\
		src_lib/arch_net.c					\
		src_lib/arch_os.c					\
		src_lib/arch_io.c					\
		src_lib/button_detect_framework.c 	\
		src_lib/miio_chip_rpc.c				\
		src_lib/main_extension.c			\
		src_lib/appln_dbg.c					\
		src_lib/call_on_worker_thread.c 	\
		src_lib/mdns_helper.c 				\
		src_lib/uap_diagnosticate.c			\
		src_lib/crash_trace.c				\
		src_lib/miio_up_manager.c			\
		src_lib/power_mgr_helper.c			\
		src_lib/text_on_uart.c				\
		src_lib/miio_command.c				\
		src_lib/miio_command_functions.c 	\
		src_lib/miio_down_manager.c			\
		src_lib/third_party_mcu_ota.c 		\
		src_lib/xmodem.c					\
		src_lib/ota.c						\
		src_lib/config_router.c				\
		src_lib/wlan_unicast_scan.c			\
		src_lib/wifi_framework_callback.c 	\
		smt_cfg/airkiss_config.c        	\
		src_lib/local_timer.c				\
		device_app/miIO_app_$(BOARD).c   \
                device_app/communication.c

		
ifeq ($(CONFIG_CPU_MW300), y)
else 
ifeq ($(CONFIG_CPU_MC200), y)
RAW_SRCS_OPEN += util/dht11.c			\
				util/rgb.c

ifeq ($(CONFIG_WiFi_8801), y)
RAW_SRCS_OPEN += src_lib/psm_upgrade_util.c
endif

endif
endif

ifeq ($(MIIO_BLE), 1)
RAW_SRCS_OPEN += ble/ble.c
RAW_SRCS_OPEN += ble/mi_band.c
endif

SRCS_OPEN = $(addprefix miio_common/Source/,$(RAW_SRCS_OPEN))

$(miio_app_name)-objs-y += $(SRCS_OPEN)

#add libm.a for math
#$(miio_app_name)-prebuilt-libs-y += -lm

$(miio_app_name)-linkerscript-y := $(d)/toolchains/gnu/$(PLATFORM)$(XIP-PM3-OPT).ld 

LIBMIIO_A_FILE := $(d)/miio_common/Source/libmiio.a
EXIST := $(wildcard $(LIBMIIO_A_FILE))

#do not move this section
ifeq ($(EXIST), )
# inlcude libmiio.mk to build libmiio
	subdir-y += libmiio.mk
	$(miio_app_name)-select-libs-y += $(miio_lib_name)
else
	$(miio_app_name)-prebuilt-libs-y += $(LIBMIIO_A_FILE)
endif

pre_check_libmiio:
	@echo "################# can add some pre check operation here "
	@echo "$($(miio_app_name)-select-libs-y)"
	@echo "$($(miio_lib_name)-cflags-y)"

#########################################################################
# define variable for generate sdk
#########################################################################

MRVL_SDK_BASE := $(d)/..
MIIO_SDK_DIR := $(d)/../miio_sdk
MIIO_APP_DIR := $(d)
SRCS_TO_COPY := $(addprefix $(d)/, $(SRCS_OPEN))


define miio_app_postprocess
$(miio_app_name): pre_check_libmiio $($(miio_app_name)-output-dir-y)/$(miio_app_name).bin
	@echo "make miio_app XIP is "$(XIP-PM3-OPT)
	
ifeq ($(EXIST),)
	@echo "will generate sdk "
	@mkdir -p $(MIIO_SDK_DIR)/miio_app/miio_common/Source
	
	@cp -r $(MRVL_SDK_BASE)/boot2 $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/build $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/docs $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/phone_apps $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/sample_apps $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/sdk $(MIIO_SDK_DIR)
	@rm -f $(MIIO_SDK_DIR)/sdk/tools/bin/Linux/*
	@cp -r $(MRVL_SDK_BASE)/wifi-firmware $(MIIO_SDK_DIR)


	@cp -r $(MRVL_SDK_BASE)/LICENSE.pdf $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/Makefile $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/README $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/README.FIRST $(MIIO_SDK_DIR)
	@cp -r $(MRVL_SDK_BASE)/RELEASE_NOTES.txt $(MIIO_SDK_DIR)

	@cp -r $(MIIO_APP_DIR)/toolchains $(MIIO_SDK_DIR)/miio_app
	@cp -r $(MIIO_APP_DIR)/build.mk $(MIIO_SDK_DIR)/miio_app
	@cp -r $($(miio_app_name)-output-dir-y)/../libs/libmiio.a $(MIIO_SDK_DIR)/miio_app/miio_common/Source

	@cp -r $(MIIO_APP_DIR)/miio_common/Source/include $(MIIO_SDK_DIR)/miio_app/miio_common/Source
	@cp -r $(MIIO_APP_DIR)/miio_common/Source/static_lib $(MIIO_SDK_DIR)/miio_app/miio_common/Source
	@cp --parents $(SRCS_TO_COPY) $(MIIO_SDK_DIR)

endif

endef
