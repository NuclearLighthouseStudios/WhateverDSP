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

# Only define these target when we're not building the library
ifdef WDSP_PATH

.PHONY: flash
flash: $(TARGET).hex
	st-flash --opt --reset --connect-under-reset --format ihex write $(TARGET).hex

.PHONY: dfu
dfu: $(TARGET).bin
	dfu-util -d ,0483:df11 -a 0 -c 1 -s 0x08000000 -D $(TARGET).bin

endif