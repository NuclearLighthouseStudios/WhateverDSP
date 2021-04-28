# WhateverDSP project makefile include

ifeq ($(TARGET), )
$(error Please set the target name)
endif

ifeq ($(BOARD), )
$(error Please set the board type)
endif

WDSP_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

include $(WDSP_PATH)/buildvars.mk

C_INCLUDES = \
-I $(WDSP_PATH)/includes/libwdsp \
-I $(WDSP_PATH)/boards/$(BOARD) \
-I $(WDSP_PATH)/cores/$(CORE)

include $(WDSP_PATH)/boards/$(BOARD)/board.mk
include $(WDSP_PATH)/cores/$(CORE)/core.mk

# Reset default goal so it doesn't get overriden by rules in the included files
.DEFAULT_GOAL :=

.PHONY: all
all: $(TARGET) $(TARGET).bin $(TARGET).hex size

.PHONY: size
size: $(TARGET)
	$(SZ) $<

.PHONY: libwdsp
libwdsp:
	$(MAKE) -C $(WDSP_PATH)

$(WDSP_PATH)/libwdsp.a: libwdsp

$(TARGET).hex: $(TARGET)
	$(HEX) $< $@

$(TARGET).bin: $(TARGET)
	$(BIN) $< $@

$(TARGET): $(WDSP_PATH)/libwdsp.a

.PHONY: vscode
vscode:
	mkdir -p .vscode
	cp -r $(WDSP_PATH)/boards/$(BOARD)/vscode/* .vscode/
	sed -i 's#_WDSP_PATH_#$(WDSP_PATH)#;s#_BOARD_#$(BOARD)#;s#_CORE_#$(CORE)#' .vscode/c_cpp_properties.json
	sed -i 's#_TARGET_#$(TARGET)#' .vscode/launch.json

.PHONY: flash
flash: $(TARGET).hex
	st-flash --opt --reset --connect-under-reset --format ihex write $(TARGET).hex

.PHONY: libclean
libclean:
	$(MAKE) -C $(WDSP_PATH) clean
	rm -f $(TARGET) $(TARGET).bin $(TARGET).hex $(TARGET).map

.PHONY: clean
clean: libclean