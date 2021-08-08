CORE = stm32f405rgt

# Only define these target when we're not building the library
ifdef WDSP_PATH

.PHONY: vscode
vscode:
	mkdir -p .vscode
	cp -r $(WDSP_PATH)/boards/$(BOARD)/vscode/* .vscode/
	sed -i'' -e 's#_WDSP_PATH_#$(WDSP_PATH)#;s#_BOARD_#$(BOARD)#;s#_CORE_#$(CORE)#' .vscode/c_cpp_properties.json
	sed -i'' -e 's#_TARGET_#$(TARGET)#' .vscode/launch.json

.PHONY: skel
skel:
	cp $(WDSP_PATH)/boards/$(BOARD)/skel/skel.c ${TARGET}.c
	cp $(WDSP_PATH)/boards/$(BOARD)/config.ini .

endif