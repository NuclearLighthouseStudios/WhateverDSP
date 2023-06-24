# ASM sources
ASM_SOURCES +=  \
cores/$(CORE)/startup_stm32f405xx.s

# C sources
C_SOURCES += \
src/sysstubs.c \
cores/$(CORE)/src/system.c

C_INCLUDES += \
-I cores/$(CORE)/includes\
-I cores/$(CORE)