# WhateverDSP project makefile include

ifeq ($(TARGET), )
$(error Please set the target name)
endif

ifeq ($(BOARD), )
$(error Please set the board type)
endif

WDSP_PATH := $(dir $(lastword $(MAKEFILE_LIST)))
CONFIG_FILE ?= $(wildcard ./config.ini)

export USER_CONFIG ?= $(shell realpath --relative-to "$(abspath $(WDSP_PATH))" "$(abspath $(CONFIG_FILE))")
export BUILD_ROOT ?= $(shell realpath --relative-to "$(abspath $(WDSP_PATH))" "$(abspath .wdsp)")

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
libwdsp:
	$(MAKE) -C $(WDSP_PATH)


$(TARGET).hex: $(TARGET)
	$(HEX) $< $@

$(TARGET).bin: $(TARGET)
	$(BIN) $< $@

$(TARGET): $(LIB_NAME).a

.PHONY: libclean
libclean:
	$(MAKE) -C $(WDSP_PATH) clean
	rm -f $(TARGET) $(TARGET).bin $(TARGET).hex $(TARGET).map

.PHONY: clean
clean: libclean

