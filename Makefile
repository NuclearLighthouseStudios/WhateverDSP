# WhateverDSP library build file

######################################
# Default build variables
######################################
BUILD_DIR ?= build
TARGET ?= libwdsp


######################################
# source
######################################
# C sources
C_SOURCES =  \
src/main.c \
src/system.c \
src/io.c\
src/audio.c\
src/midi.c

# ASM sources
ASM_SOURCES =  \
startup_stm32f405xx.s

# C includes
C_INCLUDES =  \
-I includes \
-I includes/arm \
-I includes/cmsis \
-I includes/stm

include buildvars.mk

#######################################
# build the application
#######################################
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

#######################################
# clean up
#######################################
.PHONY: clean
clean:
	-rm -fR $(BUILD_DIR) $(TARGET).a

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***
