# WhateverDSP project makefile include

ifeq ($(TARGET), )
$(error Please set the target name)
endif

ifeq ($(BOARD), )
$(error Please set the board type)
endif

WDSP_PATH := $(dir $(lastword $(MAKEFILE_LIST)))
CONFIG_FILE ?= $(wildcard ./config.ini)
BUILD_ROOT ?= .wdsp

ifneq ($(CONFIG_FILE),)
export USER_CONFIG = $(shell realpath --relative-to "$(abspath $(WDSP_PATH))" "$(abspath $(CONFIG_FILE))")
endif

LIB_BUILD_ROOT := $(shell realpath --relative-to "$(abspath $(WDSP_PATH))" "$(abspath $(BUILD_ROOT))")

include $(WDSP_PATH)/buildvars.mk
include $(WDSP_PATH)/init.mk

C_INCLUDES = \
-I $(CONFIG_DIR) \
-I $(WDSP_PATH)/includes/libwdsp \
-I $(WDSP_PATH)/boards/$(BOARD) \
-I $(WDSP_PATH)/cores/$(CORE)

include $(WDSP_PATH)/boards/$(BOARD)/board.mk
include $(WDSP_PATH)/cores/$(CORE)/core.mk

# Reset default goal so it doesn't get overriden by rules in the included files
.DEFAULT_GOAL :=

.PHONY: all
all: libwdsp $(TARGET) $(TARGET).bin $(TARGET).hex size

.PHONY: size
size: $(TARGET)
	$(SZ) $<

.PHONY: libwdsp
libwdsp: export BUILD_ROOT = $(LIB_BUILD_ROOT)
libwdsp:
	$(MAKE) -C $(WDSP_PATH)

$(LIB_NAME).a: libwdsp

$(TARGET).hex: $(TARGET)
	$(HEX) $< $@

$(TARGET).bin: $(TARGET)
	$(BIN) $< $@

$(TARGET): $(LIB_NAME).a

.PHONY: libclean
libclean: export BUILD_ROOT = $(LIB_BUILD_ROOT)
libclean:
	$(MAKE) -C $(WDSP_PATH) clean
	rm -f $(TARGET) $(TARGET).bin $(TARGET).hex $(TARGET).map

.PHONY: clean
clean: libclean

