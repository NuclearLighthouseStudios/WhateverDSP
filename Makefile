# WhateverDSP library build file

BUILD_DIR ?= build
TARGET ?= libwdsp

ifneq ($(MAKECMDGOALS), clean)
ifeq ($(BOARD), )
$(error Please set the board type)
endif

include boards/$(BOARD)/board.mk
include boards/$(BOARD)/build.mk
include cores/$(CORE)/build.mk
endif

# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

.PHONY: all
all: $(OBJECTS) $(TARGET).a($(OBJECTS))

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir $@

# Include autogenerated dependency information
-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: clean
clean:
	-rm -fR $(BUILD_DIR) $(TARGET).a
