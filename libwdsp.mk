# WhateverDSP project makefile include

ifeq ($(TARGET), )
$(error Please set the target name)
endif

ifeq ($(BOARD), )
$(error Please set the board type)
endif

WDSP_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

C_INCLUDES = -I $(WDSP_PATH)/includes/libwdsp

include $(WDSP_PATH)/buildvars.mk
include $(WDSP_PATH)/boards/$(BOARD)/board.mk
include $(WDSP_PATH)/cores/$(CORE)/core.mk


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

$(TARGET): $(WDSP_PATH)/libwdsp.a


.PHONY: vscode
vscode:
	cp -r $(WDSP_PATH)/boards/$(BOARD)/vscode/ .vscode
	sed -i 's#_WDSP_PATH_#$(WDSP_PATH)#' .vscode/c_cpp_properties.json
	sed -i 's#_TARGET_#$(TARGET)#' .vscode/launch.json

.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET).bin $(TARGET).hex $(TARGET).map
	$(MAKE) -C $(WDSP_PATH) clean
