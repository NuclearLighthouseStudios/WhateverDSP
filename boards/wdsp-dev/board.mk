CORE = stm32f405rgt

export BLOCK_SIZE ?= 16
export SAMPLE_RATE ?= 48000

# Pass block size and sample rate to code
C_DEFS += -DBLOCK_SIZE=$(BLOCK_SIZE)
C_DEFS += -DSAMPLE_RATE=$(SAMPLE_RATE)

# Only define these target when we're not building the library
ifdef WDSP_PATH

.PHONY: vscode
vscode:
	mkdir -p .vscode
	cp -r $(WDSP_PATH)/boards/$(BOARD)/vscode/* .vscode/
	sed -i 's#_WDSP_PATH_#$(WDSP_PATH)#;s#_BOARD_#$(BOARD)#;s#_CORE_#$(CORE)#' .vscode/c_cpp_properties.json
	sed -i 's#_TARGET_#$(TARGET)#' .vscode/launch.json

endif