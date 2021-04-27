# cpu
CPU = -mcpu=cortex-m4
# fpu
FPU = -mfpu=fpv4-sp-d16
# float-abi
FLOAT-ABI = -mfloat-abi=hard
# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# C defines
C_DEFS += -DSTM32F405xx

# link script
LDSCRIPT = $(WDSP_PATH)/cores/$(CORE)/STM32F405RGTx_FLASH.ld
