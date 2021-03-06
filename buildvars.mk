######################################
# Default build variables
######################################
export OPT ?= -O3
export DEBUG ?= false
export BOARD ?= wdsp-dev

ifeq ($(DEBUG), true)
export LIB_NAME ?= libwdsp-debug
else
export LIB_NAME ?= libwdsp
endif

ifeq ($(DEBUG), true)
export BUILD_DIR ?= build/debug
else
export BUILD_DIR ?= build
endif

export CONFIG_DIR = $(BUILD_DIR)/config

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
CXX = $(GCC_PATH)/$(PREFIX)g++
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
AR = $(GCC_PATH)/$(PREFIX)ar
else
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
AR = $(PREFIX)ar
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S


#######################################
# CFLAGS
#######################################

# Enable debugging if selected
ifeq ($(DEBUG), true)
C_DEFS += -DDEBUG
endif

# gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -Werror -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -Werror -ffast-math -fdata-sections -ffunction-sections
CXXFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(CXX_INCLUDES) $(OPT) -Wall -Werror -ffast-math -fdata-sections -ffunction-sections

# Always add debugging symbols
CFLAGS += -g -gdwarf-2
CXXLAGS += -g -gdwarf-2


#######################################
# LDFLAGS
#######################################

# libraries
LDLIBS = -lc -lm -lnosys
LIBDIR = -L $(WDSP_PATH)

LDFLAGS = $(MCU) -specs=nano.specs -specs=nosys.specs -u _printf_float -T$(LDSCRIPT) $(LIBDIR) -Wl,-Map=$(TARGET).map,--cref -Wl,--gc-sections


#######################################
# ARFLAGS
#######################################

ARFLAGS = cUrs