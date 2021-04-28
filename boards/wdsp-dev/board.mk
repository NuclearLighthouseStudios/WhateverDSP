CORE = stm32f405rgt

.PHONY: vscode
vscode:
	cp -r $(WDSP_PATH)/boards/$(BOARD)/vscode/ .vscode
	sed -i 's#_WDSP_PATH_#$(WDSP_PATH)#' .vscode/c_cpp_properties.json
	sed -i 's#_TARGET_#$(TARGET)#' .vscode/launch.json

.PHONY: flash
flash: $(TARGET).hex
	st-flash --opt --reset --connect-under-reset --format ihex write $(TARGET).hex