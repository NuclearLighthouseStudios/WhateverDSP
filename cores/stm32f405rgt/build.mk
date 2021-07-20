# ASM sources
ASM_SOURCES +=  \
cores/$(CORE)/startup_stm32f405xx.s

C_INCLUDES += \
-I cores/$(CORE)/includes\
-I cores/$(CORE)