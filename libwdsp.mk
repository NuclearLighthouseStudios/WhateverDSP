# WhateverDSP project makefile include

ifeq ($(TARGET), )
$(error Please set the target name)
endif

WDSP_PATH ?= $(dir $(lastword $(MAKEFILE_LIST)))

C_INCLUDES = -I $(WDSP_PATH)/includes/libwdsp

include $(WDSP_PATH)/buildvars.mk

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

.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET).bin $(TARGET).hex $(TARGET).map
